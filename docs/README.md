# Drone Documentation Index

This directory is the durable engineering record for the **Drone** reverse-engineering, decompilation, clean reimplementation, and remaster project. Chat logs, screenshots, and temporary Ghidra notes are useful working material, but a result is not considered integrated until the relevant repository documentation and ledger entries have been updated.

## Start here

1. [`RE_HANDBOOK.md`](RE_HANDBOOK.md) — research method, evidence rules, naming discipline, clean implementation boundary, and phase workflow.
2. [`STATUS.md`](STATUS.md) — current project snapshot: what is confirmed, partial, unknown, and next.
3. [`ROADMAP.md`](ROADMAP.md) — phase sequence from evidence intake through fidelity, remaster, and release.
4. [`ARCHITECTURE.md`](ARCHITECTURE.md) — target clean-engine architecture and compatibility/remaster separation.
5. [`APPLY_BUILD_VERIFY.md`](APPLY_BUILD_VERIFY.md) — reproducible checkout/build/reference-bootstrap commands.

## Evidence and provenance

- [`PROVENANCE.md`](PROVENANCE.md) — exact package/executable hashes, sizes, identity, and chain-of-custody rules.
- [`EVIDENCE.md`](EVIDENCE.md) — evidence-set summary and claim-confidence vocabulary.
- [`DECOMPILATION_STRATEGY.md`](DECOMPILATION_STRATEGY.md) — binary-analysis order and dual-build comparison strategy.
- [`reverse/GHIDRA_WORKFLOW.md`](reverse/GHIDRA_WORKFLOW.md) — repeatable Ghidra import/label/export workflow.
- [`reverse/DOS_WINDOWS_CORRESPONDENCE.md`](reverse/DOS_WINDOWS_CORRESPONDENCE.md) — how the two builds are correlated.
- [`reverse/SUBSYSTEM_MAP.md`](reverse/SUBSYSTEM_MAP.md) — evolving engine subsystem map.
- [`reverse/STRUCTURE_RECOVERY.md`](reverse/STRUCTURE_RECOVERY.md) — policy and current state for recovered structures.
- [`reverse/ENTITY_LAYOUT.md`](reverse/ENTITY_LAYOUT.md) — established Win32 `0x154` ↔ DOS `0x14F` common entity correspondence, contextual overlays, combat tail, and clean-engine boundary.
- [`reverse/COLLISION.md`](reverse/COLLISION.md) — recovered collision primitives and clean semantic implementation.
- [`reverse/FRAMEBUFFER_PIPELINE.md`](reverse/FRAMEBUFFER_PIPELINE.md) — software framebuffer, DirectDraw surface, palette/text, and present path.
- [`reverse/PALETTE_EFFECTS.md`](reverse/PALETTE_EFFECTS.md) — working/base palette ownership, dynamic gameplay bands, generic animation kernel, and exact phase-sliced DirectDraw upload schedule.
- [`reverse/BITMAP_FONT.md`](reverse/BITMAP_FONT.md) — exact DOS/Win32 FONT2 glyph cache, 0x14-byte descriptor, sheet geometry, ASCII mapping, and clean extraction boundary.
- [`reverse/GAMEPLAY_EFFECTS.md`](reverse/GAMEPLAY_EFFECTS.md) — explosion/debris pools and effect/audio relationships.
- [`reverse/SPRITE_SHEETS.md`](reverse/SPRITE_SHEETS.md) — recovered JBA sprite-grid extraction and canonical frame evidence.
- [`reverse/SCORING.md`](reverse/SCORING.md) — total score, extra-life progress, penalties, threshold conversion, and HUD arithmetic quirks.
- [`reverse/HIGH_SCORES.md`](reverse/HIGH_SCORES.md) — high-score eligibility, nine-lives disqualification, ordering, insertion, and name entry.
- [`reverse/POST_GAME_FLOW.md`](reverse/POST_GAME_FLOW.md) — lives gate, Results statistics/timing, Ordering Information handoff, high-score insertion modes, final states, and completion credits.
- [`reverse/MOTHERSHIP.md`](reverse/MOTHERSHIP.md) — registered Mothership asset cluster, special-weapon core interaction, destruction sequence, and persisted completion statistic.
- [`reverse/LID_TOP_BOSS.md`](reverse/LID_TOP_BOSS.md) — reusable `lid.jba`/`top.jba` boss family, +100 destruction milestone, resources, states, and progression dispatch.
- [`reverse/BOSS_PROGRESSION.md`](reverse/BOSS_PROGRESSION.md) — six-slot pre-Drone boss dispatch and shareware reachability boundary.
- [`reverse/GEMINI_BOSS.md`](reverse/GEMINI_BOSS.md) — second shareware boss, shared 30-frame body bank, head entities, audio, and destruction behavior.
- [`reverse/REGISTERED_BOSS_SLOT2.md`](reverse/REGISTERED_BOSS_SLOT2.md) — dormant registered encounter with surviving combat/destructible-pixel behavior but unresolved asset identity.
- [`reverse/SPIDEY_BOSS.md`](reverse/SPIDEY_BOSS.md) — registered Spidey resource/update/destruction reconstruction.
- [`reverse/BOMBER_BOSS.md`](reverse/BOMBER_BOSS.md) — registered Bomber resource/update/destruction reconstruction.
- [`reverse/WORLD_SCENERY.md`](reverse/WORLD_SCENERY.md) — three-screen scrolling scenery buffer, river/desert/registered scenery families, boss selection boundary, and two-level shareware gate.

Machine-readable research ledgers live under `reverse/ledger/`, `reverse/correspondence/`, and `reverse/structures/`. In particular, `reverse/ledger/functions.csv` and `reverse/ledger/globals.csv` are the richer cross-platform research ledgers; the platform-local maps remain compact address sheets useful during disassembly.

## Formats and installer reconstruction

- [`formats/JBA.md`](formats/JBA.md)
- [`formats/CLV.md`](formats/CLV.md)
- [`formats/FLY.md`](formats/FLY.md)
- [`formats/DEMO_DAT.md`](formats/DEMO_DAT.md)
- [`formats/SCORES.md`](formats/SCORES.md)
- [`formats/WISE_INSTALLER.md`](formats/WISE_INSTALLER.md)
- [`ASSET_CATALOG.md`](ASSET_CATALOG.md)

## Behavioral reconstruction and validation

- [`ORIGINAL_BEHAVIOR.md`](ORIGINAL_BEHAVIOR.md) — currently established black-box game behavior.
- [`reverse/PLAYER.md`](reverse/PLAYER.md) — executable-backed player entity, movement, banking, bounds, respawn, and documentation discrepancy.
- [`reverse/INPUT.md`](reverse/INPUT.md) — DOS/Win32 physical-input normalization, six-channel replay substitution, and portable semantic `GameplayInputFrame`.
- [`reverse/RAPID_MISSILES.md`](reverse/RAPID_MISSILES.md) — Ctrl rapid-fire pool, asset, cooldown, update, cleanup, collision, and rendering chain.
- [`reverse/ENEMY_BOMBS.md`](reverse/ENEMY_BOMBS.md) — bomb pool, live/replay spawn differences, movement and lifetime.
- [`reverse/SPECIAL_WEAPONS.md`](reverse/SPECIAL_WEAPONS.md) — Probe/Stinger entity, input, homing, and Probe decode/disarm lifecycle.
- [`reverse/DEMO_REPLAY.md`](reverse/DEMO_REPLAY.md) — 14-channel hybrid deterministic replay system and DOS↔Windows shared test vectors.
- [`TESTING_VALIDATION.md`](TESTING_VALIDATION.md) — synthetic tests, binary-reference tests, trace comparison, framebuffer/audio checks, and parity gates.
- [`TIMING.md`](TIMING.md) — timer evidence and unresolved simulation-cadence work.
- [`COMPATIBILITY_REMASTER_POLICY.md`](COMPATIBILITY_REMASTER_POLICY.md) — rules preventing remaster features from changing reconstructed behavior.
- [`OPEN_QUESTIONS.md`](OPEN_QUESTIONS.md) — human-readable queue of unresolved technical questions.
- [`RESEARCH_LOG.md`](RESEARCH_LOG.md) — chronological record of major discoveries and decisions.

## Port and release planning

- [`PLATFORM_PLAN.md`](PLATFORM_PLAN.md) — Linux, macOS, iPadOS, and Windows host strategy.
- [`PUBLISHING.md`](PUBLISHING.md) — repository/data separation and public-distribution boundaries.
- [`LICENSE_AND_RIGHTS.md`](LICENSE_AND_RIGHTS.md) — source-license decision status and original-game rights boundary.
- [`CONTRIBUTING.md`](../CONTRIBUTING.md) — contribution and evidence requirements.

## Milestones

- [`PHASE1.md`](PHASE1.md) — Phase 1 completed work and Phase 2 handoff.
- [`PHASE1_DOCUMENTATION_HARDENING.md`](PHASE1_DOCUMENTATION_HARDENING.md) — documentation/ledger hardening applied before Phase 2.
- [`PHASE2.md`](PHASE2.md)
- [`PHASE3.md`](PHASE3.md) — current gameplay-reconstruction findings, validation state, and remaining Phase 2 work.

Every future phase should add or update its own milestone document and update `STATUS.md`, `ROADMAP.md`, `RESEARCH_LOG.md`, and the relevant machine-readable ledgers in the same commit.

- [`FRAMEBUFFER_VALIDATION.md`](FRAMEBUFFER_VALIDATION.md) — copyright-safe indexed framebuffer snapshot, comparison, and hash-metadata workflow.
