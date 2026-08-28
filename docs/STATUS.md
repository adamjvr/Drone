# Project Status

**Repository:** `Drone`  
**Current engineering phase:** Phase 2 — Gameplay Reconstruction (in progress)  
**Primary decompilation reference:** canonical 1999 Win32 shareware executable (`BIN-WIN-SW-01`)  
**Independent cross-check:** canonical 1997 DOS shareware executable (`BIN-DOS-SW-01`)

## Status legend

- **CONFIRMED** — directly established by binary evidence, independent correspondence, or exact reproduction.
- **PARTIAL** — structure/control path is known but semantics remain incomplete.
- **OPEN** — not yet established.

## Phase 2 checkpoint

| area | status | current result |
|---|---|---|
| DOS LE extraction | CONFIRMED | reproducible script reconstructs load objects; canonical object 1 byte-for-byte matches independent extraction |
| DOS vertical retrace | CONFIRMED | `0x0006940C` polls VGA `0x3DA` bit `0x08` until retrace |
| Win32 vertical blank | CONFIRMED | `0x004018F0 -> 0x004061E0 -> IDirectDraw::WaitForVerticalBlank`; separate primitive from QPC pacing |
| Win32 Tab retrace-sync option | CONFIRMED | `VK_TAB` negates `0x0042B1B4` +1/-1; UI renders VERTICAL RETRACE / SYNC ON/OFF; +1 enables 15,000-count QPC wait |
| canonical DOS fidelity cadence | CONFIRMED | BIOS mode 13h + shared replay/update boundary + one ordinary sync-tail retrace wait establishes ~70.0863 Hz; Win32 15,000-QPC-count wall-clock cadence remains a separate historical-port question |
| Win32 gameplay orchestration | PARTIAL / strong | state-2 four-phase substep `0→1→2→3→0` plus explicit simulation→presentation boundary recovered; collision/destruction→debris updates→320×600 viewport composition→sprite/effect render→HUD→QPC→present is encoded as a clean stage-order contract; exceptional branches still need partitioning |
| Win32 game-state protocol | CONFIRMED user-facing set | raw values 0..8, 13 and 99 mapped across direct dispatcher, main-menu selections, modal handlers, pause/quit overlays and nine-lives notice; clean protocol module tested |
| Win32/DOS common sprite/entity | ESTABLISHED core layout | Win32 `0x154` and DOS `0x14F` field-level correspondence established across init/free/blit/collision/destruction; damage `+0x30`, threshold `+0x31`, destruction burst/score tail metadata, contextual overlays, and preserved unreferenced 128-byte block mapped |
| transparent sprite blit | CONFIRMED | `0x00401660`; clips to logical framebuffer and skips source index 0 |
| FLY encoding | CONFIRMED | `CURRENT.FLY` counted; gameplay trajectories raw triples with hard-coded loader counts |
| FLY X/Y semantics | CONFIRMED | first two arrays directly produce entity X/Y positions with offsets |
| FLY AUX semantics | CONFIRMED normal path | third byte controls sprite frame: signed relative delta for <=1, absolute frame aux-2 for >1 |
| trajectory groups | CONFIRMED lifecycle + fixed template catalog | all 17 startup templates cataloged with path families/counts/delays/geometry/frame banks/seeds; modes 0/1/2/10, stagger activation, path acquisition/retirement and randomized 16.16 breakaway recovered and clean-tested; dynamic substitutions remain |
| original framebuffer pipeline | CONFIRMED | guarded software 320×200 indexed framebuffer -> explicit pitched DirectDraw copy; 320×600 scenery viewport compositor/wrap and scaled transparent sprite path now established in addition to the ordinary blitter |
| collision primitives | CONFIRMED + clean implementation | point hitbox, opaque-pixel mask, Y+9 probe, and asymmetric centered-hitbox-vs-full-sprite broad phase reconstructed; synthetic tests pass |
| bitmap UI text | CONFIRMED cross-build | Win32 `0x00401470` and DOS `0x000809B0` share a 64×0x14 FONT2 descriptor cache; exact 16×4 / 7×5 mask geometry, ASCII-0x20 lookup, mask extraction/render semantics and clean safe extractor are established |
| explosion/effects cluster | CONFIRMED core debris lifecycle | mini/large/composite emitters and sound variants mapped; 0x18 particle groups plus secondary bank have motion/gravity/lifetime/teardown rules clean-tested; three 15-entry sprite-debris pools identified as junk1/junk2/wheel and their shared updater recovered |
| native fidelity framebuffer | CONFIRMED | clean 320×200 indexed buffer + palette/RGBA conversion |
| player reconstruction | CONFIRMED core slice | player root `0x00466B18`; 22x22/15-frame ship bank, Left/Right + A/Z motion, exact bounds and banking mapped; lives are deferred until death settlement; 3-life init, respawn and game-over transition are clean-tested |
| gameplay input aggregation | CONFIRMED semantic boundary | Win32 keyboard + normalized DirectInput joystick, DOS keyboard/game-port convergence, exact six-channel demo replacement, live vertical overlay and portable `GameplayInputFrame` are recovered and clean-tested |
| rapid-fire missiles | CONFIRMED core slice | Ctrl path; 8-slot 1x9 pool, 3 frames, cooldown/spawn/update/top cleanup, missile.wav voice pool and rendering chain mapped; clean module tested |
| player shield | CONFIRMED core slice | Space/demo channel 5; 75-unit high-word accumulator, +0x514 recharge, 0xBB80 drain, protection flag, shields.wav cadence and bomb-absorption path mapped; clean energy model tested |
| native fidelity host | CONFIRMED build | X11, Win32/GDI, Cocoa/CoreGraphics backends present; Linux backend builds in current validation environment |
| Probe/Stinger special weapon | CONFIRMED core lifecycle | shared 3x8 entity, load/cycle/launch/homing/Probe attachment mapped; state 4 hole interaction and state 10 impact-consumed terminal behavior now classified; downstream state-4 target and full disarm consequences partial |
| demo replay system | CONFIRMED core semantics | playback/recording flags and all 14 channels mapped; DOS/Win share zero-reset, one-preincrement-per-gameplay-update and 0x82F terminal clock; four shared demos byte-identical |
| six-Drone objective outcomes | CONFIRMED core semantics | six-entry ledger 0 unresolved / 1 disarmed / 2 detonated; normal Y=201 commit plus 60-slow-tick settlement gate recovered; `0x0041D220` detonation path, per-objective good/bad + mission interstitial, and final result art/music reductions clean-tested |
| enemy bomb gameplay slice | CONFIRMED core lifecycle | 10-slot 1x9 pool; live steering magnitude rand()%3, replay steering forced 0, Y+=2 update, 3-frame animation, visibility/lifetime, player-impact consequences, and shared spawn-gate/death quiet-period counter reconstructed |
| scoring / extra lives | CONFIRMED core semantics | total score and rolling extra-life progress separated; signed awards/penalties, one 500-point life conversion per update, Drone special penalty, negative floor and 9999 HUD quirk recovered; clean-tested |
| high scores / post-game results | CONFIRMED core semantics | lives<=0 results entry, six numeric result statistics, 58-present confirmation lock, Ordering Information handoff, 10-entry qualification/insertion, slot-0 no-save quirk, demo/L-cheat gates, final state1/state4 behavior and perfect-run credits are clean-tested |
| Lid/Top boss family | CONFIRMED core encounter slice | `lid.jba`/`top.jba` resource pair, 9+1 frames, init/update/release routines, bomb/weapon interaction, 25-count destruction progression and exact +100 boss reward established |
| world/scenery progression | CONFIRMED shareware + compiled transition semantics / PARTIAL retail parity | three stacked 320×200 scenery screens; cyclic phase-2 scroll; Drone Y=-200 gates boss dispatch; full-vs-encounter reset split recovered; objective 2 is canonical shareware stop; dormant branches 3–5 map isle/house/night and branch 6 proves zero detonations -> river/Mothership, otherwise results |

## Evidence/tooling baseline

| area | status | current result |
|---|---|---|
| source package identity | CONFIRMED | SHA-256-pinned DOS and Windows shareware packages |
| Windows installer extraction | CONFIRMED | 207 raw-DEFLATE streams; 192 installed files; CRC checked |
| DOS executable identity | CONFIRMED | Watcom C/C++, 32-bit x86 LE/DOS4GW |
| Windows executable identity | CONFIRMED | PE32/i386 native Win32; DirectDraw/DirectInput/DirectSound/WinMM |
| clean reference bootstrap | CONFIRMED | known evidence tree rebuilt without executing installer |

## Data formats

| format | status | current result |
|---|---|---|
| full-screen JBA | CONFIRMED | 768-byte RGB6 palette + 64,000 pixels; 10-lane interleave; 320×200 |
| small Windows JBA family | PARTIAL | embedded 128×128 8-bit PCX observed; container semantics open |
| CLV | CONFIRMED | 22,050 Hz unsigned 8-bit interleaved stereo PCM |
| FLY | CONFIRMED core runtime semantics | dual physical encoding, X/Y, trajectory index/step/wrap, normal AUX frame control and canonical short-file reachability solved; special-family substitutions/producers remain |
| demo DAT | CONFIRMED core semantics | 2,101 × 14 ASCII integers; all playback channel meanings mapped; trajectory-channel authoring provenance still open |
| Win32 `scores` | CONFIRMED physical encoding | 10 logical entries; name + four numeric fields hidden by per-value 300..699-byte random filler runs; safe clean decoder + deterministic compatible encoder |

## Clean implementation

| area | status | current result |
|---|---|---|
| C++20 core library | CONFIRMED | builds/tests without original data |
| JBA / CLV | CONFIRMED | synthetic regression coverage |
| FLY parser | CONFIRMED physical behavior | explicit counted/raw APIs; known loader counts; mismatch-safe behavior |
| demo DAT parser/replay adapter | CONFIRMED core semantics | raw records preserved; original byte/word narrowing reproduced; named control/trajectory/bomb/Drone checkpoints exposed |
| fidelity framebuffer | CONFIRMED | indexed 320×200 core-owned contract |
| native fidelity host | PARTIAL platform validation | Linux build validated; source backends included for Windows/macOS |
| simulation reconstruction | PARTIAL implementation | player, lifecycle, missiles, shield, Probe/Stinger, bombs, scoring/high scores, collision, semantic input/replay, trajectory groups/templates, four-phase scheduler, game-state protocol, world scroll, and objective/encounter mission progression now exist as independently written tested `drone_core` modules |

## Current blockers to completing Phase 2

1. The canonical DOS fidelity cadence and Win32 four-phase gameplay substep are established; the remaining timing architecture work is complete branch-aware whole-frame update/render partitioning and the separate nonblocking historical Win32 QPC wall-clock question.
3. The trajectory-group lifecycle, all 17 fixed startup templates, and canonical FLY path/AUX/short-file behavior are resolved; only dynamic special-family trajectory substitutions/producers and their whole-frame integration still need broader classification.
4. Full Probe disarm consequences, remaining Mothership/special-weapon details, the proper identity/resource loader for registered boss slot 2, enemy-specific Stinger consequences, bomb live-scheduler ownership, retail reachability/history for dormant world branches, remaining retail-only result presentation details still need reconstruction.
5. Typed demo replay checkpoints now exist, but deterministic whole-frame trace/framebuffer parity against the original runtime has not yet been established.

The supplied evidence set remains shareware. Full seven-level parity eventually requires a lawful full-game reference copy; missing retail behavior must not be invented.

- Boss dispatcher is now family-classified for all six slots: Lid/Top, Gemini, registered-slot-2 unknown, Spidey, Lid/Top reused, Bomber. Gemini resources are present in shareware; later registered boss resources are tracked as absent/unidentified rather than assumed.
