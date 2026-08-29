# Drone Master Roadmap

The project order is **decompile → understand → reproduce → validate → preserve → enhance**. A phase may contain many parallel workstreams, but the roadmap intentionally has **no numbered subphases**.

Every phase updates code, evidence ledgers, tests, and durable documentation together.

## Phase 0 — Evidence & Preservation — COMPLETE

Preserve/hash DOS and Windows releases, extract the Wise installer without execution, inventory the corpus, establish provenance, and keep original payloads outside Git.

## Phase 1 — Reverse-Engineering Foundation — COMPLETE

Build reproducible analysis tooling, recover initial formats, map executable/runtime anchors, establish Ghidra/ledger/documentation workflows, and create the clean C++20 foundation. Phase 1 is the canonical baseline merged into `main`.

## Phase 2 — Gameplay Reconstruction — COMPLETE

Recover the real gameplay architecture: state orchestration, entity/object layouts, DOS/Windows timing, input representation, FLY trajectories, movement/update paths, collision/projectile boundaries, scrolling/level behavior, and the first native fidelity host. At least one meaningful gameplay subsystem must execute in clean code with reference validation before this phase closes.

**Exit achieved:** the user-facing state protocol, canonical DOS cadence, semantic input boundary, Win32/DOS common entity family, trajectory lifecycle/templates, collision/projectile boundaries, scrolling/mission progression, post-game control flow, native fidelity host, and multiple clean gameplay subsystems are recovered and tested. No unresolved `critical` research question remains. Later renderer completeness, complete-game simulation, deterministic trace parity, and retail-only behavior stay assigned to their roadmap phases rather than keeping Phase 2 artificially open.

## Phase 3 — Rendering & World Reconstruction — COMPLETE

Recover sprite/frame loading, animation, clipping, layering, scrolling, palettes, HUD, text, effects, scenery composition, and reference framebuffer comparison.

**Exit achieved:** the original Win32 presentation path is represented as a corrected 19-pass semantic contract; detailed world/effect and scaled-overlay ownership, startup palette fade, HUD/special/shield presentation, JBA families, cyclic scenery composition, deterministic framebuffer snapshots/comparison tooling, and a validated Linux capture boundary are in clean code/tests. Exact original-runtime trace parity remains Phase 6 rather than blocking renderer architecture closure.

## Phase 4 — Complete Game Simulation — COMPLETE

Reconstruct player/enemy/projectile simulation, AI, collision, damage, scoring, lives, pickups, special weapons, scripted events, bosses, death/restart, level progression, and game-over behavior.

**Exit achieved:** the canonical shareware simulation now executes continuously through one portable `GameSession`: player/weapon/shield/bomb state, exact Probe attachment/decode/disarm, trajectory population/movement/collisions/accounting, both shareware boss combat state machines, normal/destructive Drone progression, native player-death lifecycle and respawn, update-side detonation RNG, mission transitions, Results/Ordering/high-score/credits control flow, and the processed-count-dependent attached-Probe bomb steering policy are owned without calling the original executable. Remaining mutable hostile candidates are registered-progression content assigned to Phase 8. Immutable sprite/path data remains an asset boundary. Render/audio presentation RNG and exact runtime trace parity are fidelity work for Phases 5–7, not blockers for complete simulation architecture.

## Phase 5 — Audio Reconstruction — IN PROGRESS

Recover sound-event mapping, playback priority, voice/channel behavior, panning, volume, loops, music if applicable, and DOS HMI versus Windows DirectSound behavioral differences behind a portable audio interface.

**Current progress:** the asset-free audio runtime now owns exact DirectSound primitives, the original 20-voice reuse/steal policy, ordered impact-event multiplicity, process-global explosion-SFX phase, Results/Ordering/Credits ownership including the exact 99..0 credits fade, all 13 proven flags-1 start sites as metadata, native Lid/Top/Gemini encounter-loop start/stop events, the exact every-eighth-traversal `level1.wav`/`level2.wav` one-shot cadence, the parameterized `drone.wav` Y=-117 start/ramp/decode/interruption/stop controls, the gameplay-owned `air.wav` +1/-1 settlement envelope/detonation/restart, and a separate host runtime for `lowbees.wav`, the menu 11025-Hz air restart and states-5/6/99 air fade-to-stop/resume behavior. The formerly unresolved Drone one-shots are now native as `hintdron.wav` at Y=-40 and `parachut.wav` immediately before the decode-completion Drone-loop stop. Static trajectory initialization is now closed too: every `squad1.wav`..`squad14.wav` family is proven as a 20-voice pool at volume 80 with no initialization frequency override. The DOS HMI **middleware** contract is now represented separately: public S.O.S. 4.x evidence establishes descriptor-backed sample start plus volume/rate/pan/loop/priority capability and a 32-voice library ceiling, while explicitly leaving Drone DOS configured voices and arbitration/lifecycle policy unresolved. Remaining Phase-5 work is executable-level DOS HMI call-site recovery followed by the portable mixer/backend.

## Phase 6 — Deterministic Replay & Validation

Decode demo semantics, create deterministic playback/state snapshots, compare simulation/framebuffer/audio traces against the originals, and turn those comparisons into regression tests.

## Phase 7 — Shareware Parity

Run the supplied shareware content end-to-end on the clean engine without relying on the original executable. Known behavioral discrepancies should approach zero and be explicitly documented.

## Phase 8 — Full-Game Reconstruction

After a lawfully obtained full release is selected and hashed as a canonical evidence set, recover registered-only levels, enemies, bosses, assets, ending behavior, and executable/content differences.

## Phase 9 — Decompilation Completion

Drive the archaeology toward near-complete semantic coverage. Every meaningful routine/global/structure should be classified, documented, correlated across builds where possible, and linked to clean behavior/tests.

## Phase 10 — Fidelity Release

Produce a historically faithful compatibility implementation preserving validated timing, logical resolution, palette behavior, mechanics, and original-data import workflow.

## Phase 11 — Remaster Engine

Add a separate modern rendering/presentation layer while keeping the validated simulation core and fidelity renderer intact.

## Phase 12 — Platform Completion

Harden production hosts for **Linux, macOS, Windows, and iPadOS** around the shared `drone_core` simulation/data contracts.

## Phase 13 — Modern UX

Add controller/touch support, remapping, display/audio settings, scalable UI, accessibility, modern pause/settings behavior, and quality-of-life features without altering fidelity rules.

## Phase 14 — Remastered Content

Add optional high-resolution graphics, enhanced effects/animation/audio, and other remastered presentation/content while preserving fidelity mode.

## Phase 15 — Packaging & Release

Finalize installers/packages, asset verification/import, CI/release automation, signing/notarization where applicable, rights/license decisions, public developer documentation, and distribution.

## Architectural invariant

The remaster is not the reverse-engineering target. `drone_core` must remain capable of validated original behavior independently of modern presentation features. Platform and remaster layers consume the reconstructed core; they do not redefine it.
