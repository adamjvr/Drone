# Research Log

This file records major findings and engineering decisions chronologically. It is intentionally concise; detailed evidence belongs in the linked specifications and ledgers.

## 2026-08-27 — Phase 2 canonical input aggregation

- Promoted Win32 `0x00406AC0` to `poll_directinput_joystick_state` and `0x00420090` to `normalize_joystick_gameplay_actions`.
- Recovered the DirectInput axis dead zone of ±360 and the configurable rapid/load/launch/shield button-index mapping.
- Established the state-2 source rule: live keyboard OR normalized joystick, or replay replacement for channels 1–6.
- Confirmed that live vertical movement is outside replay substitution in both Win32 and DOS.
- Correlated DOS game-port button/axis normalizers with the same six replay-action meanings.
- Added portable `GameplayInputFrame`, semantic live-source OR, and six-channel replay overlay with regression coverage.
- Resolved `Q-INPUT-001`; no original packed action bitfield is claimed.

## Phase 0 — Evidence intake / format reconnaissance

- Accepted the supplied DOS and Windows shareware packages as the initial evidence set and pinned them by SHA-256.
- Identified DOS `DRONE_SW.EXE` as a 32-bit x86 Watcom C/C++ LE program using DOS/4GW.
- Identified Windows `Drone_sw.exe` as a compact native Win32 PE32/i386 game using DirectDraw, DirectInput, DirectSound, WinMM, and Microsoft C/C++ runtime-era conventions.
- Recovered the Windows Wise installer payload as a chain of raw-DEFLATE streams with per-stream CRC-32 values, allowing extraction without running the installer.
- Reconstructed 192 installed Windows files from the known installer.
- Solved full-screen JBA layout and pixel deinterleave. Independent DOS and Win32 loaders agree on the 10-lane algorithm.
- Established CLV as headerless 22,050 Hz unsigned 8-bit stereo PCM.
- Compared corresponding DOS CLV and Windows WAV material and established integer floor-average stereo→mono conversion across common sample regions examined.
- Phase 1 initially interpreted FLY physical storage as count + triples; Phase 2 later superseded that generalization after correlating the actual trajectory loaders. See the later correction entry and `formats/FLY.md`.
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

## Phase 2 — Gameplay Reconstruction checkpoint

- Added a reproducible DOS LE object extractor. On the canonical DOS shareware executable, scripted object 1 matches the independently reconstructed analysis image byte-for-byte; the LE entry resolves to linear `0x0008BECC`.
- Recovered DOS `0x0006940C` as a direct VGA vertical-retrace wait polling port `0x3DA`, bit `0x08`. This materially strengthens the timing investigation but does not yet establish simulation Hz.
- Reclassified Win32 `0x00401660` as a clipped transparent indexed sprite blitter. Source palette index 0 is transparent.
- Recovered a common Win32 `0x154`-byte sprite/entity object around initializer `0x00401780` and frame release routine `0x00401820`; established X/Y, sprite dimensions, 32-frame pointer table, current frame, frame count, and multi-valued activity/state byte.
- Identified a DOS `0x14F`-byte object family with a compatible early semantic prefix as a strong cross-build layout candidate; tail layouts are intentionally kept separate.
- Corrected the Phase 1 FLY interpretation: `CURRENT.FLY` is counted, but runtime trajectory files are raw triples with loader-specific hard-coded counts.
- Promoted FLY field 0/1 to X/Y after tracing trajectory consumers that write them directly into entity positions with entity/group offsets. Field 2 remains `aux`.
- Recorded original-data quirks where canonical `Rightdiv.fly` and `Swarm.fly` are each one record shorter than the Win32 loader loop request.
- Recovered partial trajectory-group headers containing entity count, X/Y/AUX pointer slots, group offsets, counters, and inline `0x154` entities.
- Identified Win32 CRT `srand`/`rand` anchors at `0x00421EC0` / `0x00421ED0`, creating a future deterministic replay/PRNG validation anchor.
- Added the clean `IndexedFramebuffer` contract and native fidelity-host source backends for X11, Win32/GDI, and Cocoa/CoreGraphics.
- Replaced the old milestone-style roadmap with the agreed Phase 0–15 roadmap and no numbered subphases.

## Phase 2 — collision, framebuffer, text, and explosion cluster

- Promoted Win32 `0x00401F60`, `0x00401FA0`, and `0x00402000` into documented collision primitives; clean bounds-safe semantic implementations and synthetic tests added.
- Promoted common entity `+0x10/+0x14` to velocity X/Y and `+0x28/+0x2A` to 0.85-derived collision extents.
- Proved `0x004D9594` is a guarded software 320×200 framebuffer, distinct from the locked DirectDraw surface at `0x004D9584`; mapped pitched copy/fill helpers.
- Identified `0x00401470` as the `font2.jba` bitmap-text renderer through literal menu/retrace strings.
- Split mini/large/composite explosion effect routines and mapped the cyclic explode2/3/4 and expscale DirectSound voice pools.

## Phase 2 — player and rapid-fire reconstruction

- Identified the player entity at `0x00466B18`, proved its 22×22 `ship.jba` frame bank, and recovered all 15 guttered sprite-frame coordinates.
- Reconstructed Win32 player movement: Left/Right move two pixels and write horizontal-motion -1/+1; `A`/`Z` move vertically one pixel; exact gameplay clamps are X=2..297 and Y=120..175.
- Recorded the executable-vs-README discrepancy: the README describes left/right-only movement, while Win32 concretely supports `A`/`Z` vertical movement.
- Recovered the 15-frame ship banking ring and neutral recenter behavior; added a narrow clean player-motion module with regression coverage.
- Identified the normal Ctrl-fire missile pool at `0x0042F200`: eight 1×9 common entities, three shared frames from `missile.jba`, active count `0x00440274`, capacity `0x0042B198=8`, and cooldown `0x004406F4` saturating at 8.
- Corrected an early analysis note that briefly associated Space with firing: executable data flow and `shields.wav` prove Space is the shield path; Ctrl is normal rapid fire.
- Reconstructed missile spawn at player+(11,-3), Y-=3 update, three-frame animation, `y<0` edge flag, and `y<-7` retirement threshold; clean rapid-missile module and synthetic tests added.
- Mapped missile SFX pool `0x00446028` to `missile.wav` and regenerated metadata-only sprite hashes to include all three missile frames.
- Added `drone_gameplay_probe`, which exercises the clean JBA -> sprite extraction -> player/missile simulation -> transparent blit chain against user-supplied reference assets without storing original pixels in the repository.


## Phase 2 — Probe/Stinger and deterministic demo replay

- Identified special projectile root `0x0045A148` as one 3×8 common entity with blue Probe and red Stinger in frame slots 0/1; recovered load/cycle/launch/homing and Probe attachment/decode behavior into tested clean code.
- Established `0x00440594` as demo playback mode and `0x0047EBD4` as the common 2,101-frame replay index.
- Established `0x00466B00` as the recording-enabled gate: nonzero state-2 paths write replay channels and the terminal path saves `demo.dat` then clears the flag; the path that enables recording remains open.
- Mapped all 14 replay consumers: six selected controls, three trajectory-group script channels, bomb spawn/X/Y, and Drone X/Y.
- Identified the bomb replay pool at `0x004651A0`, populated from three 1×9 frames in `bomb.jba`; live recording stores successful bomb outcomes while playback recreates them.
- Confirmed trajectory replay fields 7–9 have no gameplay writer in the canonical Win32 executable, distinguishing them from live-recorded channels and suggesting pre-authored/preloaded trajectory scripting.
- Confirmed DOS `DEMOA2`, `DEMOA4`, `DEMOB1`, and `DEMOB3` are byte-for-byte identical to the Windows copies, providing cross-build deterministic evidence.
- Added semantic replay decoding, gameplay checkpoint adapter, metadata-only demo manifest, and exact preservation of the original loader's low-byte/low-word narrowing quirk.

- Reconstructed the Win32 enemy-bomb pool at `0x004651A0`: capacity 10, three shared 1×9 `bomb.jba` frames, live `rand()%3` steering magnitude, Y+=2 update, visibility/lifetime thresholds, and player/special collision paths.
- Identified and preserved a replay-specific bomb quirk: demo channels restore bomb X/Y but playback explicitly forces common field `+0x10` to zero, unlike live spawns. Clean `gameplay/enemy_bomb.*` code and synthetic tests added.

## Phase 2 — Shield, retrace toggle and special terminal states

- Recovered player shield accumulator `0x0047FCB0`: nominal reset `75<<16`, HUD displays high 16 bits, recharge `+0x514`, active drain `0xBB80`, negative clamp to zero, and protection flag `0x0046198C`. Added a clean tested shield state module preserving the high-word-only recharge/overshoot quirk.
- Established `0x0041EB70` as the shield-meter renderer and `0x0041E6D0` as the player shield visual-effect renderer; the latter's role is established while its complete pixel algorithm remains partial.
- Proved Win32 `VK_TAB` directly negates `0x0042B1B4` between +1/-1. The same UI path renders `VERTICAL RETRACE` and `SYNC ON/OFF`; +1 enables the 15,000-QPC-count state-2 wait.
- Promoted Probe/Stinger activity state 4 to an evidence-backed hole-interaction state and state 10 to a generic one-update impact-consumed terminal state reset by the common special dispatcher.
- Confirmed enemy-bomb/player collision ownership of the shield flag: unprotected impact enters player destruction, protected impact suppresses destruction and routes the bomb into the mini-explosion/effect path.


## Phase 2 — Player lives, deferred death settlement, and game-over banner

- Established `0x0042B1AC` as `player_lives`: normal initialization writes 3, the hidden L-key path writes 9 and enters the "GRANTED NINE LIVES" message state, the HUD reads/clamps it, and state-2 branches to post-game processing once it is non-positive.
- Proved that `0x0041CDF0` player destruction does not decrement lives. State-2 later consumes a life only after a world/death-effect/player/Drone settlement gate passes.
- Recovered exact settlement ordering: decrement life, reset shield to `75<<16`, reset player frame/X/Y to `0/(147,175)`, then reactivate only if lives remain; otherwise call the dedicated game-over banner routine. Added clean lifecycle code and regression tests.
- Established `0x0041E420` as the `gameover.jba` banner slide. The 117x20 sprite starts at X=325/Y=90, uses fixed-point X with velocity 270000 and decrement 2500, performs 108 vertical-blank-paced presentations, and stops at X=100. Added a pure clean animation state and metadata-only sprite hash.
- Kept the larger region beginning `0x004115BE` conservatively named post-game/results flow because it references disarm result art and multiple soundtrack choices beyond the banner itself.
- Resolved `0x00438C14` as the shared enemy-bomb spawn gate/cooldown: state-2 saturates it at 5; bomb spawns require 5 and reset it to 0; session init writes -450; player destruction writes -540 (`-20*27`); respawn becomes eligible once the same counter rises above -356. Added clean gate-state tests and removed the former generic timing-gate hypothesis.
- Recovered the six-Drone mission result ledger at `0x00472590`: value 1 is disarmed and value 2 is detonated; `0x00433B54` advances processed Drone outcomes up to six.
- Recovered post-game result reduction: count disarmed/detonated outcomes, select `disarm0.jba`..`disarm6.jba` from disarmed count (with `disarm6m.jba` when Mothership core activity is state 2), and choose results music using the exact hiphop/moon/suspense/choral branch order. Added clean `mission_outcome` code and tests.


## Phase 2 — Scoring, extra lives, and high-score persistence

- Established `0x004D95F4` as total score and `0x004D95F8` as a separate rolling extra-life progress accumulator. Normal signed score events mutate both; ordinary penalties can therefore erase life progress.
- Recovered the exact 500-point conversion: state 2 grants one life and subtracts exactly 500, consuming at most one threshold per update and preserving any remainder.
- Recovered the dedicated Drone-detonation consequence: subtract 1000 score with floor-at-zero semantics and clear extra-life progress.
- Preserved the HUD normalization quirk: negative score/progress floor to zero and score values at least 9999 lose exactly one 9999 block per HUD pass. Added clean `gameplay/scoring` code and regression tests.
- Proved the L-key cheat end-to-end from the executable: set lives to 9, set `0x004461D8` high-score-disqualification, enter state 99, render the nine-lives/disqualified messages, and bypass post-results high-score qualification.
- Recovered the ten-entry high-score table as parallel arrays ordered lowest-to-highest. Qualification is strict `new_score > existing_score`; insertion discards the lowest entry and shifts entries through the selected insertion point. Interactive names are capped at 25 characters, uppercase-normalized, and support digits/space/backspace.
- Recovered the Win32 runtime `scores` file reader/writer. Names and numbers are obscured by a clear-text three-digit padding count plus 300..699 random filler bytes before each meaningful character/digit. Added a bounds-checked clean decoder and deterministic structurally compatible encoder.
- Cross-build triangulation resolved the final two persisted high-score statistics: `0x00446DE8` is **Percentage hit**, confirmed by identical DOS arithmetic and the original results-screen label; `0x00440428` is **Mothership destroyed**, populated from live flag `0x00464B90` after the registered Mothership destruction sequence.
- Identified `0x00413290` as the Mothership asset initializer from its `hull0..3`, `panel0..3`, `damage0/1`, `hub`, `motor`, and `hole` loading cluster. The post-six-Drone transition invokes this path, linking the README's registered-game description to executable evidence.
- Established that the Mothership destruction milestone awards +500, plays `thunder2.wav`, sets the live destroyed flag, and later drives the run toward its terminal state. The clean high-score record now uses semantic fields `mothership_destroyed` and `percentage_hit`.
- Parsed canonical DOS LE internal fixups to establish an independent Mothership asset-lifecycle pair: DOS `0x0006C964` loads the same hull/panel/damage/hub/motor/hole cluster and `0x0006CDB0` releases it, corresponding to Win32 `0x00413290` / `0x00413870`. Added a reusable narrow LE xref scanner and metadata-only asset-xref manifest.

- Corrected Mothership resource ownership: `lid.jba` is not part of the Mothership loader. Win32 `0x00417350` owns a separate `lid.jba` + `top.jba` + `retro1.wav` + `level1.wav` boss resource family; paired cleanup is `0x00417450`.
- Established Lid/Top boss initializer `0x00417220` and active update `0x00416700`. The 36×40 nine-frame lid entity (`0x004406F8`) and 68×56 top/root entity (`0x00446E00`) are common `0x154` objects; lid state 2 advances `lid+0x34` to 25, then awards +100 score/progress and enters root destruction state 2, matching the README boss score rule.
- Recovered the six-entry pre-Drone boss initializer dispatch keyed by `drone_outcome_processed_count` when the canonical `drone.jba` objective entity reaches Y=`-200` (`0x00446084`, common entity `+0x04`); the Lid/Top family is reused for processed counts 0 and 4.
- Recovered the three-screen world scenery buffer at `0x004D9598` and progress branches for river/desert/isle/house/night top/mid/bottom families. The second processed-outcome branch explicitly terminates the shareware run instead of loading a third scenery family, matching the documented two-level limit.
- Classified DOS `0x00085D10` as a Mothership/orders presentation resource path rather than the live destruction producer: it loads/plays `thunder2.clv`, loads/releases Mothership art, and cycles `order1.jba`..`order5.jba`, but does not modify score or the DOS live Mothership-result scalar.


## Phase 2 — Boss dispatcher completion

- Established Gemini lifecycle: `0x00405000` update, `0x00405EF0` initialize, `0x00405FB0` load, `0x00406190` release. The loader reconstructs a shared 30-frame 56x41 body bank from `Gemini1.jba`/`Gemini2.jba`, a 43x34 `Gemhead.jba` frame, and Gemini/Level2 audio.
- Corrected the earlier Phase-1 provisional `0x00406190` generic audio-shutdown label; the function is resource-specific Gemini cleanup.
- Established Spidey lifecycle (`0x00414D80` / `0x00415AC0` / `0x00415C80` / `0x00415F40`) and state-2 counter-45 +100 destruction checkpoint. Registered assets are absent from the supplied shareware corpus.
- Established Bomber lifecycle (`0x00403650` / `0x00404350` / `0x004044B0` / `0x00404690`) and state-2 counter-60 +100 destruction checkpoint. Corrected the active-update start from provisional `0x00403640` to the direct-call target `0x00403650`.
- Classified dispatch slot 2 as a surviving registered-only combat implementation with initializer `0x00407980`, update `0x00406CC0`, release `0x00407AB0`, direct destructible-pixel sprite damage, and state-2 counter-45 +100 destruction behavior. Its dedicated loader/proper asset identity remains unresolved.
- Promoted the canonical boss dispatch to Lid/Top → Gemini → registered-slot-2 unknown → Spidey → Lid/Top → Bomber. The clean `boss_progression` module and tests preserve this ordering and the shareware reachability boundary.

- Cross-build replay clock established: DOS `0x4CE8C` and Win32 `0x0047EBD4` both reset to zero, pre-increment once per active gameplay update, and terminate at `0x82F` (2095); clean `DemoReplayTimeline` added without asserting Hz.

- DOS video mode proven directly: startup calls `0x00067E50` with `0x13`; helper invokes BIOS `INT 10h, AH=0`, establishing standard VGA mode 13h and a nominal ~70.086 Hz display-sync reference.
- Resolved the main FLY runtime semantics in Win32 trajectory updater `0x00415FA0`: trajectory-owned `+0x32` is path index, `+0x36` its signed 16-bit step, and `+0x38` its wrap threshold. The same index selects parallel X/Y/AUX arrays.
- Resolved normal FLY AUX behavior as sprite animation control: AUX <= 1 applies a signed relative frame delta, AUX > 1 selects absolute frame `AUX-2`, then the original wraps against the entity frame count. Added clean trajectory helpers and regression tests.
- Reconciled the DOS timing contract after mode-13h/retrace proof: the clean fidelity core now has a canonical ~70.0863 Hz DOS tick, while the Win32 15,000-QPC-count guard remains intentionally unconverted because the executable never queries QPC frequency.
- Resolved the `Rightdiv.fly` / `Swarm.fly` one-record-short loader mismatch as non-required gameplay data in canonical path cycles: Rightdiv's canonical replay groups cap at index 117 (exactly the physical file's final index), while Swarm groups cap at 945, below both the last three physical records and the nonexistent loader slot 949.

## Phase 2 — trajectory-group lifecycle and Win32 state protocol

- Recovered the canonical trajectory-group pool as 17 fixed `0x2148`-byte records rooted at `0x00495CF0`; promoted `0x0042B18C` to `trajectory_group_count` and `0x0042B190` to `active_trajectory_group_count`.
- Promoted trajectory-group `+0x00` from an opaque byte to an established mode field: 0 inactive, 1 persistent loop, 2 retire-on-path-wrap, 10 breakaway fly-off. Promoted `+0x02` to active-entity count and distinguished it from `+0x10` activated-slot count.
- Recovered exact non-primary stagger activation using `+0x0C/+0x0E`, including the original quirk that the delay counter is not reset after every fixed slot has already activated.
- Recovered mode-2 path-wrap retirement and zero-active group teardown. Activity 3 is now trajectory-contextually established as path acquisition: the entity approaches the current path-derived target before entering activity 1 normal following.
- Recovered randomized live mode-10 breakaway gating and motion: demo/recording suppression, phase-2 gate, `rand()%300 < processed Drone count`, 16.16 positions, `0x8000` initial axis speeds, +700 acceleration to `0x28000`, off-screen targets, exact cleanup bounds, and horizontal frame progression. Added clean helpers and regression coverage.
- Completed the tracked user-facing Win32 `game_state_raw` map: 0 exit, 1 main-menu reset entry, 2 gameplay, 3 instructions, 4 menu re-entry, 5 pause, 6 quit confirmation, 7 ordering information, 8 high scores, 13 demo launch, 99 nine-lives/disqualification notice.
- Promoted `0x00418AC0` to `run_main_menu`, established its seven visible selection transitions, and identified `0x0041C4B0` as the nine-page instructions viewer and `0x0041B730` as the ordering-information modal.
- Established the state-1/state-4 menu lifecycle distinction and exact state-13 consumption: enable demo playback, reset replay index to zero, then enter state 2. Added a narrow clean `game_state` protocol module and tests.
- Closed critical `Q-STATE-001`; reconciled stale Phase 2/status/open-question documentation so the already-resolved DOS cadence and FLY semantics are no longer described as open.

## Phase 2 — fixed trajectory-template catalog and gameplay substep phase

- Completed the contiguous Win32 startup catalog for all **17** fixed `0x2148` trajectory-group templates from `0x00409060..0x00409CD5`, including path family, entity count, initial mode/active state, stagger interval, inclusive path end, sprite dimensions/frame count, and non-default formation/path-index seeds. Added `manifests/trajectory_group_templates.csv` plus clean `trajectory_templates` code and regression coverage.
- Established the two runtime-generated trajectory descriptor families conservatively as `Generated402` and `Generated422`. Preserved the group-15 startup anomaly: its runtime descriptor is Generated422 while its inactive-slot initial X/Y sample is sourced from Generated402.
- Tightened generated-AUX evidence: the path generator writes `0xFF` to each just-advanced AUX index; allocation slot 0 is not proven initialized by that loop and is deliberately not invented in the clean model.
- Resolved Win32 global `0x0053C4D8` during active state 2 as an exact **four-phase gameplay substep** with cycle `0→1→2→3→0`. The phase advances near the start of the gameplay invocation and phase 2 gates selected slower animation/effect/random-transition work, including normal trajectory AUX animation and live breakaway eligibility.
- Added `gameplay_phase` clean code/tests and `docs/reverse/GAMEPLAY_UPDATE_PHASE.md`, recording the first reliable whole-frame ordering landmarks from creation/trajectory work through boss, collision/effects, rendering/HUD, QPC pacing, and framebuffer presentation without overstating branch-complete partitioning.
- Reconciled status, Phase 2, timing, subsystem, structure, findings, globals, and open-work documentation so the fixed template catalog and four-phase protocol are no longer listed as unresolved.
- Identified the formerly unnamed post-trajectory state-2 call `0x0041E4D0` as `update_drone_detonation_effect`, paired with `0x0041D220` `trigger_drone_detonation_sequence`. The updater is gated to gameplay phase 0, detonation tick >25, and Drone activity 2; it emits explosion sprites around the captured Drone center and drives the late destruction-settlement field after the logical tick caps at 330.
- Recovered the exact update-side Drone-detonation RNG sequence in `0x0041E4D0`: each eligible phase-0 effect tick consumes 17 MSVC CRT `rand` draws, emits four center-scatter and four 90-degree radial-ring explosion requests, resets settlement at tick 329 and advances it above 329. `0x00401D20` itself consumes no RNG. The separate render-only `0x0041EBE0` radial-noise pass retains its own geometry-dependent RNG stream and remains fidelity-renderer work.


## Phase 2 — cyclic scenery scroll ownership and Drone approach split

- Exhaustively traced Win32 `world_scroll_row` `0x004D9590`. Normal gameplay initializes it to **599**, decrements it exactly on shared gameplay phase 2, and wraps exactly `0 -> 599`; there is no second active-gameplay producer. Added clean `gameplay/world_scroll` code and tests.
- Established state-7 Ordering Information as the only non-gameplay reuse of that scroll scalar: it resets the row to `0`, advances an independent local `0→1→2→0` counter, and decrements/wraps on local phase 2. This modal cadence is intentionally not conflated with the four-phase gameplay scheduler.
- Promoted `0x00403560` to `compose_ordering_information_world_background`: it copies exactly 145 cyclic scenery rows into framebuffer rows 35..179, preserving the modal UI bands. Added the bounds-safe clean compositor and wrap tests.
- Corrected the former “world/progression coordinate” interpretation of `0x00446084`. `0x00446080` is the already-established `drone.jba` common-entity root, so `0x00446084` is its `position_y`; session init writes `(155,-850)`, demo channels 13/14 record/replay the same entity X/Y, and the pre-Drone boss dispatch fires at Drone Y=`-200`.
- Narrowed `Q-LEVEL-001`: shareware scrolling and boss-approach coordinate ownership are no longer blockers. Remaining level/world work is registered/endgame transition semantics (especially branch 6) and original naming/reachability evidence.


## Phase 2 — post-game results, high-score handoff, and credits

- Partitioned the inline Win32 post-game region beginning at `0x004115BE`: active state 2 enters it directly when `player_lives <= 0`; it is not a separate top-level state.
- Promoted byte `0x0042F1FC` to the post-game presentation-suppression flag. Quit/demo-terminal abort paths set it to skip Results plus Ordering Information, while high-score qualification remains independently gated only by demo playback and high-score disqualification.
- Recovered all six rendered result statistics (hits, misses, total, integer percentage hit, score, disarmed Drones) and the exact 58-present/vblank minimum confirmation lock before `confirm_input_pressed` is consulted.
- Established the exact post-results high-score invocation `run_high_score_table(1, qualifying_index, disarmed_count)` versus menu display `(0,0,0)`. Preserved the original index-0 quirk: placeholder `ENTER YOUR NAME`, no interactive editor, and no score-file save.
- Promoted `0x00404720` to `run_completion_credits` from its `credits.wav`, `credit1.jba`, and `credit2.jba` resource path. Six disarmed Drones plus the established Mothership destruction outcome runs credits and returns through state 1.
- Added clean `post_game` planning code and regression tests, and resolved `Q-RESULT-001`.
- Resolved the long-standing `Q-ENTITY-001` 0x14-byte stride as the FONT2 glyph descriptor/cache, not a gameplay entity: Win32 builds 64 records at `0x00466C90` and DOS independently builds the same 64×0x14 layout at data offset `0x6F80`, with exact 16×4 / 7×5 gutter geometry, character-0x20 indexing, mask allocation/extraction/render helpers, a new DOS↔Win32 correspondence, and clean `fidelity/font2.*` regression coverage.

- Closed `Q-ENTITY-002`: promoted DOS `0x00068220` / `0x000682D0` / `0x00068300` as the init/free/blit counterparts of the Win32 common entity family and established the `0x14F` ↔ `0x154` field map. Recovered common damage accumulator `+0x30`, destruction threshold `+0x31`, destruction-burst count (DOS `+0x14D`, Win32 `+0x14F`) and score value (DOS `+0x14E`, Win32 `+0x150`); classified contextual overlays and the 128-byte unreferenced middle block; added exact combat metadata to all 17 trajectory templates.

## 2026-08-27 — Phase 2 closure / Phase 3 transition

Phase 2 closes after `Q-ENTITY-002` established the field-level Win32 `0x154` ↔ DOS `0x14F` common entity correspondence. The machine-readable queue has no unresolved critical simulation-architecture question. The roadmap now advances to Phase 3 rather than retaining renderer completeness, complete-game simulation, deterministic trace parity, or retail-only evidence as artificial Phase-2 blockers.

A durable `scripts/check_phase2_exit.py` gate checks this boundary in CTest. Phase 3 begins from the existing indexed-framebuffer, sprite blit/extraction, world viewport, FONT2, debris/effect, and simulation→presentation ordering contracts.


## 2026-08-27 — Phase 3 dynamic palette presentation

- Closed `Q-RENDER-001` by classifying the late Win32 presentation cluster. `0x004011E0` is now `upload_directdraw_palette_range`, an inclusive DirectDraw `SetEntries` wrapper over a biased 0x44-stride palette source.
- Recovered `0x0041EFE0` / `0x0041EE90` as initializer/updater pairs for four purpose-built mutable bands: sparse flashes 110..112, red breathing 96..102, timed yellow/olive 128..148, and timed green 103..109. Exact random-consumption order, colors, clamp/reversal values, periods and toggles are clean-tested.
- Recovered `0x00403490` as the distinct generic late-game palette animation kernel over 64..170, 192..213 and 224..233, including negative channel clamp, blue-channel bound/stop priority and `rand()%100 < 2` inactive activation.
- Established the settled state-2 DirectDraw upload schedule: phase 0 uploads 32..42 + 64..110; phase 1 uploads 111..156; phases 2/3 upload 157..170 + 192..213 + 224..234. Unsettled paths upload 0..255.
- Added `fidelity/palette_effects` plus a dedicated `drone_fidelity` CTest target. Historical 0x44 records remain evidence only; clean code separates RGB, effect controls and host upload planning.

## 2026-08-27 — Phase 3 small-JBA / embedded-PCX container

- Resolved `Q-JBA-002` from all three canonical Windows-only small `.JBA` members (`Logo.jba`, `River.jba`, `Screen.jba`). Byte 0 is an opaque-preamble length `N`; the embedded PCX begins exactly at `1+N` (offsets 65, 5, and 39 respectively).
- Established the common PCX contract: version 5, RLE, 8 bits/pixel, one plane, bounds 0..127 in both axes, 128 bytes/line, and exactly 16,384 decoded indices.
- Established the trailer boundary exactly: after RLE completion, precisely 768 bytes remain and are a shared full-range RGB8 palette. The canonical files omit the conventional PCX `0x0C` 256-color palette marker.
- Added a separate clean `load_small_jba_pcx128` decoder, PPM inspection support, synthetic markerless/RLE regression coverage, a metadata-only analyzer, and `manifests/small_jba_pcx.csv`. The ordinary 320×200 RGB6/lane-interleaved JBA loader remains separate.
- A complete canonical Win32 executable string inventory contains many runtime `.jba` names but not `Logo.jba`, `River.jba`, or `Screen.jba`; this milestone therefore establishes physical format/import semantics without inventing a game-runtime owner.

## 2026-08-28 — Phase 3 closure: scaled overlays, startup fade, outcome cursor

- Resolved the scaled state-2 block `0x00410C15..0x00410E9D`: active `miniexp1` and `explode1` pool members with contextual `+0x14E == 1` use the scaled route, followed by a shared `debris1/debris2a/debris3` objective-destruction effect.
- Established that the three scaled debris sprites are shared by Mothership destruction and Drone detonation, including source dimensions/frame counts, initial velocities, phase-2 symmetric growth and exact visibility prefilters.
- Corrected the presentation catalog from 18 to 19 passes by restoring the startup palette fade at `0x00410E9D..0x00410F34`; recovered x87 truncate-toward-zero conversion of `255-counter*4.19` and phase-2 counter progression 0..62.
- Identified the former auxiliary HUD sprite as `square.jba`, a 13x18 current Drone-outcome cursor whose target moves upward by 19 pixels per committed outcome and disappears after the sixth outcome.
- Phase 3 exit criteria are now met: renderer/world architecture is deterministic and comparison-ready. Roadmap advances to Phase 4 complete-game simulation; exact original-runtime trace parity remains Phase 6.

## 2026-08-28 — Phase 4 continuous session ownership

- Added clean `GameSession` campaign/encounter ownership instead of recreating original global-memory ABI.
- Implemented full-campaign versus encounter-only reset boundary using the recovered reset scope.
- Integrated continuous active-gameplay ticking for phase cadence, player input/motion, rapid missiles, Probe/Stinger state, shield, existing enemy bombs, cleanup, world scroll and one-extra-life conversion.
- Kept encounter target/redirect facts explicit until actor collections and encounter producers are integrated.
- Added dedicated C++ session tests plus an asset-free deterministic 120-update `drone_session_probe` oracle.

## 2026-08-28 — Phase 4 normal Drone objective ownership

- Re-read the Win32 state-2 Drone block `0x0040E4A8..0x0040E849` against the continuous-session scheduler and promoted normal Drone position/progression into `GameSession`. Full reset evidence initializes the objective at `(155,-850)` and settlement scalar `0x004D9600` at 61.
- Established the exact phase-2 approach/hold route: unresolved Y advances below 45; the transient -117/-40 audio landmarks are immediately replaced by -116/-39; Y=44 resets the hold counter; Y=45 increments `0x004460B6`; exact hold count 4200 is the timeout handoff into the still-separate destructive countdown.
- Tightened completed-Probe behavior: decode status 1 releases the Y=45 hold, the existing Y=201 disarm commit forces Y=202, Y=230 resets settlement, the next completed-disarm tick reaches Y=231 and clears decode status, and Y remains 231 until the early phase-2 settlement scalar reaches exactly 60.
- Integrated `run_mission_outcome_transition` semantics at the session boundary without proprietary presentation assets. Processed count 1 performs the encounter-only Gemini continuation and rebuilds Drone Y to -1200; processed count 2 performs the compiled shareware Results/EndRun branch, zeroes lives, and rebuilds Y to -1350.
- Removed external `drone_x` and `boss_approach_boundary_reached` inputs from `GameSessionTargetContext`. Probe homing/pinning now reads the owned Drone X, while boss dispatch derives the exact Y=-200 boundary from the owned phase-2 path.
- Added `gameplay/drone_objective.*`, dedicated objective regression tests, expanded whole-session transition tests, and retained the byte-identical 120-update session oracle. Destructive Drone countdown/detonation remains the next explicit Phase-4 slice.

## 2026-08-28 — Phase 4 Drone detonation and life-loss ownership

- Promoted 16-bit `0x00491CAC` to the Drone destruction countdown: idle is 100; the unresolved Y=45 timeout starts it at zero only when idle; the pre-phase state-2 block increments it; reaching 99 immediately restores 100 and triggers the destruction sequence before the four-phase scheduler advances.
- Integrated the logical `0x0041D220` trigger contract into `GameSession`: activity 2, Probe-completion clear, 15x38 center capture (`x+7`,`y+19`), detonation tick/settlement reset, exact -1000 score floor plus extra-life-progress clear, and outcome-2/processed-count commit. Because the logical detonation tick advances later in the same update, trigger update ends at tick 1.
- Promoted contextual Drone `+0x32` (`0x004460B2`) to the detonation destruction-settlement WORD. The post-trajectory phase-0 updater is active only for tick>25, resets the WORD at tick329, increments it for capped tick330, and requests four explosion emissions while drifting the captured Y center. Direct framebuffer/randomized rendering remains outside gameplay.
- Integrated the earlier activity-2 / settlement>70 gate. With more than one life, the mission interstitial/encounter reset runs first; then lives are decremented unconditionally and Drone Y is rebuilt from processed count. Last life skips the interstitial and reaches zero. The shareware count-2 destructive Results branch preserves the original signed `0 -> -1` life quirk because the transition zeroes lives before the caller decrement.
- Expanded Drone-objective and whole-session tests for timeout handoff, pre-phase countdown timing, same-update detonation tick 1, phase-0 effect settlement, ordinary restart, last-life game-over and count-2 EndRun. The 120-update normal-play session oracle remains byte-identical.

## 2026-08-28 — Phase 4 Probe decode and Drone weapon-entry ownership

- Re-read Win32 `0x0040F206..0x0040F7F7` and corrected the earlier collision-boundary assumption: rapid missiles and the launched Probe/Stinger test Drone root `0x00446080` with `0x00401F60` point-vs-hitbox, not the opaque-pixel primitive. The 15x38 Drone therefore has inclusive 12x32 collision extents.
- Integrated rapid-missile Drone collision in ascending pool order and red-Stinger Drone collision. Both require Drone activity 1 plus idle destruction countdown `>99`, consume the projectile, request the proven eight-hit effect burst and start the already-owned countdown at zero.
- Completed the blue-Probe attachment/decoder contract: attachment awards +10, live thresholds are `(rand()%70+450)*difficulty` and `(rand()%70+300)*difficulty`, demo thresholds are fixed 210/150, status is exactly `0 -> 3 -> 1`, phase 2 advances to tick 1 on the same update phase 1 completes, and final completion awards +500 plus consumes `rand()%60+40` for the effect parameter.
- Promoted `0x00421ED0` from a generic CRT-rand identity to its exact MSVC LCG contract: `state = state*214013 + 2531011` with uint32 wrap and return `(state>>16)&0x7fff`. Added seedable clean process-lifetime PRNG state without claiming an unproven original startup seed API.
- Removed the semantic completed-disarm input from `GameSessionTargetContext`. Decoder completion now marks the owned Drone disarmed before normal Drone movement on the same update; Y>230 clears the completed decoder state. Whole-session tests also prove the +500 completion is visible to the later one-extra-life conversion that same update.
- Re-read Win32 `0x0040F35F..0x0040F4B8` from the preserved disassembly and established the late enemy-bomb -> Probe/Stinger collision path. `0x00402000` tests bomb `(x,y+9)` against the special entity; the 3x8 common entity therefore contributes inclusive 2x6 extents. A hit consumes bomb and special. Attached Probe decoder reset occurs only when status is not complete and phase-2 elapsed is already greater than zero; phase-1-only elapsed state is not explicitly cleared by this branch.
- Integrated that producer into `GameSession` before the later weapon-to-Drone collision stage, added dedicated helper/session regression coverage, and preserved the distinct Probe/Stinger impact-event contracts without pulling DirectSound/framebuffer work into the gameplay core.

## 2026-08-28 — Phase 4 enemy-bomb player death and respawn ownership

- Re-read the complete late enemy-bomb collision loop `0x0040F330..0x0040F5A3` and preserved its per-slot ordering: Probe/Stinger test first, player test second, and no bomb-activity recheck between them. One overlapping bomb may therefore consume the special and still hit the player in the same iteration; the bomb pool active count is decremented once afterward.
- Established the player collision geometry used by this path: the 22×22 `ship.jba` common entity contributes inclusive 18×18 extents, while the bomb point remains `(x,y+9)` through `0x00402000`.
- Integrated shielded bomb absorption and unshielded lethal entry into `GameSession`. A lethal hit auto-launches a merely loaded Probe/Stinger before deactivating the player, requests the recovered player-hit/death presentation events, and drives the shared enemy-bomb gate to `-20*27 = -540`.
- Integrated the deferred player-life settlement gate `0x0040E272..0x0040E2DB`: after the shared bomb gate advances above -356, death-effect activity is zero, the player is inactive, lives remain and Drone activity is not 2, the session decrements life, resets shield/frame/position, then respawns if the decremented count remains positive or requests game over at zero.
- Kept the unrecovered player-death effect animation outside gameplay. `GameSessionTargetContext` carries only the exact semantic presentation fact `player_death_effect_inactive`, avoiding invented effect timing while preserving the original settlement dependency.

## 2026-08-28 — Phase 4 red-Stinger target-priority ownership

- Re-read Win32 `0x0040CD47..0x0040CD9D` and `0x0040DF47..0x0040E04E` and promoted shared target pointer `0x004D8508` to explicit state. A successful special load resets it to dummy entity `0x0045A708`, whose X is explicitly written to 160 and whose zero/BSS width makes the initial homing coordinate exactly 160.
- Established the exact frame-1 priority chain: active Mothership panel -> Mothership hole; otherwise Gemini with nearest active head by head-X-to-player-X distance (ties select B); then Lid/Top top only with top state 1 and lid frame >3; Spidey; registered slot 2; Bomber; then the still-unidentified active common-entity owner at `0x00459F90` using dynamic target geometry reached through `0x00495CE8`.
- Preserved the stateful no-match behavior: the movement block leaves `0x004D8508` untouched when no candidate qualifies. The clean session therefore retains the previous target rather than substituting the Drone or screen center on every miss; only successful load explicitly restores the X=160 dummy.
- Integrated `gameplay/stinger_targeting.*` into `GameSession`. Shareware-owned Gemini/Lid activity gates are sourced from the pre-boss-update session snapshot so targetability does not move one scheduler stage early, while geometry/frame facts for actor families whose movement is not yet session-owned remain explicit encounter inputs.
- Added a dedicated target-selection test suite plus whole-session regressions for priority, Gemini tie behavior, persistent target retention and load-time dummy reset.

## 2026-08-28 — Phase 4 live transient trajectory formation ownership

- Re-read Win32 `0x0040D390..0x0040D947` and encounter setup `0x004181E2..0x00418251`. The live phase-2 scheduler uses WORD threshold `310 - 20*processed - 30*difficulty`, initializes its WORD counter 30 below that value, advances it by 3 per eligible phase-2 call, and forces a spawn roll when the counter reaches the threshold.
- Established exact ordinary spawn chance `3*processed + 4*difficulty` (recording mode forces 28) and preserved RNG ordering: `rand()%1200` occurs before Drone-position/activity, active-group-count and registered Mothership-destruction suppression gates.
- Recovered progression-dependent fixed-group selection and the group-0 quirk: shareware counts 0/1 draw 0..11, counts 2/3 draw 1..16 but reroll 12/13 through 0..11, later progression draws 1..16; an initially selected inactive group 0 is legal, while a busy scan wraps 17 back to 1.
- Recovered runtime family substitutions and offsets, including left/right dive and left/right drop coin flips, Swarm/Swoop/Frisbee/generated offsets, optional 14-way flight-SFX RNG consumption, and per-actor `rand()%22 < processed+2` formation jitter with independent `[-30,29]` X/Y values.
- Integrated `gameplay/trajectory_spawn.*` and removed the host-supplied trajectory group/X/Y selection fields from `GameSessionTargetContext`. Immutable path samples and exact sprite-mask collision hits remain external. Original `0x00466B04` is documented separately from mission result total `0x00446078` rather than being incorrectly aliased.

## 2026-08-28 — Phase 4 primary trajectory replenishment and encounter accounting

- Recovered Win32 `0x0040CEE8..0x0040D070` as the persistent primary group-0 replenisher. It skips phase 2 entirely, otherwise consumes `rand()&0x7ff` before all later gates, forces the effective roll to 1 during demo playback or when group 0 has zero active actors, and passes only below `4*(processed Drone count + difficulty)`.
- Recovered exact actor insertion: first inactive group-0 slot in ascending order; `rand()%100` chooses left `(-30,100)`, top `(160,-30)` or right `(350,100)` entry; only X/Y/activity are rewritten and activity becomes 3, preserving retained path index/frame for normal acquire-path motion. An inactive group 0 returns to persistent mode and active-group ownership. Normal live control jumps directly to the later transient producer, so the demo-scripted flight-SFX tail at `0x0040D25B` is not charged to this replenishment path.
- Promoted `0x00466B04` to encounter-local alien total and paired `0x0047EC3C` to encounter-local alien hit count. Reset seeds them to 7/0; the interstitial renders hit, missed, total and percentage from the pair; the post-encounter transition folds them into the distinct mission-wide counters `0x00446078` / `0x0044084C`.
- Integrated encounter-local total ownership into `GameSession`: reset starts at seven primary actors, successful primary replenishment and transient first-actor activation increment it, and later trajectory stagger activation increments it per actor. Campaign-wide folding remains deliberately separate until the original mixed accounting sites are reconciled.
- Added a dedicated primary-replenishment suite covering strict probability boundary, forced rolls, three entry regions, retained actor state, group reactivation, destruction suppression, RNG draw counts and whole-session accounting.

## 2026-08-28 — Phase 4 alien hit and mission-statistics accounting

- Reconciled the mixed accounting sites around common trajectory update rather than treating them as generic Results counters. Non-primary stagger activation at `0x0041610E..0x00416121` increments both encounter-local total `0x00466B04` and mission-wide total `0x00446078` immediately.
- Recovered source-sensitive destruction accounting: rapid-missile transparent-pixel trajectory destruction at `0x004165E4..0x00416607` increments both encounter hit `0x0047EC3C` and mission hit `0x0044084C`, while the special-weapon trajectory destruction at `0x0040EFD1..0x0040EFDF` increments encounter hit only.
- Confirmed the later interstitial does not compensate for those live increments: it renders hit/missed/total/percentage from the local pair, then `0x0041E237..0x0041E25D` adds the entire encounter total/hit pair into the mission pair before encounter-only reset. The executable therefore double-counts stagger actors and rapid-missile kills in mission Results; the clean implementation preserves the behavior rather than correcting it.
- Integrated `gameplay/alien_accounting.*` and source-tagged trajectory hit events into `GameSession`. Encounter reset owns 7/0, every established trajectory destruction increments the local hit counter, only rapid-missile kills apply the immediate mission-hit increment, stagger activations apply the immediate mission-total increment, and both normal and destructive mission interstitials snapshot and fold the full local pair before reset.
- Added dedicated regression coverage for integer percentage rendering, full-pair fold, initial 7/0 state, source-sensitive hit increments, stagger double counting, and the exact interstitial snapshot/fold/reset ordering.

## 2026-08-28 — Phase 4 native trajectory weapon collision producers

- Re-read the trajectory and late-special collision regions and split the old generic hit boundary into the three actual producers. Rapid missiles at `0x00416495..0x00416607` use `0x00401FA0` current-frame opaque-pixel collision, consume the missile and add +3 damage; launched Probe/Stinger uses the actor 0.85 point hitbox and direct destruction; the separate six-frame `stinger.jba` display uses `0x00402FC0` and adds +15 only on frames 3..5.
- Preserved late-block ordering: state 3 is captured after bomb collision; the Drone collision occurs first inside that block, but a resulting Probe attach/special-state change does not cancel the subsequent trajectory scan. The actor loop likewise does not re-test activity, allowing a Probe made inactive on its first direct trajectory hit to destroy additional overlapping actors.
- Corrected accounting attribution: `0x0040EFD1..0x0040EFDF` belongs to Stinger-display AoE destruction and increments encounter hits only. Direct launched Probe/Stinger trajectory destruction is a separate path and does not increment the encounter hit counter at the recovered site.
- Integrated `gameplay/trajectory_collision.*`, the owned six-frame Stinger-display state, and immutable trajectory-frame mask input into `GameSession`; removed external `trajectory_hits` semantic injection. Direct special destruction deliberately preserves the actor's retained damage byte.
- Added dedicated regression coverage for transparent/opaque pixels, actor-major missile consumption, Stinger display frame gating/retirement, multi-actor Probe direct destruction, Stinger display activation, source-sensitive accounting and the same-update Drone-attach-to-trajectory-scan quirk.

## 2026-08-28 — Native Lid/Top combat integration

- Promoted Win32 `0x00417220` / `0x00416700` from a lifecycle-only boss shell into native Phase-4 gameplay ownership.
- Recovered exact 16.16 root initialization/movement, player-left-X tracking, horizontal acceleration/cap, Y>=240 retreat, live phase-2 bomb chance/gate/slot/position behavior, and reused-bomb field preservation.
- Recovered the distinct rapid-missile paths: `top.jba` opaque-pixel shielding under the `root.x+39` pre-gate and the separate frame-0 lid-opening weakpoint at `root+(53,23)`.
- Recovered lid activity-1 opening, activity-6 closing and same-update close fallthrough, plus the Stinger-only exposed core at frame>6 and lid Y>0.
- Confirmed state-2 scheduler ordering: common special dispatch precedes the boss call at `0x0040E858`, so boss collision state 10 persists until the next gameplay update.
- Kept randomized debris/audio as presentation events and retained only immutable `top.jba` frame pixels as the boss's remaining collision asset input.

## 2026-08-28 — Native Gemini combat integration

- Re-read Win32 `0x00405EF0` / `0x00405000` and promoted the second shareware boss from lifecycle-only ownership to native gameplay. The initializer activates both 56×41 bodies and 43×34 heads, clears side damage, initializes body-A 16.16 motion, and derives body B/head geometry at exact offsets `(170,0)`, `(6,41)` and `(176,41)`.
- Recovered shared-root player tracking (`±0x44C` X acceleration with difficulty-scaled cap), Y>=240 retreat, opposing 30-frame phase-2 body animation, and phase-2 enemy-bomb emission. Bomb chance is `rand()%100 < 2*difficulty`; source selection is `rand()%10` with preferred-side fallback, and reused bomb motion/frame fields are preserved.
- Established the exact side-local damage bytes/thresholds: A `0x00464D70/71`, B `0x00464EC8/C9`; Beginner thresholds are 20/20, higher-difficulty thresholds are 35/30. Launched Probe/Stinger collision tests head current-frame opaque pixels first, body current-frame pixels second; Probe adds 3, Stinger adds 15 and activates the separate centered Stinger display. Threshold comparison is strict `damage > threshold`.
- Integrated both independent +100 transitions and 20-phase-2-tick body retirements directly into `GameSession`, removed the external shareware boss-destruction-trigger boundary, and sourced Gemini Stinger target geometry from the native pre-boss-update snapshot.
- Confirmed that side-A threshold code only *temporarily* zeros body-A/shared-root velocity while spawning randomized presentation effects, then restores it before side-B processing. Gameplay motion therefore remains shared-root. Randomized Gemini point-particle/destruction work and the procedural beam still consume the shared CRT PRNG outside the clean presentation owner, so exact presentation/gameplay cross-RNG sequencing remains explicit fidelity work.

## 2026-08-28 — Native player-death explosion lifecycle

- Re-read `trigger_player_destruction_sequence` (`0x0041CDF0`) together with the phase-2 singleton update `0x0040E1DA..0x0040E271`, render gate `0x00410BA8`, common initializer call at `0x00407F4E`, and frame-loader write at `0x0040A785`.
- Established `0x00491CE0` as a separate 42×38 player-death explosion singleton with terminal frame 27. Lethal destruction centers it over the 22×22 player, copies common-entity motion, writes activity 3 and signed frame -6; its randomized debris/audio work remains presentation-side.
- Established the exact phase-2 lifecycle: move; clear if outside `x=-42..319` / `y=-38..199`; increment frame; frame 0 writes activity 1; frame 27 writes activity 0. Because the bounds clear precedes frame-zero activation, an out-of-bounds `-1 -> 0` update can momentarily restore activity 1 before the next update clears it again.
- Integrated `gameplay/player_death_effect.*` into `GameSession`, removed `player_death_effect_inactive` from `GameSessionTargetContext`, and made deferred life settlement observe the native singleton in the same update. Dedicated and whole-session regressions cover pre-roll, visible/terminal timing, bounds ordering, lethal activation, settlement blocking and same-update retirement→respawn.

## 2026-08-28 — Phase 4 native Results/post-game continuity

- Re-read the inline Win32 state-2 post-game tail at `0x004115BE` and tightened the scheduler contract: lives<=0 diverts before ordinary gameplay work; result counter `0x3A` suppresses `confirm_input_pressed` polling for exactly 58 completed presentation iterations, and the first confirmation opportunity is the following loop iteration.
- Integrated the existing exact `win32_post_game_plan()` into `GameSession`. Session-owned campaign outcome/statistics/eligibility state now materializes Results directly, while the persisted ten-entry score table lives above campaign resets.
- Added `PostGameRuntimeState` / `step_game_session_post_game()` to own Results -> state-7 Ordering Information -> state-8 high-score handoff -> optional completion credits -> final state-1/state-4 sequencing. Pixel/audio presentation, interactive name editing and legacy score-file persistence remain host/UI responsibilities.
- Added dedicated regression coverage for pre-gameplay lives<=0 diversion/no RNG consumption, all 58 locked presentations plus separate confirmation iteration, presentation suppression with slot-zero qualification, perfect completion credits and persisted score-table survival across `FullCampaign` reset.

## 2026-08-28 — Phase 5 DirectSound/audio-event foundation

- Recovered the Win32 DirectSound primitive layer around `0x00406200..0x00406860`: RIFF/WAVE PCM slot loading, secondary-buffer duplication, rewind-before-play, exact game-volume conversion `30*(v-100)`, direct frequency setting, stop+rewind, release and raw status query.
- Tightened `0x00420020`: every invocation scans exactly 20 voice handles from index 0, treats only raw status `1` as busy, selects the first status !=1, and steals/restarts voice 0 when all twenty are exactly 1. No round-robin cursor exists.
- Added asset-free `drone::audio` cue/queue contracts and began emitting ordered semantic events directly at proven `GameSession` call sites: rapid missile, shield pulse, special load/cycle, ordinary special launch, transient Squad flight selection and mission interstitial sound. Historical collapsed late-bomb audio flags remain transitional until per-impact ordering is represented without loss.
- Added `scripts/analyze_audio_assets.py` and `manifests/audio_asset_crosswalk.csv`. Current metadata inventory is 61 Windows WAVs and 59 DOS audio files (56 CLV, 2 WAV, 1 HMI), with 58 matched stems, four Windows-only stems and one DOS-only stem. Hash metadata exposes Windows Bomb/Missile and DOS Boss/Top1 aliases without shipping sound data.

## 2026-08-28 — Phase 5 ordered impact audio and explosion-SFX runtime

- Moved late enemy-bomb audio out of collapsed presentation booleans: the per-slot collision pass now emits the exact DirectSound order, including `probe3.wav` stop-before-impact, Probe/Stinger-specific impact pools, same-slot fallthrough to `bigexp3.wav`, and earlier-slot auto-launch before later-slot stop.
- Promoted the already-mapped `0x00402900` / `0x0042EFD8` explosion variant mechanism into portable runtime state. The process-global sequence is `explode2, explode2, explode3, explode4`, and it survives campaign/encounter resets.
- Recovered exact variant-call multiplicity at rapid/Stinger trajectory impacts, Gemini Probe/Stinger impacts, and Lid/Top top/lid lanes; the exposed-core Lid/Top Stinger destruction does not use this helper.
- Added the native boss-bomb fire cue: successful Gemini and Lid/Top spawns use the same `missile.wav` 20-voice base pool at volume 50 but preserve source/default frequency rather than forcing the player-missile 22050 Hz override.
- Increased the allocation-free per-update audio-event queue to 256 entries so multi-actor impact fanout preserves original sound-call multiplicity.

## 2026-08-28 — Phase 5 long-form playback ownership

- Exhaustively re-audited the canonical Win32 calls to `0x00406730` for effective DirectSound play flags rather than only literal pushes. Thirteen calls are proven flags-1 starts: eight literal-1 sites plus five register-propagated sites (`0x0040C8F3`, `0x0040E52F`, `0x0041A3EE`, `0x0041E298`, `0x0041E395`).
- Mapped the flags-1 assets/owners that are established in the canonical shareware executable: Bomber `bomber1.wav`, Credits `credits.wav`, Gemini `gemini.wav`, state-2/menu/interstitial `air.wav`, Drone `drone.wav`, Spidey `spidey.wav`, Lid/Top `retro1.wav`, menu `lowbees.wav`, Ordering/post-encounter `thunder2.wav`. Registered boss slot 2 (`0x0042EFE0`) remains deliberately unnamed because no canonical loader assignment was found.
- Corrected the tempting broad assumption that long-form music always loops: Results selects `hiphop/moon/suspense/choral` but calls `Play` with flags 0 at `0x0041176C`, then releases the local buffer at `0x00411C5D`.
- Recovered synchronous lifetime details for Ordering Information (`thunder2.wav`: local load -> flags-1 play -> stop/reset -> release), completion credits (`credits.wav`: local load -> flags-1 play -> 100-to-0 fade -> stop/reset -> release), and main-menu `lowbees.wav` (volume 0 -> flags-1 play -> fade toward 80 -> stop/reset/release on exit).
- Added a metadata-only loop-call catalog and extended semantic cue definitions for all four Results tracks plus Ordering/Credits. `GameSession` now emits exact start/stop ownership events across Results confirmation, Ordering completion, high-score-to-credits handoff, and credits completion.

## 2026-08-28 — Phase 5 native shareware boss loop ownership

- Promoted the two normally reachable shareware boss loops from the metadata-only flags-1 catalog into `GameSession` semantic ownership. Lid/Top activation starts `retro1.wav` (volume 70, loop flag 1); Gemini activation starts `gemini.wav` (volume 100, loop flag 1).
- Recovered playback stop ownership separately from resource release. Lid/Top stops `retro1.wav` immediately on the exposed-core Stinger transition at `0x00416C1E..0x00416C2A`, before writing lid activity 2; its established no-`0x00402900` exception is preserved.
- Recovered Gemini's paired stop rule: each threshold branch checks the other body; the first destroyed side leaves `gemini.wav` running and only the transition that leaves neither body activity 1 stops it (`0x00405773..0x00405789`, mirrored `0x00405C4A..0x00405C6B`). Existing Probe/Stinger explosion-variant calls precede that final stop, and the clean queue preserves the ordering.
- Left `air.wav`, `drone.wav`, `lowbees.wav`, registered boss loops and fade envelopes out of this slice where simple Play/Stop events would lose established volume-control semantics. Those remain the next long-form/mixer-control work rather than being approximated.

## 2026-08-28 — Phase 5 parameterized Drone loop control

- Recovered the complete shareware `drone.wav` control path from the preserved Win32 state-2 disassembly. The dedicated slot is loaded with volume 90, while the active objective starts it with flags 1 at Y=-117, stores process-global volume scalar `0x00440278 = 0`, applies volume 0 and then skips the landmark to Y=-116 (`0x0040E529..0x0040E544`).
- Recovered the pre-movement approach ramp: on phase 2 with active Drone, `-116 < Y < 45`, and scalar <80, `0x0040E4C9..0x0040E4E5` increments `0x00440278` once and sends the new value through the DirectSound volume helper before normal Drone movement.
- Recovered Probe-decoder volume ownership. Phase-1 completion sets `drone.wav` to 60 before status becomes phase 2 (`0x0040CE00..0x0040CE0B`); an enemy-bomb knockoff during attached phase 2 restores 80 before decoder reset and Probe impact audio (`0x0040F3C8..0x0040F3D7`). Successful phase-2 completion stops/rewinds the Drone slot at `0x0040CEB5..0x0040CEBE` after a separate one-shot whose asset identity remains unresolved here.
- Recovered the remaining native stop producers: the exact 4200th Y=45 hold tick (`0x0040E5B8..0x0040E5C4`), rapid-missile Drone collision (`0x0040F249..0x0040F255`) and red-Stinger Drone collision (`0x0040F69E..0x0040F6AC`). Ordinary blue-Probe attachment does not stop the loop.
- Extended the clean audio event contract with parameterized `SetVolume` and retained `0x00440278` in process-lifetime `OriginalAudioRuntimeState`. `GameSession` now emits start/ramp/decode/interruption/stop events at the already-native state transitions, with regression tests for exact value/order and both destructive weapon stops.
- Kept `air.wav`, the Y=-40 Drone one-shot, the decode-completion one-shot and menu/credits fades out of this slice where their asset/control ownership is not yet fully integrated; no filenames or fade envelopes were invented.
