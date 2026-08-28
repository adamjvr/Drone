# Drone

Reverse engineering, annotated decompilation, clean reimplementation, and eventual remaster of **DRONE** by Pixelsplash Software (DOS 1997 / Windows 1999).

## Target platforms

- Linux
- macOS
- iPadOS
- Windows

The long-term goal is a documented, behaviorally validated reconstruction of the original engine plus a modern portable remaster. Original game binaries and proprietary assets are **not** stored in this repository. Checked-in tools reconstruct a local reference workspace from user-supplied copies.

## Current status

Phase 3 now includes an evidence-backed 28-subpass Win32 world/effect catalog beneath the corrected 19-pass outer renderer contract, plus scaled explosion/objective-debris routing, startup palette fade, and complete late HUD/outcome-cursor presentation.

**Phase 4 — Complete Game Simulation is in progress.** Phase 3 is complete. `GameSession` now separates campaign and encounter ownership, preserves full-vs-encounter reset semantics, continuously ticks the recovered player/projectile/shield/bomb/cadence/scroll helpers, and owns all 17 trajectory groups plus the normal Drone objective path. Drone X/Y travel, Y=45 hold, completed-disarm departure, Y=201 commit, Y=230 settlement reset, automatic Y=-200 shareware boss dispatch, count-1 Gemini continuation, count-2 Results/EndRun transition, and the recovered Lid/Top/Gemini lifecycle/score tails now execute through the whole-session boundary. Template-selection randomness, exact Probe decode production, destructive Drone detonation, opaque-pixel hit detection, remaining non-trajectory encounters, boss movement/attacks and death/post-game continuity remain current Phase-4 work.

Phase-2 closure is enforced by `scripts/check_phase2_exit.py`; it fails if a critical simulation-architecture question is reopened or the roadmap regresses. See [`docs/PHASE2.md`](docs/PHASE2.md), [`docs/PHASE3.md`](docs/PHASE3.md), [`docs/STATUS.md`](docs/STATUS.md), and [`docs/ROADMAP.md`](docs/ROADMAP.md).

### High-confidence Phase 1 results

| Area | Result |
|---|---|
| full-screen `.JBA` | 768-byte RGB6 palette + 64,000 indexed pixels; 320×200; 10-lane stored pixel order |
| small Windows `.JBA` | byte-sized opaque-preamble length + 128×128 8-bit PCX RLE + markerless raw 256×RGB8 palette; separate physical family |
| DOS `.CLV` | raw unsigned 8-bit stereo PCM at 22,050 Hz |
| DOS→Windows audio relation | corresponding Win mono sample is integer floor-average of DOS stereo sample pair over compared common regions |
| `.FLY` | `CURRENT.FLY` counted; runtime paths are raw `int16 x,int16 y,int8 aux` triples with hard-coded loader counts; X/Y semantics established |
| demo `.DAT` | 2,101×14 ASCII integers; controls, trajectory-script data, bomb checkpoints, and Drone X/Y channels mapped; four DOS/Windows files byte-identical |
| Windows installer | known Wise payload recovered as 207 raw-DEFLATE streams; 192 installed files; CRC-32 validated |
| Win32 loop | startup/surface/message loop rooted at `0x00404E30`; game update rooted at `0x0040BA50` |
| timing | canonical DOS fidelity cadence is established at ~70.0863 Hz from mode 13h + one ordinary retrace wait per logical update; Win32 QPC limiter frequency remains a separate host-history question |

## Documentation is a project deliverable

Stable research must not live only in chat history or a local Ghidra database. Start with:

- [`docs/README.md`](docs/README.md) — documentation index;
- [`docs/RE_HANDBOOK.md`](docs/RE_HANDBOOK.md) — evidence/naming/clean-reimplementation rules;
- [`docs/PROVENANCE.md`](docs/PROVENANCE.md) — exact evidence identities and hashes;
- [`docs/TESTING_VALIDATION.md`](docs/TESTING_VALIDATION.md) — parity strategy;
- [`docs/reverse/GHIDRA_WORKFLOW.md`](docs/reverse/GHIDRA_WORKFLOW.md) — repeatable local binary-analysis workflow.

Machine-readable research state lives under `reverse/ledger/`, `reverse/correspondence/`, and `reverse/structures/`.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The library and synthetic tests require no original game data.

## Reconstruct the local reference installation

Copy the two canonical source packages to:

```text
.reference/input/drone_sw.zip
.reference/input/drone_sw(win).zip
```

Then:

```bash
python3 scripts/bootstrap_reference.py .reference/input .reference/work
python3 scripts/verify_reference.py .reference/work
```

The source package hashes are checked before extraction. The Windows installer is decompressed directly and is never launched.

Full known corpus metadata is publishable in:

- `manifests/dos_shareware_files.csv` — 187 files;
- `manifests/windows_shareware_files.csv` — 192 files;
- `manifests/fly_trajectories.csv` — FLY hashes/counts/ranges and recovered loader-count metadata.

## Native fidelity host

On supported desktop hosts, the project now builds `drone_fidelity_host`, which presents an original full-screen JBA through the clean 320×200 indexed framebuffer contract:

```bash
./build/drone_fidelity_host .reference/work/windows/Sights/Titlesh.jba 3
```

Linux uses X11, Windows uses Win32/GDI, and macOS uses Cocoa/CoreGraphics. The Linux host additionally accepts `DRONEFB1` input and supports `--headless` capture for shell/CI validation; see [`docs/LINUX_FIDELITY_HOST.md`](docs/LINUX_FIDELITY_HOST.md). This host is a presentation shell, not yet the reconstructed game simulation.

## Inspect recovered formats

```bash
./build/drone_inspect jba-info .reference/work/windows/Sights/Titlesh.jba /tmp/drone-title.ppm
./build/drone_inspect clv-info .reference/work/dos/DRONE.CLV /tmp/drone.wav
./build/drone_inspect fly-info .reference/work/windows/Data/Current.fly
./build/drone_inspect fly-info .reference/work/windows/Data/Rightdiv.fly
./build/drone_inspect demo-info .reference/work/windows/Data/Demoa2.dat
```

## Repository layout

```text
include/, src/             clean portable implementation
tools/                     native project analysis utilities
scripts/                   extraction/inventory/reproducibility tools
reverse/windows/           Win32 address maps and control-flow notes
reverse/dos/               DOS address maps and notes
reverse/ghidra/            reproducible label/helper scripts
reverse/correspondence/    DOS↔Windows relationships
reverse/structures/        original structure/field evidence
reverse/ledger/            findings and unresolved-question tracking
docs/                      handbook, specifications, architecture, roadmap
manifests/                 hashes and non-payload corpus metadata
.reference/                local original evidence; ignored by Git
```

## Rights boundary

The repository contains independently written project code, tools, specifications, and metadata—not original game payloads. No project source-code license has been selected yet; see [`docs/LICENSE_AND_RIGHTS.md`](docs/LICENSE_AND_RIGHTS.md) before public release/contribution setup.

- Phase 3 framebuffer parity tooling: see `docs/FRAMEBUFFER_VALIDATION.md`.
- Phase 3 gameplay HUD reconstruction: see `docs/reverse/HUD_PRESENTATION.md`.
- Phase 3 scaled-overlay reconstruction: see `docs/reverse/SCALED_OVERLAYS.md`.
- Phase 4 integration plan: see `docs/PHASE4.md`.
- Continuous session ownership and deterministic state oracle: see `docs/GAME_SESSION.md`.
