# Project Status

**Repository:** `Drone`  
**Current engineering phase:** Phase 4 — Complete Game Simulation  
**Primary decompilation reference:** canonical 1999 Win32 shareware executable (`BIN-WIN-SW-01`)  
**Independent cross-check:** canonical 1997 DOS shareware executable (`BIN-DOS-SW-01`)

## Phase 4 current integration

| area | status | current result |
|---|---|---|
| continuous `GameSession` ownership | ESTABLISHED first contract | clean campaign-vs-encounter state split; full/encounter reset; active-state-only deterministic tick; player, rapid missile, special weapon, shield, enemy-bomb movement plus ordered late special/player collisions, deferred player respawn settlement, four-phase cadence, world scroll and one-extra-life conversion integrated |
| whole-session regression oracle | CONFIRMED clean oracle | asset-free `drone_session_probe` executes a fixed 120-update semantic input script and is checked exactly by CTest |
| encounter actor integration | CURRENT | all 17 trajectory groups, persistent group-0 replenishment, exact live transient formation timing/template/path randomization, encounter-local alien hit/total statistics with the original mission double-count/fold quirks, and shareware-reachable Lid/Top/Gemini lifecycle/score tails are session-owned; exact sprite-mask collision producers, boss movement/attacks and remaining non-trajectory enemies remain to be connected |
| death/restart/post-game continuity | PARTIAL Phase-4 integration | bomb-driven player destruction entry, -540 spawn quiet period and exact deferred life-consumption/respawn/game-over gate are session-owned; death-effect presentation and remaining results execution are still external |
| session → fidelity presentation handoff | OPEN Phase-4 integration | Phase-3 renderer contracts are complete; complete semantic presentation inputs are not yet generated from `GameSession` each frame |

## Status legend

- **CONFIRMED** — directly established by binary evidence, independent correspondence, or exact reproduction.
- **PARTIAL** — structure/control path is known but semantics remain incomplete.
- **OPEN** — not yet established.

## Phase 2 closure baseline

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
| dynamic Win32 palette effects | CONFIRMED | `0x004011E0` inclusive DirectDraw uploads; generic `0x00403490`; purpose-built `0x0041EFE0`/`0x0041EE90` bands; exact settled four-phase upload ranges clean-tested |
| gameplay HUD presentation | CONFIRMED | score/lives FONT2 anchors, six-Drone mini-probe outcome strip, Probe/Stinger status protocol/target reticle clamps, and exact 4-pixel shield-meter geometry/color bands are recovered and clean-tested |
| player reconstruction | CONFIRMED core slice | player root `0x00466B18`; 22x22/15-frame ship bank, Left/Right + A/Z motion, exact bounds and banking mapped; lives are deferred until death settlement; 3-life init, respawn and game-over transition are clean-tested |
| gameplay input aggregation | CONFIRMED semantic boundary | Win32 keyboard + normalized DirectInput joystick, DOS keyboard/game-port convergence, exact six-channel demo replacement, live vertical overlay and portable `GameplayInputFrame` are recovered and clean-tested |
| rapid-fire missiles | CONFIRMED core slice | Ctrl path; 8-slot 1x9 pool, 3 frames, cooldown/spawn/update/top cleanup, missile.wav voice pool and rendering chain mapped; late Drone point-hitbox collision now deactivates the first colliding slot and starts the owned destruction countdown; clean module tested |
| player shield | CONFIRMED core slice | Space/demo channel 5; 75-unit high-word accumulator, +0x514 recharge, 0xBB80 drain, protection flag, shields.wav cadence and bomb-absorption path mapped; clean energy model tested |
| native fidelity host | CONFIRMED Linux capture boundary | X11, Win32/GDI, Cocoa/CoreGraphics backends present; Linux host accepts JBA/DRONEFB1, supports display-free landmark capture, and has an automated byte-exact no-DISPLAY round-trip gate |
| Probe/Stinger special weapon | CONFIRMED core lifecycle + Drone interaction | shared 3x8 entity, load/cycle/launch/homing mapped; Probe-vs-Drone point-hitbox attachment, +10 award, exact live/demo two-stage thresholds, decode status 0→3→1, +500 completion and same-update Drone release are clean-integrated; enemy-bomb knockoff and exact phase-2-only decoder interruption are integrated; red Stinger-vs-Drone starts the destruction countdown; exact stateful hostile target priority/retention and X=160 dummy reset are clean-integrated; downstream state-4 Mothership details and candidate actor geometry/AI remain partial |
| demo replay system | CONFIRMED core semantics | playback/recording flags and all 14 channels mapped; DOS/Win share zero-reset, one-preincrement-per-gameplay-update and 0x82F terminal clock; four shared demos byte-identical |
| six-Drone objective outcomes | CONFIRMED core semantics | six-entry ledger 0 unresolved / 1 disarmed / 2 detonated; normal Y=201 commit plus 60-slow-tick settlement gate recovered; `0x0041D220` detonation path, per-objective good/bad + mission interstitial, and final result art/music reductions clean-tested |
| enemy bomb gameplay slice | CONFIRMED core lifecycle | 10-slot 1x9 pool; live steering magnitude rand()%3, replay steering forced 0, Y+=2 update, 3-frame animation, visibility/lifetime, ordered Probe/Stinger→player late collision loop, shield/lethal consequences, and shared spawn-gate/death quiet-period counter reconstructed |
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
| small Windows JBA family | CONFIRMED physical format | byte-0 preamble length; PCX at `1+N`; 128×128 8-bit single-plane RLE; final markerless 768-byte RGB8 palette; runtime owner not asserted |
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
| world/effect subpass catalog | CONFIRMED Win32 order | 28 evidence-backed subpasses resolve boss composites, direct point particles, Gemini procedural effect, explosion/debris routing and actor/projectile tail before scaled overlays |
| native fidelity host | PARTIAL platform validation | Linux build validated; source backends included for Windows/macOS |
| simulation reconstruction | PARTIAL continuous integration | `GameSession` now continuously owns campaign/encounter state, all 17 trajectory groups, exact Probe attachment/decode/disarm state, attached-Probe bomb interruption, ordered bomb→player shield/lethal handling, player destruction entry and deferred respawn/game-over settlement, rapid-missile/Stinger Drone-hit producers, normal Drone objective X/Y travel/hover/disarm/settlement, the exact timeout-driven pre-detonation countdown, outcome-2/-1000 detonation commit, logical effect settlement, Drone life-loss/restart/EndRun ordering, automatic Y=-200 shareware boss dispatch, and Lid/Top/Gemini lifecycle/score tails; red-Stinger target selection/retention, live transient trajectory spawn selection/timing, and encounter-to-mission alien-statistics accounting are now internal; death-effect presentation, trajectory/boss collision producers, direct detonation visuals, boss movement/attacks and remaining non-trajectory actor geometry/AI remain Phase-4 work |

## Phase 3 closure / Phase 4 priorities

Phase 3 is complete. The renderer/world architecture now includes the corrected 19-pass Win32 presentation order, detailed world/effect and scaled-overlay catalogs, startup palette fade, complete late HUD/outcome-cursor contracts, framebuffer comparison tooling, and Linux headless capture validation.

Phase 4 priorities are now:

1. Extend established continuous `GameSession` ownership from trajectories, Drone/Probe interaction and shareware boss lifecycle into the remaining non-trajectory encounter actors and boss movement/attack producers.
2. Finish the remaining non-trajectory actor geometry/AI and collision producers feeding the now-native Stinger selector without inventing unavailable retail behavior.
3. Continue whole-frame integration into trajectory/boss collision producers, reconstruct the player-death presentation producer behind the now-integrated respawn gate, and complete the remaining non-alien results continuity without pulling retail-only unknowns forward from Phase 8.
4. Feed the Phase-3 fidelity presentation contracts from clean session state while preserving the simulation/presentation boundary.

Exact original-runtime trace parity remains Phase 6; end-to-end shareware discrepancy closure remains Phase 7.
