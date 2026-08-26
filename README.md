# Drone

Reverse engineering, annotated decompilation, clean reimplementation, and eventual remaster of **DRONE** by Pixelsplash Software (DOS 1997 / Windows 1999).

## Target platforms

- Linux
- macOS
- iPadOS
- Windows

The long-term goal is a documented, behaviorally validated reconstruction of the original engine plus a modern portable remaster. Original game binaries and proprietary assets are **not** stored in this repository. Checked-in tools reconstruct a local reference workspace from user-supplied copies.

## Current status

**Phase 1 is complete and the project is at Phase 2 entry.** The evidence/tooling layer is reproducible, several core formats are recovered, the Win32 outer loop/state dispatcher/timing limiter are mapped, and clean C++20 format code is tested. Phase 2 focuses on timing, gameplay subsystem/structure recovery, and the first interactive fidelity host.

See [`docs/STATUS.md`](docs/STATUS.md) for the exact confirmed/partial/open matrix and [`docs/ROADMAP.md`](docs/ROADMAP.md) for milestone exit criteria.

### High-confidence Phase 1 results

| Area | Result |
|---|---|
| full-screen `.JBA` | 768-byte RGB6 palette + 64,000 indexed pixels; 320×200; 10-lane stored pixel order |
| DOS `.CLV` | raw unsigned 8-bit stereo PCM at 22,050 Hz |
| DOS→Windows audio relation | corresponding Win mono sample is integer floor-average of DOS stereo sample pair over compared common regions |
| `.FLY` | count followed by records stored as `int16,int16,int8`; semantics unresolved |
| demo `.DAT` | flat ASCII stream of 14-signed-integer records; semantics unresolved |
| Windows installer | known Wise payload recovered as 207 raw-DEFLATE streams; 192 installed files; CRC-32 validated |
| Win32 loop | startup/surface/message loop rooted at `0x00404E30`; game update rooted at `0x0040BA50` |
| Win32 pacing | optional busy wait for QPC low-32 delta >= 15,000 counts; intended Hz deliberately unresolved |

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
- `manifests/windows_shareware_files.csv` — 192 files.

## Inspect recovered formats

```bash
./build/drone_inspect jba-info .reference/work/windows/Sights/Titlesh.jba /tmp/drone-title.ppm
./build/drone_inspect clv-info .reference/work/dos/DRONE.CLV /tmp/drone.wav
./build/drone_inspect fly-info .reference/work/windows/Data/Current.fly
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
