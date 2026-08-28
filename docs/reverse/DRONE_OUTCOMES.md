# Six-Drone Objective Outcomes and Results Selection — Win32 Shareware

Drone's central mission objective is represented explicitly in the Win32 executable as a six-entry outcome ledger. This document records the recovered status values, progression counter, and the post-game art/music selection derived from them.

## Canonical objective count

The original documentation states that the mission contains six nuclear Drones to disarm. The executable independently enforces the same count: the resolution counter at `0x00433B54` is incremented when a Drone outcome is committed and multiple paths compare it against `6`.

The clean compatibility model therefore uses exactly six outcome slots.

## Outcome ledger

The byte array rooted at **`0x00472590`** stores the per-Drone result for already-processed objectives.

Established values are:

| raw value | semantic meaning | evidence |
|---:|---|---|
| `0` | unresolved / no committed result | result-commit paths test before assigning; initialization begins at zero |
| `1` | **disarmed** | post-game counts value 1 and uses that count to select `disarm0.jba` through `disarm6.jba` |
| `2` | **detonated** | destructive Drone path writes 2; post-game counts it separately from disarmed outcomes |

The scalar **`0x00433B54`** is the number/index of Drone objectives whose progression has advanced far enough to be included in the result ledger. Result-selection code loops only over this count rather than blindly reading all six entries.

## Normal objective travel and hover

The normal state-2 Drone route is now partitioned tightly enough to execute in clean code. A full campaign reset initializes the `drone.jba` entity at **X=155, Y=-850** and initializes shared settlement scalar `0x004D9600` to **61**. Normal travel is gated to gameplay phase 2.

Before Probe completion, Drone Y advances by one while it is below 45. Two approach sound/effect landmarks do not persist as end-of-update positions: reaching **-117** is immediately replaced by **-116**, and reaching **-40** is immediately replaced by **-39**. At Y=44 the 16-bit hold counter at `0x004460B6` is reset; when the unresolved Drone reaches **Y=45**, that counter advances once per phase-2 update and the Drone holds in place. The exact timeout comparison is **4200** phase-2 hold ticks. That timeout starts the separate destructive countdown path; clean Phase 4 exposes the timeout event but does not yet fold the detonation lifecycle into the normal objective owner.

Completed Probe decode is represented by status value 1 at `0x0045BEEA`. On that path the Drone receives its single phase-2 Y increment in the later branch, allowing it to leave Y=45. After the normal Y=201 commit described below, it continues to Y=230, which resets settlement to zero. The next completed-disarm phase-2 movement reaches **Y=231**; because the original clears the completed-decode status once Y is greater than 230, Y then remains fixed at 231 while settlement continues toward 60.

The pre-boss dispatch occurs later in the same state-2 region and compares the owned Drone Y directly against **-200**. Boss activation can therefore be derived from Drone motion itself rather than supplied as an external event.

## Outcome commits

Two particularly clear producers establish the status roles:

- the destructive path around `0x0040BF94` writes `outcomes[processed_count] = 2`, increments the count, and stops the six-Drone progression once the new count reaches 6;
- the normal Drone-exit/progression path around `0x0040E6E3` accepts the non-destruction path at exactly Drone Y=`201`; if the current slot is still unresolved it writes `1`, increments the processed count, and advances Drone Y to `202`. Drone activity `2` is explicitly excluded;
- the destructive path commits `2` immediately in the detonation progression path and increments the same processed count.

After a normal disarm commit, reaching Drone Y=`230` resets shared settlement scalar `0x004D9600` to zero. That scalar advances only on gameplay phase `2`, saturating at `61`; once Drone Y is greater than `230`, the mission transition is invoked when the scalar is exactly `60`. This establishes a deliberate 60-slow-phase settlement delay between leaving the playfield and entering the objective interstitial.

The result values, commit boundary, and post-disarm settlement gate therefore no longer need provisional names.

## Drone detonation effect timing and the post-trajectory call

The destructive setup routine at **`0x0041D220`** is now established as `trigger_drone_detonation_sequence`. It captures the current Drone sprite center into `0x004603A0/0x004603A4`, sets the Drone common-entity activity byte (`0x004461C2`) to `2`, resets the destruction tick `0x0042B1B0` and the Drone-context `+0x32` settlement field, and performs the already-established -1000 score / extra-life-progress reset.

The previously unnamed state-2 call at **`0x0041E4D0`** is its update-side counterpart, `update_drone_detonation_effect`. The state-2 orchestrator calls it after trajectory processing with the current four-phase gameplay substep. Its expensive body runs only when all three conditions hold:

```text
gameplay_substep_phase == 0
drone_detonation_tick > 25
drone.activity == 2
```

On those eligible calls it drifts the captured Y center down by one, emits four randomized explosion sprites around the center using `spawn_explosion_sprite`, and performs the large direct-framebuffer radial/distortion/fade effect used by the Drone destruction presentation. The logical destruction tick itself is incremented once per state-2 update and capped at **330** by the earlier orchestrator bookkeeping. At tick 329 the updater resets the Drone-context `+0x32` settlement field; once the capped tick is greater than 329, eligible phase-0 calls increment that field. The earlier state-2 gate advances the outcome/destruction path only after this field exceeds 70.

This separates two timing domains that should remain distinct in clean code: the detonation tick advances every logical gameplay update, while the expensive visual/settlement updater runs only on one of the four gameplay substeps. The direct framebuffer distortion itself remains a fidelity-rendering reconstruction target rather than being approximated inside the portable gameplay core.

## Per-objective mission interstitial and encounter handoff

`0x0041D690` is now established as `run_mission_outcome_transition`. Before it changes scenery or encounter resources, it reduces the already-processed outcome prefix and presents two mission cards.

The first card reports the just-resolved outcome:

- a detonated Drone selects `badN.jba`, where `N` is the cumulative detonated count;
- a disarmed Drone selects `goodN.jba`, where `N` is the cumulative disarmed count.

The accompanying sound is `detonate.wav` for a detonated last outcome and `deepness.wav` for a disarmed last outcome.

The second card is indexed by total processed objectives:

```text
1 -> mission1.jba
2 -> mission2.jba
3 -> mission3.jba
4 -> mission4.jba
5 -> mission5.jba
6 -> miss6yes.jba  if detonated count == 0
6 -> miss6no.jba   otherwise
```

This is a **mission interstitial**, not the later post-game results screen. After the interstitial, the same routine performs the scenery/resource transition and calls `initialize_gameplay_session(0)`. The zero argument is now proven to mean **encounter-only reset**: it skips campaign-wide initialization and therefore preserves score, lives, the six outcome slots, and processed count. `initialize_gameplay_session(1)` remains the full new-campaign reset.

The clean, asset-independent reconstruction lives in `gameplay/mission_progression.*` and returns semantic good/bad, audio, briefing, and encounter-transition plans without embedding the proprietary files.

## Post-game count reduction

The post-game/results region beginning at `0x004115BE` computes two counts over the processed prefix of the outcome array:

```text
disarmed  = count(outcome == 1)
detonated = count(outcome == 2)
```

The disarmed count is then used directly as a jump-table index for the result art:

```text
0 -> disarm0.jba
1 -> disarm1.jba
2 -> disarm2.jba
3 -> disarm3.jba
4 -> disarm4.jba
5 -> disarm5.jba
6 -> disarm6.jba   (normal variant)
6 -> disarm6m.jba  (when raw completion-variant byte 0x004726DA == 2)
```

This result-screen usage is the strongest independent semantic proof that raw outcome value 1 means **disarmed**.

## Results music selection

The same region selects one of four WAV files using the exact branch order below. The former `mothership_destroyed` placeholder is now resolved: `0x004726DA` is `mothership_core_target_entity + 0x142`, the common-entity activity/state byte. Value `2` is the established Mothership destruction-sequence outcome used by the results path.

```text
if disarmed == 6 and mothership_destroyed:
    hiphop.wav
else if detonated > 4:
    moon.wav
else if disarmed < 4:
    suspense.wav
else:
    choral.wav
```

The order is fidelity-significant. For example, a result with six detonations satisfies both `detonated > 4` and `disarmed < 4`, but the original chooses `moon.wav` because that branch is tested first.

## Clean implementation

The independently written representation lives in:

- `include/drone/gameplay/mission_outcome.hpp` / `src/gameplay/mission_outcome.cpp` for final-results reduction;
- `include/drone/gameplay/mission_progression.hpp` / `src/gameplay/mission_progression.cpp` for objective commit, settlement, interstitial, reset-scope, and encounter-transition contracts;
- `include/drone/gameplay/drone_objective.hpp` / `src/gameplay/drone_objective.cpp` for the normal phase-2 Drone travel, Y=45 hold/timeout landmark, completed-disarm departure and settlement gate;
- synthetic cases in `tests/test_gameplay.cpp`, `tests/test_drone_objective.cpp`, and `tests/test_game_session.cpp`.

The clean model exposes:

```text
Unresolved = 0
Disarmed   = 1
Detonated  = 2
```

and reproduces the exact disarm-art index plus results-music branch ordering without embedding original artwork or audio.

## Relationship to the larger results flow

The surrounding Win32 region beginning at `0x004115BE` is now partitioned at the gameplay/control-flow level. It is entered when lives are non-positive, optionally presents these result choices and six statistics, enforces a 58-presentation confirmation lock, enters the shareware Ordering Information modal, performs high-score qualification/insertion, and runs completion credits for the six-disarm Mothership outcome. See [`POST_GAME_FLOW.md`](POST_GAME_FLOW.md).

## Open questions

- Complete semantics of Mothership core-target states other than the established state-2 destruction outcome; see [`MOTHERSHIP.md`](MOTHERSHIP.md).
- Complete integration of the already-recovered destructive timeout/countdown/detonation/life-loss branch into `GameSession`.
- DOS-side storage/selection correspondence for the same six objective outcomes.
