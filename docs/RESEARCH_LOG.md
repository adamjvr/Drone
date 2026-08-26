# Research Log

This file records major findings and engineering decisions chronologically. It is intentionally concise; detailed evidence belongs in the linked specifications and ledgers.

## Phase 0 — Evidence intake / format reconnaissance

- Accepted the supplied DOS and Windows shareware packages as the initial evidence set and pinned them by SHA-256.
- Identified DOS `DRONE_SW.EXE` as a 32-bit x86 Watcom C/C++ LE program using DOS/4GW.
- Identified Windows `Drone_sw.exe` as a compact native Win32 PE32/i386 game using DirectDraw, DirectInput, DirectSound, WinMM, and Microsoft C/C++ runtime-era conventions.
- Recovered the Windows Wise installer payload as a chain of raw-DEFLATE streams with per-stream CRC-32 values, allowing extraction without running the installer.
- Reconstructed 192 installed Windows files from the known installer.
- Solved full-screen JBA layout and pixel deinterleave. Independent DOS and Win32 loaders agree on the 10-lane algorithm.
- Established CLV as headerless 22,050 Hz unsigned 8-bit stereo PCM.
- Compared corresponding DOS CLV and Windows WAV material and established integer floor-average stereo→mono conversion across common sample regions examined.
- Established FLY physical storage as a count followed by `int16,int16,int8` triples; semantics intentionally left unresolved.
- Established demo DAT physical record width as 14 signed ASCII integers; semantics intentionally left unresolved.

## Phase 1 — Executable reconstruction / clean asset core

- Added reproducible reference bootstrap and hash verification.
- Added clean C++20 decoders/parsers and synthetic tests for JBA, CLV, FLY, and demo DAT.
- Added native `drone_inspect` utility.
- Mapped Win32 startup/message/surface loop at high level. `0x00404E30` initializes platform subsystems and calls `0x0040BA50` once per acquired render surface/update iteration.
- Identified the six-entry direct dispatcher rooted at `0x0040BA50` and global selector at `0x0042B188`; preserved the name `game_state_raw` because additional raw values are used outside the direct table.
- Identified the Win32 frame-limiter guard and QPC busy wait with threshold 15,000 counts. Explicitly declined to convert this to FPS without counter-frequency/DOS timing evidence.
- Established repository separation between original evidence (`.reference/`), RE findings (`reverse/`), and independently written clean code (`src/`, `include/`).

## Documentation hardening patch

- Promoted documentation to a required phase deliverable rather than supplemental notes.
- Added the master RE handbook, provenance record, testing/parity strategy, compatibility/remaster policy, platform plan, Ghidra workflow, subsystem/structure/correspondence documentation, open-question ledger, research log, and installer-format specification.
- Added machine-readable findings, correspondence, structure, and question ledgers to prevent discoveries from existing only in chat or local decompiler projects.
