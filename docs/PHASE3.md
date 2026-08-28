# Phase 3 — Rendering & World Reconstruction

**Status:** complete.

Phase 3 turns the Phase-2 gameplay/fidelity primitives into a documented original presentation pipeline. It does not broaden into complete game simulation; that belongs to Phase 4. The goal is to make original visual output reproducible enough for framebuffer-level comparison while keeping presentation separate from `drone_core` gameplay rules.

## Phase-2 rendering baseline inherited

Already established before Phase 3 begins:

- 320×200 indexed framebuffer and RGB6 palette conversion;
- original transparent sprite blitter and sprite-sheet extraction geometry;
- 320×600 cyclic world/scenery compositor;
- scaled transparent sprite path;
- FONT2 64-glyph mask cache semantics;
- particle/sprite-debris update boundaries and several effect render anchors;
- simulation → presentation → host stage ordering;
- native X11, Win32/GDI, and Cocoa/CoreGraphics fidelity-host source backends.

These contracts are inputs to Phase 3, not work to rediscover.

Current reconstruction also includes a detailed evidence-backed decomposition of the old world-sprite batches into boss composites, direct point-particle writes, the Gemini procedural effect, explosion/debris routing, trajectory/projectile/player layers, and late impact sprites; see [`reverse/WORLD_PRESENTATION_SUBPASSES.md`](reverse/WORLD_PRESENTATION_SUBPASSES.md).

## Phase-3 closure result

1. **Dynamic palette system — COMPLETE.** `Q-RENDER-001` is resolved: base/working palette ownership, startup fade-in, generic and purpose-built animators, and exact DirectDraw range-upload scheduling are clean-tested.
2. **Small-JBA format — COMPLETE.** `Q-JBA-002` is resolved for the canonical Windows small family without inventing a runtime owner.
3. **Layering/composition — COMPLETE for Phase-3 scope.** The ordinary state-2 path is a corrected 19-pass presentation contract. The world/effect region is decomposed into evidence-backed subpasses, scaled explosion/debris routing is explicit, the startup fade is restored to its correct position, and HUD/special/shield/outcome-cursor semantics are clean-tested.
4. **Reference framebuffer validation — COMPLETE as tooling/boundary.** `DRONEFB1`, exact/region comparisons, metadata fingerprints and local-only capture workflow make clean output suitable for original-runtime comparison without checking proprietary captures into Git. Obtaining and driving exact original-runtime trace parity remains Phase 6 validation work.
5. **Native host validation — COMPLETE for Phase-3 requirement.** Linux/X11 accepts the shared indexed-framebuffer snapshot contract and has automated display-free capture/round-trip validation. Production hardening across all targets remains Phase 12.

**Exit achieved:** the clean project now has an evidence-backed original rendering/world pipeline capable of deterministic composition from clean state and suitable for framebuffer comparison. Remaining open research questions concern complete simulation, retail-only content, historical authoring/timing provenance or later exact-parity work; none requires keeping renderer/world architecture in Phase 3.

## Non-goals

- Completing every enemy, boss, projectile, and mission state machine (Phase 4).
- Full deterministic gameplay/frame/audio trace parity (Phase 6).
- Registered-only world/content reconstruction without a canonical lawful retail evidence set (Phase 8).
- Modern high-resolution presentation or UX redesign (Phases 11–14).

## Closure boundary

Phase 3 closes on renderer/world **architecture and deterministic comparison readiness**, not on perfect original-runtime trace equality. Exact state/frame/audio trace parity is explicitly Phase 6, end-to-end shareware discrepancy closure is Phase 7, registered-only world content is Phase 8, and production platform hardening is Phase 12. This prevents later validation/content work from artificially holding the renderer-reconstruction phase open.
