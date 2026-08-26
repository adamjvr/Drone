# Project Status

**Repository:** `Drone`  
**Current engineering milestone:** Phase 1 complete / Phase 2 entry  
**Primary decompilation reference:** canonical 1999 Win32 shareware executable (`BIN-WIN-SW-01`)  
**Independent cross-check:** canonical 1997 DOS shareware executable (`BIN-DOS-SW-01`)

## Status legend

- **CONFIRMED** — directly established by binary evidence, independent correspondence, or exact reproduction.
- **PARTIAL** — structure/control path is known but semantics remain incomplete.
- **OPEN** — not yet established.

## Evidence/tooling

| area | status | current result |
|---|---|---|
| source package identity | CONFIRMED | SHA-256-pinned DOS and Windows shareware packages |
| Windows installer extraction | CONFIRMED | 207 raw-DEFLATE streams recovered; 192 installed files; CRC checked |
| DOS executable identity | CONFIRMED | Watcom C/C++, 32-bit x86 LE/DOS4GW |
| Windows executable identity | CONFIRMED | PE32/i386 native Win32; DirectDraw/DirectInput/DirectSound/WinMM |
| clean reference bootstrap | CONFIRMED | known evidence tree reproducibly rebuilt without executing installer |

## Data formats

| format | status | current result |
|---|---|---|
| full-screen JBA | CONFIRMED | 768-byte RGB6 palette + 64,000 pixels; 10-lane interleave; 320×200 |
| small Windows JBA family | PARTIAL | embedded 128×128 8-bit PCX observed in at least three files; container semantics open |
| CLV | CONFIRMED | 22,050 Hz unsigned 8-bit interleaved stereo PCM |
| DOS CLV → Win WAV relationship | CONFIRMED for compared common regions | integer floor-average stereo→mono relationship |
| FLY | PARTIAL | count + triples stored as int16/int16/int8; meanings open |
| demo DAT | PARTIAL | 14 signed ASCII integers per record; meanings open |

## Executable reconstruction

| area | status | current result |
|---|---|---|
| Win32 startup/message/render loop | CONFIRMED at top level | main routine `0x00404E30`; DirectX init `0x00404B60`; game update `0x0040BA50` |
| Win32 state dispatcher | PARTIAL | direct table for raw states 0..5; additional transition/sentinel values observed |
| Win32 QPC limiter | CONFIRMED mechanically | waits for low-32-bit delta >= 15,000 when enabled |
| intended simulation rate | OPEN | must be established from DOS timer/runtime evidence; no FPS asserted |
| software renderer/blitters | OPEN | framebuffer pointer/pitch globals have candidates; routines not fully mapped |
| input aggregation | PARTIAL | candidate poll routine identified; semantic mapping incomplete |
| entity pools/structures | PARTIAL | 0x14-byte stride observed in cleanup context; fields/role not established |
| collision/projectiles/enemies | OPEN | next-phase targets |
| level scripting/scrolling | OPEN | next-phase target |

## Clean implementation

| area | status | current result |
|---|---|---|
| C++20 core library | CONFIRMED | builds and tests independently of original data |
| JBA decoder | CONFIRMED | synthetic regression coverage |
| CLV decoder/downmix | CONFIRMED | synthetic regression coverage |
| FLY parser | CONFIRMED structurally | semantics intentionally unnamed |
| demo DAT parser | CONFIRMED structurally | semantics intentionally unnamed |
| native inspection tool | CONFIRMED | exports/info paths for known formats |
| modern interactive host | OPEN | Phase 2 target |
| simulation reconstruction | OPEN | begins after timing/entity/control contracts are recovered |

## Phase 2 immediate objectives

1. Recover the DOS timing source and determine the intended fixed simulation cadence.
2. Partition the Win32 state-2 gameplay region into named behavioral subsystems.
3. Build entity/structure offset tables from all producers and consumers.
4. Recover player input/movement/fire/shield/special-weapon paths.
5. Recover framebuffer/blitter/HUD paths.
6. Establish FLY semantics through code consumers and runtime correlation.
7. Locate demo playback/recording and assign the 14 fields only when proven.
8. Build the first interactive modern fidelity host using the 320×200 indexed reference framebuffer.

## Known project constraint

The supplied evidence set is shareware. Full seven-level parity will eventually require a lawful full-game reference copy. Missing retail content must not be inferred from filenames and then presented as recovered fact.
