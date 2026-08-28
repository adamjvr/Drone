# FLY trajectory format

**Confidence:** dual encoding, X/Y semantics, path progression, and normal-runtime AUX animation semantics confirmed.

## Correction to the Phase 1 interpretation

The FLY family is not one universally counted file format.

### `CURRENT.FLY` — counted working/composite form

`CURRENT.FLY` is CRLF ASCII and contains:

```text
record_count
x0
y0
aux0
x1
y1
aux1
...
```

The canonical file declares **443** records and contains exactly 443 triples.

### Gameplay trajectory assets — raw form

Files such as `Loop.fly`, `Leftdive.fly`, and `Swarm.fly` contain only flat ASCII triples:

```text
x0 y0 aux0
x1 y1 aux1
...
```

Whitespace is not semantically significant. There is no record count in these files. The original Win32 loader allocates three arrays and uses a hard-coded record count at each load site.

Recovered runtime storage is:

```cpp
struct FlyRecord {
    int16_t x;
    int16_t y;
    int8_t  aux;
};
```

This C++ structure describes semantic values, not necessarily an interleaved runtime structure: the original Win32 loader stores the three fields in separate arrays.

## Why field 0 and field 1 are now X/Y

Trajectory-group consumers dereference the first two array pointers and compute entity positions directly:

```text
entity.x = x_array[entity.path_index] + entity.x_offset + group.x_offset
entity.y = y_array[entity.path_index] + entity.y_offset + group.y_offset
```

The data ranges independently agree with screen coordinates: paths commonly enter from negative Y, beyond X=319, or other off-screen positions before moving through the 320×200 playfield.

This promotes field 0 → `x` and field 1 → `y` to high confidence. Phase 2 now also traces the third array through Win32 `0x00415FA0`: for ordinary trajectory-owned entities it is sprite-frame control.

## AUX sprite-frame semantics

The normal trajectory updater reads `aux_array[entity.path_index]` only on the animation/update phase (`mode == 2`) and applies it to entity byte `+0x140` (`current_frame`):

```text
if aux > 1:
    current_frame = aux - 2
else:
    current_frame += aux       // byte-wide signed delta

if current_frame >= frame_count:
    current_frame = 0
if current_frame < 0:
    current_frame = frame_count - 1
```

Thus `-1`, `0`, and `+1` mean previous/hold/next frame, while values `2..` directly select frames `0..`. This explains families such as `Loop.fly` (`0/1`), `Newcurly.fly` (`2..33` → frames `0..31`), `Swarm.fly` (`2..17` → frames `0..15`), and `Swoop.fly` (`2` → frame 0). Out-of-range positive selectors collapse to frame 0 through the original bounds check; negative relative results wrap to the final loaded frame.

`Frisbee1.fly` and `Frisbee2.fly` remain a loader-specific exception: their loaders scan the on-disk third token but write a constant into the runtime AUX array, so their file token values must not automatically be treated as the live animation stream.

## Path progression semantics

The same updater resolves the trajectory-owned meanings of common-entity offsets `+0x32/+0x36/+0x38`:

```text
path_index += path_step        // 16-bit add
if path_index > path_end_index // signed comparison
    path_index = 0
```

The resulting `path_index` selects all three parallel X/Y/AUX arrays. These names are **contextual**: other object families reuse the common entity's scratch/control offsets for unrelated state, so this does not claim universal meanings for every `0x154`-byte entity.

## Canonical Win32 loader counts

The publishable metadata is machine-readable in [`../../manifests/fly_trajectories.csv`](../../manifests/fly_trajectories.csv).

| file | physical records | Win32 loader records | notes |
|---|---:|---:|---|
| `Current.fly` | 443 | 443 | counted form |
| `Frisbee1.fly` | 937 | 937 | raw; loader scans AUX but writes a constant into its third array |
| `Frisbee2.fly` | 426 | 426 | raw; loader scans AUX but writes a constant into its third array |
| `Leftdive.fly` | 119 | 119 | raw |
| `Leftdrop.fly` | 200 | 200 | raw; unusual large tail values are present in canonical data |
| `Loop.fly` | 380 | 380 | raw; unusual tail values present |
| `New.fly` | 499 | not yet established | raw; appears distinct from the normal known loader sites |
| `Newcurly.fly` | 232 | 232 | raw |
| `Rightdiv.fly` | 118 | **119** | canonical file is one triple shorter than loader loop |
| `Ritedrop.fly` | 200 | 200 | raw; unusual tail values present |
| `Swarm.fly` | 949 | **950** | canonical file is one triple shorter than loader loop |
| `Swoop.fly` | 190 | 190 | raw |

The short-file mismatches are recorded as original-data/original-loader quirks, but their gameplay reachability is now resolved for the canonical group assignments. `Rightdiv.fly` is used by replay trajectory slots 1/2, whose entities set `path_end_index = 117`; the physical file supplies exactly indices `0..117`, so the loader's requested index 118 is never consumed. The two static Swarm groups set `path_end_index = 945`; physical indices `946..948` and the nonexistent loader slot 949 are outside the normal path cycle. Thus the unchecked reads are loader quirks rather than required gameplay samples in the recovered canonical paths.

The clean parser still never invents a final record; callers can choose strict validation or intentionally read the physically present records for research.

## Clean API

- `load_counted_fly(path)` — counted `CURRENT.FLY` form;
- `load_raw_fly(path, expected_records, strict)` — raw runtime trajectory form;
- `known_fly_asset(filename)` — recovered loader-count metadata for canonical known paths;
- legacy `load_fly(path)` remains an alias for the counted form so old Phase 1 callers fail loudly instead of silently mis-parsing raw trajectories.

## Open questions

- whether every special trajectory family reaches the normal AUX consumer;
- purpose of `New.fly` and its relationship to `CURRENT.FLY`/the original path-authoring tooling;
- whether the extreme tail values in several files are intentionally unreachable sentinels, stale editor data, or consumed behavior.


## Trajectory-group lifecycle relationship

`update_trajectory_groups` (`0x00415FA0`) now establishes the surrounding runtime lifecycle as well as individual FLY sample consumption. The canonical Win32 pool contains 17 fixed `0x2148`-byte groups. Mode 1 loops FLY paths persistently; mode 2 retires an already-following entity when its path index wraps; activity state 3 acquires the current path target before becoming a normal follower; mode 10 abandons FLY samples for accelerated 16.16 breakaway motion.

The canonical `Rightdiv.fly` and `Swarm.fly` one-record loader over-reads are not observable in recovered canonical path cycles: Rightdiv groups cap at physical index 117 and Swarm groups cap at 945. Clean code records the original mismatch without fabricating an unreachable sample.

See [`../reverse/TRAJECTORY_GROUPS.md`](../reverse/TRAJECTORY_GROUPS.md) for the complete recovered group lifecycle.
