# Demo DAT format

**Physical format confidence:** high.  
**Channel semantics confidence:** high for all fourteen channels at the consumer level; provenance of trajectory channels 7–9 remains partially open.

Drone demo files are headerless ASCII integer streams. The canonical Win32 recorder writes one signed decimal integer followed by `\n` for each channel. A replay is exactly **2,101 frames**, each containing **14 integers**.

The same physical format is used by the DOS and Windows releases. Four demo files present in both supplied shareware sets are byte-for-byte identical; see [`../reverse/DEMO_REPLAY.md`](../reverse/DEMO_REPLAY.md) and [`../../manifests/demo_replays.csv`](../../manifests/demo_replays.csv).

## Canonical Win32 routines

| address | role |
|---|---|
| `0x0041A460` | save 2,101 replay frames to `demo.dat` using text mode `wt` |
| `0x0041A5F0` | load `data\\<name>` using text mode `rt` |

The writer emits each value through format string `%d\n`. The loader scans each value with `%d`.

## Channel table

| field | Win32 storage | established meaning |
|---:|---|---|
| 1 | signed byte array `0x00454B18` | Left Arrow |
| 2 | signed byte array `0x00468E50` | Right Arrow |
| 3 | signed byte array `0x00461990` | Up Arrow — launch loaded Probe/Stinger |
| 4 | signed byte array `0x004D8518` | Down Arrow — load/cycle Probe/Stinger |
| 5 | signed byte array `0x00433BA8` | Space — shield |
| 6 | signed byte array `0x004324E0` | Ctrl — rapid missile |
| 7 | signed byte array `0x0043E588` | trajectory-group pool slot; `99` is canonical no-event sentinel |
| 8 | signed 16-bit array `0x00462A48` | trajectory-group X offset copied to group `+0x08` |
| 9 | signed byte array `0x0047EC40` | trajectory/path family selector `0..3`; `99` is canonical no-explicit-selector sentinel |
| 10 | signed byte array `0x00467DE8` | enemy bomb spawned this frame |
| 11 | signed 16-bit array `0x004D6438` | spawned bomb X |
| 12 | signed 16-bit array `0x004D4360` | spawned bomb Y |
| 13 | signed 16-bit array `0x0046C0B8` | Drone entity X |
| 14 | signed 16-bit array `0x00469FE8` | Drone entity Y |

Fields 1–6 are **not** a complete capture of every live keyboard action. In particular, the Win32 executable's `A`/`Z` vertical player movement is not represented in the fourteen replay channels.

## Original narrowing behavior

The loader calls its `%d`/`fscanf`-like routine with pointers directly into the byte and word channel arrays. The parsed integer write is wider than those eventual gameplay reads; gameplay later observes only the low 8 or 16 bits and sign-extends them when the replay is saved again.

The clean semantic decoder therefore preserves the raw `int32` textual values and separately reproduces the original low-byte/low-word signed narrowing. It intentionally does not reject a text value merely because it exceeds the nominal channel width.

## Trajectory sentinels

Playback compares field 7 and field 9 against decimal `99` (`0x63`). Canonical files use `99` when no trajectory-group event or explicit path-family selector is consumed for that frame.

For an active field-7 event, playback selects a fixed-stride trajectory-group slot from the pool rooted at `0x00495CF0`. Field 9 can replace the selected group's trajectory pointer with one of four families:

| value | pointer-table root |
|---:|---:|
| 0 | `0x00454AF0` |
| 1 | `0x00466C78` |
| 2 | `0x004417C0` |
| 3 | `0x00464B18` |

Field 8 is then copied verbatim as signed 16-bit data to trajectory-group `+0x08`, already independently established as the group's X offset.

## Bomb checkpoint

When field 10 is nonzero, demo playback activates a free common entity from the pool rooted at `0x004651A0`, populated from `bomb.jba`, and restores fields 11/12 into its X/Y position. The canonical asset initialization gives these entities 1×9 geometry and three shared sprite frames.

Live recording code writes the same event flag and the resulting bomb coordinates into fields 10–12, making this a deterministic checkpoint for an otherwise stochastic enemy projectile spawn.

## Drone checkpoint

During demo playback, fields 13 and 14 are copied into the canonical Drone entity (`0x00446080`) position. During recording-enabled live execution, the current Drone position is copied back into those channels.

## Clean API

`load_demo_dat()` retains raw 14-integer records. `decode_demo_record()` and `load_demo_frames()` expose the established semantic view while preserving the raw record. The gameplay adapter in `gameplay/demo_replay.*` converts those fields into clean input and deterministic-event checkpoints.

No original replay payload is stored in the repository. [`../../manifests/demo_replays.csv`](../../manifests/demo_replays.csv) contains only hashes and aggregate metadata.
