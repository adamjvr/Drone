# Original Behavior Specification — Shareware Documentation Baseline

## Evidence and scope

This document paraphrases behavioral statements from the **supplied DOS DRONE Shareware v1.01 README** and turns them into black-box validation requirements. Documentation claims are valuable evidence, but executable behavior remains authoritative when edge cases differ.

The supplied shareware documentation states that the shareware build contains **two levels** and the registered game contains **seven**.

This file deliberately separates documented rules from details that still require binary/runtime confirmation.

## Game objective

Drone is a top-view vertically scrolling shooter. The player's mission is not simply to destroy every target: six alien nuclear missiles (“Drones”) must be disarmed with blue probes while enemy escorts and bosses are fought with conventional rapid-fire missiles and red stingers.

The registered-game description says successfully disarming all six Drones leads to an alien Mothership encounter. The supplied shareware evidence unexpectedly contains the Mothership code path and a substantial asset cluster (`hull*`, `panel*`, `damage*`, `hub`, `motor`, `hole`, and related audio), so that behavior can be reverse engineered now. This does **not** mean the registered encounter is part of the normal shareware progression or that full-game parity can be claimed from shareware evidence alone.

## Player movement and primary fire

Documented keyboard behavior:

| action | default key |
|---|---|
| move left | Left Arrow |
| move right | Right Arrow |
| rapid fire | either Ctrl key |
| toggle/load probe vs stinger | Down Arrow |
| launch selected probe/stinger | Up Arrow |
| shield | Space |
| pause | P |
| resume | R |
| end current game | Esc |

Player movement is described in the README as left/right only, with the player ship positioned near the bottom of the vertically scrolling playfield. **The canonical Win32 executable contradicts that description:** its keyboard path also maps `A` to one-pixel upward movement and `Z` to one-pixel downward movement, clamped to Y=120..175. Compatibility work therefore preserves the executable behavior while retaining the README statement as historical documentation evidence. See [`reverse/PLAYER.md`](reverse/PLAYER.md).

## Special weapons and targeting

### Red stinger

Documented behavior:

- stingers home toward enemy ships;
- the targeting system attempts to lock onto the nearest/most threatening enemy when stingers are selected;
- their explosion is larger/more powerful than normal rapid fire and can affect multiple enemies.

Exact target-selection ranking, homing acceleration/turning, blast radius, damage, and collision order remain executable-recovery tasks.

### Blue probe

Documented behavior:

- probes are used to disarm Drones;
- when probes are selected, targeting attempts to lock onto the Drone even before the Drone is visible;
- a launched probe homes toward the Drone;
- after attachment, decoding/disarming takes time and depends on selected skill level;
- enemy ships can shoot an attached probe off, requiring another probe;
- the targeting/communication system handles only one active probe-or-stinger interaction at a time, so an attached probe prevents simultaneous stinger use.

Exact attachment conditions, decode/disarm timers, difficulty scaling, probe vulnerability, and special-weapon state machine remain to be recovered.

## Drone interaction

The documentation explicitly warns that the player's objective is to **disarm**, not destroy/detonate, the Drones. Firing conventional weapons carelessly as a Drone approaches can detonate it.

A warning beep is documented when a Drone is approaching.

## HUD

Documented HUD behaviors:

- six Drone icons are shown;
- the current Drone is boxed/outlined;
- blue icons indicate disarmed Drones;
- red icons indicate detonated Drones;
- probe status is shown in the lower-left HUD area;
- score appears at lower right beneath remaining lives.

Exact pixel coordinates, text/font rendering, icon-state timing, and intermediate probe status values remain renderer/state reconstruction targets.

## Scoring

| event | score delta |
|---|---:|
| destroy small enemy | +1 |
| destroy medium enemy | +25 |
| destroy boss | +100 |
| attach probe | +10 |
| disarm Drone | +500 |
| small enemy escapes | -1 |
| medium enemy escapes | -25 |
| Drone detonates | -1000 |

The executable establishes the edge semantics behind “every 500 points earned.” It keeps **two** signed 32-bit values: total score (`0x004D95F4`) and a separate rolling extra-life progress bucket (`0x004D95F8`). Ordinary positive awards and escape penalties apply the same signed delta to both. When progress reaches 500, state 2 grants exactly one life and subtracts 500, preserving the remainder; only one threshold is consumed per update. Drone detonation is special: score loses 1000 with a floor at zero and extra-life progress is reset to zero. The HUD also floors negative score/progress and performs the original one-step `score -= 9999` quirk whenever score is at least 9999. See [`reverse/SCORING.md`](reverse/SCORING.md).

## Boss ordering

Boss ships are documented as appearing immediately before Drones. Exact boss triggers, health/damage model, movement, and relationship to level sequencing remain open.

## Difficulty/skill

The documentation states that probe decoding/disarming duration depends on selected skill level. It does not provide the numerical scaling in the behavior section. Difficulty selection/state and every gameplay quantity affected by it must be recovered from the executable.

## Joystick behavior

The DOS documentation describes configurable standard joystick support. Configuration asks the user to assign/calibrate at least:

- rapid fire;
- probe/stinger load-toggle;
- probe/stinger launch;
- shield;
- joystick center/calibration.

It also describes top-hat use as useful for simultaneous firing plus special-weapon control on hardware where ordinary buttons cannot be registered concurrently.

The executable comparison now confirms that architecture: DOS game-port/keyboard handling and Win32 DirectInput/keyboard handling converge on independent semantic action tests, and both substitute the same six demo channels during playback. The clean engine therefore normalizes physical devices into `GameplayInputFrame` instead of reproducing old game-port/DirectInput layouts. See [`reverse/INPUT.md`](reverse/INPUT.md).

## Cheats/debug-like controls documented to players

### `L` — nine lives

The executable proves this behavior end-to-end. Pressing `L` **sets** `player_lives` to 9, sets `high_score_disqualified` (`0x004461D8`) to 1, enters message state 99, and renders `<L> KEY GRANTED NINE LIVES!` plus `DISQUALIFIED FROM HIGH SCORES`. The post-results qualifier explicitly skips the high-score table when that flag is nonzero. Demo playback is independently ineligible as well. See [`reverse/HIGH_SCORES.md`](reverse/HIGH_SCORES.md).

### `Tab` — vertical retrace sync toggle

The README states that `Tab` toggles “vertical retrace sync”; turning it off makes the game run much faster and is presented as a novelty/cheat-like behavior.

This is highly relevant to timing reconstruction. Phase 2 now proves the Win32 mapping directly: `VK_TAB` negates global `0x0042B1B4` between `+1/-1`, the same feedback path renders `VERTICAL RETRACE` plus `SYNC ON/OFF`, and `+1` enables the recovered 15,000-QPC-count pacing wait. The separate DirectDraw `WaitForVerticalBlank` path remains independently mapped. The true simulation cadence is still unresolved because the executable never queries QPC frequency and display waits do not by themselves prove update rate.

## Performance-era behavior

The DOS README notes materially different performance under pure DOS versus running in a Windows DOS shell on period hardware, including possible slow graphics or audio hiccups. This is historical context, not a target requirement for the remaster. The compatibility target is intended game behavior, not reproducing host DMA starvation.

## Black-box test IDs to create as subsystems are recovered

| ID | behavior target | current status |
|---|---|---|
| `BHV-CTRL-001` | player directional movement | README says left/right only; Win32 proves Left/Right plus A/Z vertical movement and exact bounds; clean reconstruction implemented |
| `BHV-WPN-001` | Ctrl/Fire launches normal rapid-fire weapon | Win32 path recovered: 8-slot 1x9 missile pool, cooldown/spawn/update/top cleanup implemented; target collision semantics still expanding |
| `BHV-SHIELD-001` | Space consumes rechargeable protection energy | Win32 75-unit high-word accumulator, recharge/drain ordering, active flag, bomb protection and sound cadence mapped; clean state model tested |
| `BHV-WPN-002` | Down loads/cycles special type; Up launches | Win32 lifecycle mapped and clean reconstruction tested |
| `BHV-PROBE-001` | probe targets Drone and can attach/disarm | Win32 target/attach/decode timing and demo-mode thresholds mapped; later disarm consequences still partial |
| `BHV-STING-001` | stinger targets hostile and homes | frame-1 homing and generic impact-consumed lifecycle mapped; per-enemy damage consequences partial |
| `BHV-TARGET-001` | only one probe/stinger targeting communication active | documented; implementation pending |
| `BHV-HUD-001` | six Drone indicators with current/disarmed/detonated states | executable outcome ledger now maps raw 0/1/2 as unresolved/disarmed/detonated; renderer details still pending |
| `BHV-SCORE-001` | scoring table deltas | documented values retained; Win32 total/progress accumulators, normal signed deltas, Drone special penalty, negative floors and 9999 HUD quirk recovered and clean-tested |
| `BHV-LIFE-001` | extra life every 500 points earned | Win32 rolling progress bucket recovered; exactly one 500-point threshold consumed per state-2 update with remainder preserved; clean-tested |
| `BHV-CHEAT-001` | L grants nine lives and invalidates high score | executable proves set-to-9, disqualification flag, message state/text, and post-results qualification bypass; clean eligibility logic tested |
| `BHV-TIME-001` | Tab toggles retrace/pacing and off runs faster | Win32 key/global/UI/QPC mapping and DOS scan-code/global/VGA-retrace gate proven; canonical simulation cadence remains open |

As each rule is recovered, this table should point to trace/tests rather than remaining a documentation-only checklist.
