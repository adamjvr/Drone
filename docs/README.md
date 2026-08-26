# Drone Documentation Index

This directory is the durable engineering record for the **Drone** reverse-engineering, decompilation, clean reimplementation, and remaster project. Chat logs, screenshots, and temporary Ghidra notes are useful working material, but a result is not considered integrated until the relevant repository documentation and ledger entries have been updated.

## Start here

1. [`RE_HANDBOOK.md`](RE_HANDBOOK.md) — research method, evidence rules, naming discipline, clean implementation boundary, and phase workflow.
2. [`STATUS.md`](STATUS.md) — current project snapshot: what is confirmed, partial, unknown, and next.
3. [`ROADMAP.md`](ROADMAP.md) — milestone sequence and exit criteria from evidence intake through release.
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

Machine-readable research ledgers live under `reverse/ledger/`, `reverse/correspondence/`, and `reverse/structures/`. In particular, `reverse/ledger/functions.csv` and `reverse/ledger/globals.csv` are the richer cross-platform research ledgers; the platform-local maps remain compact address sheets useful during disassembly.

## Formats and installer reconstruction

- [`formats/JBA.md`](formats/JBA.md)
- [`formats/CLV.md`](formats/CLV.md)
- [`formats/FLY.md`](formats/FLY.md)
- [`formats/DEMO_DAT.md`](formats/DEMO_DAT.md)
- [`formats/WISE_INSTALLER.md`](formats/WISE_INSTALLER.md)
- [`ASSET_CATALOG.md`](ASSET_CATALOG.md)

## Behavioral reconstruction and validation

- [`ORIGINAL_BEHAVIOR.md`](ORIGINAL_BEHAVIOR.md) — currently established black-box game behavior.
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

Every future phase should add or update its own milestone document and update `STATUS.md`, `ROADMAP.md`, `RESEARCH_LOG.md`, and the relevant machine-readable ledgers in the same commit.
