# Trajectory Group Lifecycle

**Status:** Win32 trajectory-group lifecycle and all 17 fixed startup templates established; special runtime substitutions/whole-frame integration remain in progress.

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
- `tests/test_gameplay.cpp`

The implementation covers path-index arithmetic, AUX frame control, the complete 17-template startup catalog, stagger activation, mode-2 retirement, zero-active teardown, breakaway eligibility, 16.16 breakaway movement, off-screen bounds, and breakaway frame direction.

## Still open

Phase 2 still needs:

- remaining dynamic special-family substitutions/producers beyond the now-cataloged fixed startup templates;
- broader whole-frame validation tying recovered group transitions to original runtime traces;
- semantic names for unrelated counters touched by some spawn/activation branches where evidence is still insufficient.

## Static combat metadata

The startup regions for all 17 fixed templates also initialize common-entity combat tail fields. `TrajectoryGroupTemplate::combat` and `manifests/trajectory_group_templates.csv` now preserve the exact recovered `destruction_threshold` (`+0x31`), `destruction_burst_count` (`+0x14F`), and signed `score_value` (`+0x150`). Values are template data, not universal defaults; notably groups 4/9 use `25,5,25` and groups 12/13 use `25,6,25`. See [`ENTITY_LAYOUT.md`](ENTITY_LAYOUT.md).
