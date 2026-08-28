# Mothership Encounter Reconstruction

## Scope and evidence boundary

This document records the executable-backed reconstruction of Drone's alien Mothership encounter. The supplied README describes the Mothership as the final registered-game sequence reached after all six Drones are successfully disarmed.

The supplied shareware evidence contains a substantial Mothership code and asset cluster in both the DOS and Windows corpora. That allows the encounter machinery to be reverse engineered now, but it does **not** by itself prove that the normal shareware progression exposes the complete registered encounter. Full-game parity remains a later evidence milestone.

## Encounter transition

The Win32 transition contract is now exact in the supplied PE. `run_mission_outcome_transition` counts the processed six-Drone ledger; after processed count reaches `6`, it enters the Mothership branch **only when the cumulative detonated count is zero**. In that path it reloads the river three-screen scenery stack, performs `initialize_gameplay_session(0)` (the encounter-only reset that preserves campaign score/lives/outcomes), releases the Bomber family, and calls `0x00413290`. If even one of the six Drones detonated, branch 6 instead forces the run toward results.

Therefore the compiled relationship is exactly **all six Drones disarmed -> Mothership endgame**. The canonical shareware campaign still normally stops after objective two; later branches are dormant evidence until compared against a lawful retail reference.

`0x00413290` is established as `load_mothership_assets` because it loads the following coherent asset family:

- `hull0.jba` through `hull3.jba`;
- `panel0.jba` through `panel3.jba`;
- `damage0.jba` and `damage1.jba`;
- `hub.jba`;
- `motor.jba`;
- `hole.jba`.

The same filenames are present in the DOS shareware corpus. That correspondence is now executable-backed rather than filename-only: DOS `0x0006C964` independently loads the same 13 core JBA names and DOS `0x0006CDB0` is its paired guarded cleanup. Win32's corresponding pair is `0x00413290` / `0x00413870`.

Because DOS/4GW LE data references are stored as fixups, `scripts/find_dos_le_internal_xrefs.py` recovers the canonical internal-reference patch sites. The resulting metadata-only evidence is checked into `reverse/dos/mothership_asset_xrefs.csv`; it places all 13 asset references in the DOS loader's logical page 93.

`lid.jba` is **not** part of this Mothership loader. Its ownership is now established separately: Win32 `0x00417350` loads `lid.jba`, `top.jba`, `retro1.wav`, and `level1.wav` for a reusable boss family documented in [`LID_TOP_BOSS.md`](LID_TOP_BOSS.md). This correction prevents two visually/mechanically distinct composite encounters from being merged merely because both contain a “hole/lid/top”-like concept.

## Composite object family

The encounter is not represented as one monolithic sprite. It uses several normal `0x154` common entities and related object pools for hull, panels, damage, hub, motors, hole/weak-point presentation, and a central collision target.

The central target entity is rooted at:

```text
0x00472598
```

It is initialized through the common entity constructor as a 16×12 object. Its standard common-entity activity byte is therefore:

```text
0x00472598 + 0x142 = 0x004726DA
```

This resolves a previous documentation mistake: `0x004726DA` is **not** an independent “result variant” global. It is the Mothership target entity's ordinary `+0x142` activity/state byte.

The update path positions this target relative to the moving Mothership composite. One observed placement uses offsets of approximately `+50,+56` from the surrounding hull/composite position path. Exact ownership of every positional base and subassembly remains under reconstruction.

## Probe/Stinger relationship

The special projectile's state-4 path is part of the Mothership encounter.

A launched special first collides with the `hole.jba` entity, entering the established `HoleInteraction` state. A later collision tests the central target at `0x00472598`.

For Stinger/frame 1, the collision path examines two motor/subassembly activity states. If those prerequisites are already satisfied, it writes:

```text
mothership_core_target_entity.activity = 2
mothership_destruction_counter = 0
```

If the motor prerequisites are not yet satisfied, the branch instead selects one of the active motor-like subobjects and advances that object's state. This gives the Stinger path an evidence-backed relationship to disabling Mothership subassemblies before the final core/destruction sequence.

The exact names and complete state machines of those two motor objects remain open; the project does not invent additional boss mechanics beyond the observed transitions.

## Destruction sequence

When the central target's activity state is `2`, the encounter enters a timed destruction sequence.

Important globals:

| address | semantic name | established role |
|---|---|---|
| `0x00472598` | `mothership_core_target_entity` | common `0x154` target entity |
| `0x004726DA` | target `+0x142` state | state `2` gates destruction sequence/results outcome |
| `0x004725CA` | `mothership_destruction_counter` | sequence progression counter |
| `0x004725D0` | `mothership_destruction_award_threshold` | initialized to `15` |
| `0x00464B90` | `mothership_destroyed` | live 0/1 completion statistic persisted to high scores |

The threshold is initialized to **15**. It is a timing/progression threshold, not a “15 hits” count.

On the update phase where the incremented destruction counter reaches 15, Win32 performs the following established actions:

```text
score                 += 500
extra_life_progress   += 500
play thunder2.wav
mothership_destroyed   = 1
```

This is visible directly around `0x00412C35..0x00412C89`.

The same counter then continues well beyond 15. Later thresholds drive randomized explosion/debris activity and progressive teardown; after counts greater than roughly 100 and 170, terminal encounter/run behavior becomes visible. Those later stages are only partially named and remain active Phase 2 work.

## High-score statistic

The live scalar `0x00464B90` is copied into the fourth logical numeric field of a high-score entry. That field is therefore semantically established as **Mothership destroyed**.

The legacy score record is now:

```text
name
Drones disarmed
score
Mothership destroyed
Percentage hit
```

The persistent Mothership field is stored as a signed 16-bit decimal integer even though the established producer uses Boolean-style values `0` and `1`.

See [`HIGH_SCORES.md`](HIGH_SCORES.md) and [`../formats/SCORES.md`](../formats/SCORES.md).

## Results-screen connection

The post-game/results region checks the same target `+0x142` state. State `2` participates in the all-six-Mothership-success presentation path, selecting `disarm6m.jba` and the `hiphop.wav` branch under the already recovered result conditions.

This resolves the old `completion_variant_two` placeholder: the code is consulting Mothership core-target state, not an unrelated result-mode flag.

See [`DRONE_OUTCOMES.md`](DRONE_OUTCOMES.md).

## What is established vs open

### Established

- the post-six-Drone Mothership transition exists in the supplied Win32 executable and requires zero detonated outcomes (all six Drones disarmed);
- the transition reloads river scenery, uses the encounter-only reset, releases Bomber resources and then loads/stages the Mothership;
- `0x00413290` loads the coherent Mothership asset cluster;
- `0x00472598` is a normal common entity owned by the encounter;
- `0x004726DA` is exactly that entity's `+0x142` state;
- state-4 Stinger collision can advance motor/subassembly state and ultimately set core state `2`;
- core state `2` gates the destruction sequence;
- the destruction threshold is 15 progression counts;
- reaching it awards +500, plays `thunder2.wav`, and sets `mothership_destroyed=1`;
- that live flag is persisted into high-score records;
- later result presentation reuses core state `2`.

### Open

- complete semantic names for every hull/panel/motor/hub/damage subobject and every state value;
- complete Mothership movement and attack behavior;
- exact mechanics leading each motor/subassembly to its prerequisite state;
- complete late destruction-sequence thresholds and visual/audio effects;
- DOS gameplay/destruction-state correspondence beyond the now-established load/release asset lifecycle; DOS `0x00085D10` is now classified as a separate Mothership/orders presentation routine because it loads `thunder2.clv` plus the Mothership resources and cycles `order1.jba`..`order5.jba` but does not modify score or the live Mothership-result scalar;
- whether any shareware-accessible state can enter the full encounter despite the README's registered-game description.

These remain tracked in `Q-MSHIP-001` and the structure/function ledgers.

## Presentation ownership

The ordinary state-2 presentation tail renders the Mothership composite first
after world-viewport composition (`0x004100DD..0x00410230`). The recovered
component ownership is:

- hull entity `0x0043FE68` — `hull0.jba`..`hull3.jba`;
- panel entity `0x00446C90` — `panel0.jba`..`panel3.jba`;
- damage entities `0x00472848` / `0x004726F0` — `damage0.jba` / `damage1.jba`;
- hub entity `0x00440120` — `hub.jba`;
- motor entities `0x00491A00` / `0x00491B58` — shared `motor.jba` frames;
- hole entity `0x00433700` — `hole.jba`;
- central collision/core owner `0x00472598`.

When special state `0x0045A28A == 4`, the hole and special projectile are drawn
before the normal component composite. Otherwise hole visibility and the
panel/hull/hub/motor-versus-damage choices follow their component activity
states. The central core target itself is collision/state ownership and is not
simply another unconditional ordinary sprite at this call site. See
[`WORLD_PRESENTATION_SUBPASSES.md`](WORLD_PRESENTATION_SUBPASSES.md).
