# Phase 1 — Executable Reconstruction and Native Asset Core

## Objective

Turn initial evidence discoveries into reproducible engineering infrastructure, begin mapping the original engine, and establish a clean portable codebase plus a durable research record suitable for a long-running public decompilation/remaster effort.

## Canonical evidence

Phase 1 addresses and findings are tied to the exact shareware evidence IDs/hashes in `docs/PROVENANCE.md`.

## Completed engineering work

1. **Reference bootstrap is reproducible.** The DOS ZIP is extracted normally; the known Windows Wise installer is decompressed without executing it. Source-package hashes are checked first and every recovered Wise stream is validated for uncompressed length and CRC-32.
2. **Complete corpus metadata is preserved.** File path/size/SHA-256 inventories for all 187 DOS and 192 reconstructed Windows installed files are checked in under `manifests/` without payload bytes.
3. **Portable format library exists.** Confirmed JBA, CLV, FLY, and demo physical structures are implemented in clean C++20.
4. **Synthetic regression suite exists.** Format tests use project-created fixtures and require no original game data.
5. **Native inspection tool exists.** `drone_inspect` can decode/report recovered formats against local reference files.
6. **Primary Win32 control loop is located.** `0x00404E30` handles window creation/platform initialization/message pumping/surface acquisition, calls `0x0040BA50`, and presents/releases the DirectDraw surface.
7. **Game dispatcher is located.** `0x0040BA50` directly dispatches global `0x0042B188` through a six-entry table for values 0..5. Other code writes values including 6, 7, 8, 13, and 99, so this remains `game_state_raw` rather than a prematurely closed enum.
8. **Timing limiter is mechanically established.** When `0x0042B1B4 == 1`, the gameplay path busy-waits on `QueryPerformanceCounter` until the low-32-bit delta from `0x004677E8` reaches `0x3A98` (15,000). Because the executable does not import `QueryPerformanceFrequency`, no simulation FPS is asserted yet.
9. **Full-screen JBA loader is source-level specified.** DOS and Windows independently establish palette loading plus the same 10-lane 64,000-pixel deinterleave.
10. **Audio storage/conversion is specified.** DOS CLV is 22.05 kHz unsigned 8-bit stereo; compared Windows WAV replacements use integer floor-average mono conversion over common sample regions.
11. **Documentation is now a phase deliverable.** The project includes a master RE handbook, provenance, Ghidra workflow, DOS↔Windows correspondence policy/table, subsystem/structure ledgers, validation strategy, platform/remaster policy, open questions, and research log.

## Validation performed

- C++20 project configured and built successfully in the development environment.
- `ctest` synthetic format suite passed.
- canonical source packages passed hash verification;
- known Windows installer reconstructed 192 installed files with stream CRC checking;
- `verify_reference.py` matched the canonical DOS and Windows game executable hashes;
- clean format code was exercised on the local original reference corpus during Phase 1 research.

## Known limitations / intentionally unresolved claims

- the Win32 15,000-QPC-count threshold is **not** yet an FPS value;
- the semantic meaning of FLY fields is unknown;
- the semantic meaning of 14 demo fields is unknown;
- state values beyond the direct 0..5 dispatcher remain unresolved protocol values;
- at the Phase 1 exit, the observed 0x14-byte stride did not justify naming an entity type; Phase 2 later resolved it as the cross-build `FONT2.JBA` glyph descriptor/cache rather than gameplay state;
- small Windows JBA/embedded-PCX container semantics remain partial;
- full-game levels/content are outside the supplied shareware evidence set.

## Phase 1 exit state

Phase 1 is complete. The repository can rebuild its known evidence workspace locally, builds/tests independently of original payloads, records the major binary/data findings reproducibly, and is ready to move from static reconstruction into timing/gameplay subsystem recovery and an interactive modern fidelity host.

## Phase 2 handoff priorities

1. DOS timer and intended simulation cadence.
2. Win32 state protocol and active-gameplay region decomposition.
3. Input aggregation/canonical action map.
4. Renderer/blitter/HUD contracts.
5. Entity pool and structure-offset recovery.
6. FLY and demo semantics from consumers.
7. First desktop fidelity host displaying the indexed framebuffer live.
