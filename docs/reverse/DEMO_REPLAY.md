# Demo replay reconstruction

This document describes the executable-backed replay system recovered during Phase 2. It complements the physical format specification in [`../formats/DEMO_DAT.md`](../formats/DEMO_DAT.md).

## Result

Drone's demos are **hybrid deterministic replays**. They are neither simple keyboard macros nor complete serialized game states.

The 14 channels combine:

1. selected player controls;
2. pre-scripted trajectory-group events;
3. deterministic enemy-bomb spawn checkpoints; and
4. per-frame Drone position checkpoints.

This design lets the original game replay scenes that contain PRNG-dependent behavior without recording every entity in the world.

## Playback and recording modes

### Demo playback flag — `0x00440594`

`0x00440594 == 1` is established as demo playback mode.

Evidence:

- the attract/demo selector calls `0x0041A5F0` to load a `DemoA*`/`DemoB*` recording;
- the same transition sets `0x00440594` to `1` before entering gameplay state 2;
- normal gameplay initialization clears it;
- state-2 input paths substitute replay channels for live keys when it is set;
- live-only random paths are suppressed;
- Probe decode timers use fixed replay values `210` and `150` instead of live randomized/difficulty-scaled thresholds.

### Recording-enabled flag — `0x00466B00`

When nonzero, `0x00466B00` enables writes into the replay channels during live state-2 execution. Established writers include:

- fields 1–6 when corresponding control actions are accepted;
- fields 10–12 when an enemy bomb is spawned;
- fields 13–14 from the Drone entity position.

The replay frame index advances when either playback mode or recording-enabled mode is active. When the runtime terminal threshold is reached, a recording-enabled session calls `0x0041A460` to serialize `demo.dat` and clears `0x00466B00`.

The user-facing or hidden setter that begins recording has not yet been located; that does not change the established function of the flag once nonzero.

### Replay frame index — `0x0047EBD4`

This 32-bit index selects the current element of all fourteen parallel channel arrays. It is reset to `0` during gameplay initialization and **pre-incremented once per active gameplay update** while playback or recording is active. Channel consumers therefore use the post-increment value.

The runtime terminal comparison is `index >= 0x82F` (**2095**). This same reset/increment/cutoff protocol is independently present in the DOS build at data offset `0x4CE8C`.

The physical canonical DAT files contain **2,101 records** (`0..2100`). Physical record count and runtime terminal index are therefore deliberately recorded as separate facts; we do not infer that all 2,101 records correspond to completed active gameplay updates.

## Channel consumers

### Controls 1–6

Playback substitutes these channels for live Win32 key queries:

| channel | action | live key |
|---:|---|---|
| 1 | player left | Left Arrow (`VK_LEFT`, `0x25`) |
| 2 | player right | Right Arrow (`VK_RIGHT`, `0x27`) |
| 3 | launch loaded special projectile | Up Arrow (`VK_UP`, `0x26`) |
| 4 | load/cycle Probe or Stinger | Down Arrow (`VK_DOWN`, `0x28`) |
| 5 | shield | Space (`0x20`) |
| 6 | rapid missile | Ctrl (`0x11`) |

The live `A`/`Z` player-Y controls are not present in the replay channel set. More strongly, Win32 consumes those vertical controls outside the replay-mode selection blocks, so live vertical movement remains possible during demo playback. The clean input aggregator therefore replaces only channels 1–6 and preserves live vertical/meta controls. See [`INPUT.md`](INPUT.md).

### Trajectory script 7–9

Field 7 is consumed as a trajectory-group slot. The selected object comes from the fixed-stride group pool rooted at `0x00495CF0`; the index expression has a stride of `0x2148` bytes. Canonical supplied demos use slots 1–6 and sentinel 99.

Field 9 chooses one of four trajectory pointer families when below 99. Field 8 is copied to group offset `+0x08`, already established independently as signed X group offset.

A notable archaeological result is that the canonical Win32 shareware executable contains **no gameplay writer** to the field-7/8/9 arrays. Their references are limited to playback, save, and load (plus an adjacent-address comparison in unrelated audio-pool setup). That means these trajectory channels were pre-scripted, retained from a preloaded source, or authored by another development path/tool. We do not claim which until additional evidence appears.

### Enemy bomb checkpoint 10–12

`0x004651A0` is a common-entity pool initialized as 1×9 sprites. `bomb.jba` is loaded and `0x00401860` extracts three frames into the first entity; frame pointers are propagated to the rest of the pool.

During playback:

1. field 10 nonzero requests a bomb event;
2. the game finds an inactive bomb-pool entity;
3. it activates that entity;
4. field 11 becomes entity X;
5. field 12 becomes entity Y.

During recording-enabled live play, a successful bomb spawn writes `1`, X and Y back into those three channels.

A newly recovered detail is deliberately asymmetric: the live spawn path writes `rand() % 3` to common-entity field `+0x10`, which the active bomb update uses as horizontal steering magnitude. Demo playback restores the recorded X/Y and then explicitly writes `+0x10 = 0`. Replayed bombs therefore fall vertically even though live bombs can steer by 0, 1, or 2 pixels horizontally per update. This original quirk is preserved by the clean replay-specific spawn helper. See [`ENEMY_BOMBS.md`](ENEMY_BOMBS.md).

This is a concrete example of the replay system recording selected stochastic outcomes rather than trying to reproduce every random call from input alone.

### Drone checkpoint 13–14

The canonical Drone target entity is rooted at `0x00446080` and populated from `drone.jba`.

- Playback reads field 13 into Drone X and field 14 into Drone Y.
- Recording-enabled live play writes current Drone X/Y into fields 13/14.

These channels give us per-frame reference coordinates for one of the game's central moving targets.

## DOS ↔ Windows evidence

The DOS shareware set contains four demo recordings also present in the Windows set:

- `DEMOA2.DAT`;
- `DEMOA4.DAT`;
- `DEMOB1.DAT`;
- `DEMOB3.DAT`.

All four files have **identical SHA-256 hashes across the DOS and Windows distributions**. This is unusually strong evidence that the format and intended replay behavior bridge both original implementations.

The executable protocol now cross-checks that content evidence. DOS uses `0x4CE88` as playback mode, `0x4CE84` as recording-enabled mode and `0x4CE8C` as the replay index. At `0x00077C34..0x00077C46` it increments the index once when either mode is active, and `0x000782E0` compares the index against the same `0x82F` terminal value used by Win32.

The public metadata is reproduced by:

```bash
python3 scripts/analyze_demo_assets.py \
  .reference/work/windows/Data \
  --dos-root .reference/work/dos \
  --output manifests/demo_replays.csv
```

## Clean reconstruction

The public core deliberately separates raw file archaeology from gameplay semantics:

```text
ASCII replay
    |
    v
DemoRecord[14]              raw lossless textual integers
    |
    v
DemoFrame                   original byte/word narrowing + named channels
    |
    v
DemoGameplayFrame
    |-- horizontal player input
    |-- special/shield/missile input
    |-- trajectory checkpoint
    |-- bomb checkpoint
    `-- Drone position checkpoint
```

Relevant files:

- `include/drone/formats/demo.hpp`;
- `src/formats/demo.cpp`;
- `include/drone/gameplay/demo_replay.hpp`;
- `src/gameplay/demo_replay.cpp`;
- `manifests/demo_replays.csv`.

## Validation value

These recordings can become deterministic parity fixtures incrementally. We do **not** need the complete engine before they become useful.

Near-term checks can compare:

- reconstructed accepted control actions;
- trajectory-group event selection and X offsets;
- bomb spawn frames/positions;
- Drone X/Y checkpoints.

Later, once the whole frame simulation is reconstructed, the same replay corpus can drive state snapshots and framebuffer comparisons against the original executables. The clean `DemoReplayTimeline` models the established executable clock (zero reset, pre-increment, terminal index 2095) without inventing a fixed Hz.

## Remaining questions

- What original developer workflow authored channels 7–9?
- What path sets `0x00466B00` nonzero to begin recording in the shipped Win32 executable, if reachable?
- Which replay events, beyond the currently recovered checkpoints, still depend on matching CRT PRNG state exactly?
- Why does the Win32 replay omit the live `A`/`Z` player-Y controls—were demos authored without vertical movement, or is another state path involved?
