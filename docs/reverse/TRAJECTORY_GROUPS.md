# Trajectory Group Lifecycle

**Status:** Win32 trajectory-group lifecycle, all 17 fixed startup templates, persistent primary-group replenishment, the common live transient-wave producer, and clean whole-session ownership/destruction dispatch are established; special-family producers and exact runtime trace parity remain later work.

This document describes the publishable semantic contract recovered from `update_trajectory_groups` at `0x00415FA0`. It records addresses, fields, constants, and behavior without distributing original executable bytes or original FLY payloads.

## Pool organization

The canonical Win32 build owns a fixed trajectory-group pool at `0x00495CF0`:

- `trajectory_group_count` at `0x0042B18C` is initialized to **17**;
- each group record is separated by a fixed **`0x2148` byte stride**;
- `active_trajectory_group_count` at `0x0042B190` tracks how many group records are currently live;
- each group begins with a `0x14`-byte recovered header followed by inline common `0x154`-byte entities.

The clean engine does not mirror the original packed record. It exposes a semantic `TrajectoryGroupLifecycle` containing only established lifecycle fields.

## Header fields

| offset | width | established meaning |
|---:|---:|---|
| `+0x00` | 1 | group mode |
| `+0x01` | 1 signed | fixed inline entity count |
| `+0x02` | 1 | currently active entity count |
| `+0x04` | 4 | pointer to adjacent X/Y/AUX trajectory-pointer slots |
| `+0x08` | 2 signed | group X offset |
| `+0x0A` | 2 signed | group Y offset |
| `+0x0C` | 2 signed | stagger/spawn-delay counter |
| `+0x0E` | 2 signed | stagger/spawn-delay interval |
| `+0x10` | 2 signed | number of fixed inline slots activated so far |
| `+0x14` | — | first inline `0x154` entity |

The record stride is larger than the minimum header-plus-observed-entities description, so template-specific capacity/padding beyond the fields above remains an archaeology question. Clean code should not infer unobserved members merely from the stride.

## Group modes

The byte at `+0x00` has four behaviorally established values:

| raw | clean name | behavior |
|---:|---|---|
| 0 | `Inactive` | group is not live |
| 1 | `PersistentLoop` | active trajectory followers wrap and continue |
| 2 | `RetireOnPathWrap` | active path-following entities retire when their just-advanced path index wraps |
| 10 | `BreakawayFlyOff` | active members abandon their FLY path and accelerate toward off-screen targets |

Mode 2 is **not demo-only**. Replay-scripted waves use it, but live wave-spawn paths also create mode-2 groups. The semantic distinction is lifetime behavior: the entity exits when its path completes.

## Trajectory-family entity activity

Within this subsystem, common entity byte `+0x142` has at least these meanings:

| raw | clean name | trajectory-family behavior |
|---:|---|---|
| 0 | `Inactive` | slot is not participating |
| 1 | `FollowingPath` | consume the current X/Y/AUX sample normally |
| 3 | `AcquiringPath` | move by two pixels per axis toward the current path-derived target; become state 1 when both coordinates match |

These meanings are contextual to trajectory-owned entities. Other object families reuse `+0x142` for their own state machines.

## Staggered activation

The primary group (index 0) does not use the fixed-slot stagger branch. For non-primary groups, each logical update performs a 16-bit increment of `+0x0C`.

When the incremented counter equals `+0x0E` and `activated_entity_count < entity_count`:

1. the old activated count identifies the next inline entity;
2. `activated_entity_count` increments;
3. byte-wide `active_entity_count` increments;
4. the delay counter resets to zero;
5. the selected entity becomes activity 1;
6. its trajectory index is reset to zero.

A subtle original behavior is preserved: once every fixed slot has activated, hitting the interval again does **not** reset the counter. The counter simply continues incrementing.

Transient-wave creation seeds the first entity as active, sets both active and activated counts to 1, initializes mode 2, and increments the global active-group count.

## Path progression and mode-2 retirement

For each trajectory-owned slot, `0x00415FA0` performs the already recovered path step:

```text
path_index = int16(path_index + path_step)
if path_index > path_end_index:
    path_index = 0
```

The comparison is signed and the add is 16-bit. When that wrap occurs:

- mode 1 continues looping;
- mode 2 retires the entity only if its trajectory activity is already 1;
- activity 3 survives the wrap and continues acquiring the current path target.

A mode-2 escape also applies the original entity score value as a negative score/extra-life-progress event. The clean lifecycle helper deliberately handles only the group-count transition; scoring remains in the scoring subsystem so responsibilities do not become entangled.

Every retirement path decrements the byte-wide active count. When it reaches zero, the group mode becomes 0 and the global active-group count decrements.

## Mode-10 breakaway

A non-primary live group can randomly convert to mode 10 only when all of these are true:

- demo playback is off;
- recording mode is off;
- the updater's phase argument equals 2;
- every fixed group slot has already activated;
- the group is not already in mode 10;
- `rand() % 300 < drone_outcome_processed_count`.

This makes breakaway probability increase with mission progression and deliberately disables the transition during deterministic replay/recording paths.

### Breakaway initialization

For each active member:

- integer X/Y are copied to 16.16 fixed-point scratch coordinates;
- X and Y speeds start at `0x8000`;
- horizontal target is `-60` or `321`;
- vertical target is `-60` or `201`.

The target choices sit just beyond the cleanup boundaries.

### Breakaway update

Each axis independently:

1. adds **700** to speed;
2. caps speed at **`0x28000`**;
3. moves the 16.16 coordinate toward its target;
4. derives the visible integer position with arithmetic shift-right by 16 semantics.

During update phase 2, horizontal movement also animates the entity:

- moving right decrements current frame with wrap;
- moving left increments current frame with wrap.

The direction/frame relationship is intentionally preserved even though it may look reversed to a clean-room implementer.

An entity retires after crossing any original limit:

```text
x < -59 or x > 320 or y < -59 or y > 200
```

The same active-count/group teardown rule then applies.

## Persistent primary-group replenishment

Before the phase-2 transient-wave producer, Win32 `0x0040CEE8..0x0040D070` maintains the persistent seven-member group 0 on every shared gameplay substep **except phase 2**. The branch consumes `rand() & 0x7ff` first. Demo playback or a currently empty primary group replaces the effective roll with `1`; otherwise replenishment requires:

```text
(rand() & 0x7ff) < 4 * (processed_drone_count + difficulty)
```

The capacity gate then requires active primary actors below the fixed group count and Drone activity other than destruction state 2. The first inactive inline actor is selected in ascending slot order. A second draw chooses its acquisition entry:

- `rand()%100 < 34` -> `(-30,100)`;
- `34..65` -> `(160,-30)`;
- `>65` -> `(350,100)`.

Only X/Y and activity are rewritten; activity becomes `3` (`AcquiringPath`). The actor's retained path index and frame are intentionally **not reset**, so the normal trajectory updater seeks its retained current-path sample. If group 0 had become inactive, the producer restores mode `1` and increments the active-group count. It always increments group-0 active actor count and encounter-local alien total `0x00466B04`. Normal live control then jumps to `0x0040D390`; the nearby `0x0040D25B` optional flight-SFX RNG tail belongs to the separate demo-scripted formation activation path and is **not** consumed by primary replenishment.

## Live transient formation producer

The normal-live producer at `0x0040D390..0x0040D947` is now recovered and clean-integrated rather than supplied as an already-selected group from the host. It runs only on shared gameplay phase 2 while demo playback is off.

Encounter setup derives two signed 16-bit scheduler values from processed-Drone count `P` and difficulty `D` (`1..3`):

```text
interval_threshold = 310 - 20*P - 30*D
interval_counter   = interval_threshold - 30
```

On every eligible phase-2 call, the counter advances by **3**. Reaching/exceeding the threshold resets the counter to zero and forces the current spawn chance to `1200`. Otherwise the ordinary live chance is `3*P + 4*D`; recording mode substitutes **28**. The producer always consumes `rand()%1200` before the later position/activity gates, so a suppressed attempt still advances the original CRT RNG.

After a passing roll, spawning is suppressed when Drone Y is at least 200, the active-group count is already `group_count-1` (16), Drone activity is destruction state 2, or the registered-only Mothership-core destruction state is 2.

The fixed-group draw is progression-dependent:

- `P < 2`: `rand()%12`, so canonical shareware progression draws only groups **0..11**;
- `P == 2 or 3`: `rand()%16 + 1`, but candidates **12/13** are rerolled through `rand()%12`;
- `P >= 4`: `rand()%16 + 1`.

A subtle pool rule matters: an initially selected **group 0 is legal if it has become inactive**. Busy scanning increments the index and wraps `17 -> 1`, so group 0 is not revisited after the first candidate. This corrects the earlier simplified description of the live producer as strictly non-primary.

Once a free group is chosen, it becomes mode 2 and consumes the original optional flight-SFX RNG (`rand()&0x7f < 0x50`, then `rand()%14`). Runtime path/offset substitution then follows the selected template family:

- Swarm: X offset `rand()%200 - 50`;
- Swoop/NewCurly: X offset `rand()%250 - 50`;
- Frisbee1/Generated402/Generated422: Y offset `rand()%100 - 50`, X zero;
- Frisbee2: X zero;
- LeftDrop/RightDrop: X zero and a `rand()%10` coin flip selects left vs right drop;
- Loop/LeftDive/RightDive/default dive family: a `rand()%10` coin flip selects left vs right dive, then X offset `rand()%200 - 100`.

For non-Swarm live non-recording waves, one `rand()%22` decides whether every fixed actor receives independent X/Y formation offsets in `[-30,29]`; the randomization branch is taken when the roll is below `P+2`. Otherwise all actor formation offsets are zeroed. The common activation tail then resets every slot, activates slot 0 at sample zero and seeds the mode-2 stagger lifecycle already documented above.

A successful transient first-actor activation increments original scalar `0x00466B04`. Cross-site recovery establishes that scalar as the **encounter-local alien total** paired with encounter-local hit counter `0x0047EC3C`: reset seeds 7/0; primary replenishment and transient first-actor insertion increment only the local total; each later non-primary stagger activation at `0x0041610E` increments **both** local total and mission-wide `0x00446078`. Likewise, a rapid-missile trajectory destruction at `0x004165E4` increments both local hit and mission-wide `0x0044084C`, while the **Stinger-display AoE** destruction at `0x0040EFD1` increments local hit only; the launched Probe/Stinger direct-destruction scan is a separate path and does not increment that counter at the recovered site. The encounter interstitial still renders hit/miss/total/percentage from the local pair and `0x0041E237` later adds that entire pair into the mission pair. The original therefore double-counts stagger activations and rapid-missile kills in mission Results; the clean engine preserves that quirk explicitly rather than aliasing or normalizing the counters.

## FLY relationship

Normal path-following entities use the parallel FLY X/Y/AUX arrays documented in [`../formats/FLY.md`](../formats/FLY.md). AUX controls sprite animation; the group lifecycle described here determines when an entity follows, acquires, loops, retires, or abandons that path.

The canonical `Rightdiv.fly` and `Swarm.fly` loader over-reads do not require synthesized samples for recovered canonical group cycles because their group end indices never reach the missing requested slot.


## Canonical 17-template catalog

The fixed pool is not a homogeneous set of empty records. The contiguous startup initializer at `0x00409060..0x00409CD5` constructs all 17 templates, including their path family, sprite geometry, frame-bank size, inclusive path end, stagger interval, and the few non-zero per-slot offsets/index seeds. The publishable machine-readable form is [`../../manifests/trajectory_group_templates.csv`](../../manifests/trajectory_group_templates.csv); the clean executable form is `trajectory_templates.*`.

| group | runtime family | entities | stagger | inclusive end | sprite | frames | startup notes |
|---:|---|---:|---:|---:|---:|---:|---|
| 0 | Loop | 7 | 0 | 375 | 28×21 | 15 | mode 1; all 7 active; path indices `0,53,106,159,212,265,318`; explicit path step +1 |
| 1 | LeftDive | 6 | 18 | 117 | 26×26 | 16 | inactive |
| 2 | LeftDive | 7 | 13 | 117 | 28×21 | 15 | inactive |
| 3 | Swarm | 4 | 1 | 945 | 35×22 | 16 | diamond offsets `(0,-25),(25,0),(0,25),(-25,0)` |
| 4 | Swarm | 1 | 1 | 945 | 44×46 | 16 | inactive |
| 5 | Swoop | 7 | 13 | 185 | 28×21 | 15 | inactive |
| 6 | NewCurly | 9 | 10 | 230 | 35×30 | 32 | inactive |
| 7 | Frisbee1 | 9 | 9 | 935 | 14×12 | 32 | inactive |
| 8 | Frisbee2 | 9 | 5 | 425 | 14×12 | 32 | inactive |
| 9 | LeftDrop | 5 | 16 | 195 | 44×46 | 16 | inactive |
| 10 | LeftDrop | 6 | 12 | 195 | 35×22 | 16 | inactive |
| 11 | LeftDrop | 6 | 12 | 195 | 26×26 | 16 | inactive |
| 12 | LeftDive | 4 | 18 | 117 | 31×41 | 32 | inactive |
| 13 | LeftDrop | 5 | 16 | 195 | 31×41 | 32 | inactive |
| 14 | Generated402 | 6 | 11 | 402 | 16×11 | 16 | startup-generated path descriptor at `0x004462F0` |
| 15 | Generated422 | 6 | 11 | 422 | 16×11 | runtime descriptor `0x004462E0`; inactive startup X/Y are nevertheless seeded from Generated402 |
| 16 | LeftDive | 6 | 11 | 117 | 23×23 | 32 | inactive |

Every non-primary template starts with mode 0, active count 0, path index 0, and entity activity 0. Group X/Y offsets are zero in the canonical startup state. `sprite_entity_init` (`0x00401780`) does **not** initialize common-entity `+0x36`, so clean code records an explicit static path step only for group 0, where the initializer actually writes `+1`; it does not manufacture a path-step value for the other templates.

The group-15 startup sample-source mismatch is preserved as evidence rather than “fixed.” Its group header points to the Generated422 descriptor, but its inactive slot initializer samples X/Y through Generated402. Because live activation later supplies coordinates/path state, this appears to be harmless startup residue, but that reachability interpretation is kept separate from the raw recovered fact.

### Generated path families

The two non-FLY families are built immediately before the group templates:

- `0x004462F0/+4/+8`: allocated for 410 X/Y/AUX entries and generated until X passes 321; group 14 uses inclusive end 402.
- `0x004462E0/+4/+8`: allocated for 430 X/Y/AUX entries and generated in the opposite X direction until X falls below -20; group 15 uses inclusive end 422.

Each generator writes `0xFF` (`-1`) into the AUX slot for the **just-advanced** path index, matching one-frame reverse animation under the normal FLY/AUX contract. The allocation's index-0 AUX byte is not proven initialized by this loop, so clean documentation does not fabricate a value for it. The clean names intentionally encode terminal-index identity rather than guessing original developer terminology.

## Clean implementation

Evidence-backed helpers live in:

- `include/drone/gameplay/trajectory.hpp`
- `src/gameplay/trajectory.cpp`
- `include/drone/gameplay/trajectory_encounter.hpp`
- `src/gameplay/trajectory_encounter.cpp`
- `tests/test_gameplay.cpp`
- `tests/test_trajectory_encounter.cpp`

The implementation covers path-index arithmetic, AUX frame control, the complete 17-template startup catalog, stagger activation, mode-2 retirement, zero-active teardown, breakaway eligibility, 16.16 breakaway movement, off-screen bounds, breakaway frame direction, owned mutable actor/group state, the common live transient-wave activation contract, proven-hit damage/destruction/score/group teardown, and native rapid-missile/current-frame-mask plus direct-special/Stinger-display collision production. `GameSession` owns this encounter state continuously when immutable path samples are supplied.

## Whole-session integration and remaining boundaries

Phase 4 now owns all 17 groups inside `GameSession`. The established common live producer can activate an already-selected inactive non-primary template as mode 2, reset its fixed slots, activate slot 0 immediately, and then rely on the normal stagger updater for the remaining members. Proven collision hits can be dispatched through the original byte-damage threshold, destruction bursts, signed score award and zero-active group teardown. Mode-2 path completion applies the established negative score event.

Still intentionally separate:

- immutable FLY/generated path samples and extracted current-frame mask pixels remain asset inputs; live template selection and collision production are now native;
- immutable extracted current-frame mask pixels, which remain asset data while the rapid-missile opaque-pixel producer itself is now native;
- remaining dynamic special-family substitutions/producers beyond the fixed startup templates;
- deterministic original-runtime trace comparison, which remains Phase 6.

## Static combat metadata

The startup regions for all 17 fixed templates also initialize common-entity combat tail fields. `TrajectoryGroupTemplate::combat` and `manifests/trajectory_group_templates.csv` now preserve the exact recovered `destruction_threshold` (`+0x31`), `destruction_burst_count` (`+0x14F`), and signed `score_value` (`+0x150`). Values are template data, not universal defaults; notably groups 4/9 use `25,5,25` and groups 12/13 use `25,6,25`. See [`ENTITY_LAYOUT.md`](ENTITY_LAYOUT.md).
