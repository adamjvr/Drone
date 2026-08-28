# Phase 3 — Rendering & World Reconstruction

**Status:** in progress.

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

## Current Phase-3 work

1. **Late dynamic-palette classification — COMPLETE.** `Q-RENDER-001` is resolved: the generic and purpose-built palette animators, their initialization state, DirectDraw range-upload primitive, and exact settled phase-sliced upload ranges are clean-tested. See [`reverse/PALETTE_EFFECTS.md`](reverse/PALETTE_EFFECTS.md).
2. **Small-JBA container completion — COMPLETE.** `Q-JBA-002` is resolved from all three canonical Windows members: byte-sized opaque preamble length, PCX at `1+N`, exact 128×128 single-plane 8-bit RLE, and markerless raw RGB8 palette trailer. The clean decoder remains separate from the 320×200 JBA path; canonical game-runtime ownership is not asserted.
3. **Layering and composition — IN PROGRESS.** The ordinary state-2 presentation chain is now an explicit 18-pass contract from world composition through sprite/effect batches, scaled overlays, HUD/shield/status layers, palette animation/upload and final present. Remaining work is finer per-actor HUD/effect ownership. See [`reverse/PRESENTATION_ORDER.md`](reverse/PRESENTATION_ORDER.md).
4. **Reference framebuffer fixtures — TOOLING COMPLETE / CAPTURES PENDING.** `DRONEFB1`, `drone_framecheck`, SHA-256 metadata tooling, region comparisons, and public empty manifest schemas are implemented and clean-tested. No proprietary original-runtime frame is committed; lawful reference captures remain a local evidence task. See [`FRAMEBUFFER_VALIDATION.md`](FRAMEBUFFER_VALIDATION.md).
5. **Native host validation.** Preserve the shared indexed-framebuffer boundary while validating presentation behavior on Linux first and later macOS/Windows; iPadOS production hosting remains a Phase-12 platform-completion concern.

## Non-goals

- Completing every enemy, boss, projectile, and mission state machine (Phase 4).
- Full deterministic gameplay/frame/audio trace parity (Phase 6).
- Registered-only world/content reconstruction without a canonical lawful retail evidence set (Phase 8).
- Modern high-resolution presentation or UX redesign (Phases 11–14).

## Exit direction

Phase 3 should leave the project with an evidence-backed original rendering/world pipeline capable of deterministic composition from clean simulation state and suitable for reference framebuffer comparison. Exact Phase-3 closure will be recorded in the roadmap when those renderer/world contracts are complete rather than inferred from a fixed number of milestones.
