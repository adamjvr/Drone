# Contributing to Drone

Drone is an evidence-driven reverse-engineering project. A contribution is valuable not only when it produces working code, but when another engineer can understand **which original behavior it represents and why the claim is credible**.

## Before contributing

Read:

- `docs/RE_HANDBOOK.md`;
- `docs/DEVELOPMENT_CONVENTIONS.md`;
- `docs/PROVENANCE.md`;
- `docs/TESTING_VALIDATION.md`.

The project source license/inbound contribution policy has **not yet been selected**. Public contribution solicitation should wait until `docs/LICENSE_AND_RIGHTS.md` is resolved.

## Evidence requirements

Reverse-engineering claims should identify their evidence and confidence. Prefer, in descending strength:

1. exact binary address/file offset and unambiguous instruction/data behavior;
2. independent DOS/Windows correspondence;
3. controlled runtime trace or exact output reproduction;
4. original supplied documentation corroborated by runtime behavior;
5. explicitly labeled hypothesis.

Do not turn plausible guesses into established terminology.

## Naming

Use `fieldN`, `unknown_0xNN`, `*_raw`, or `*_candidate` while semantics are unresolved. Rename only when producers/consumers, cross-build evidence, or controlled behavior establish the meaning.

## Code boundary

- `reverse/` records evidence and independently written analysis.
- `src/` and `include/` contain maintainable clean implementation.
- Do not paste bulk decompiler output into production source or documentation.
- Do not commit original Pixelsplash binaries/assets; keep them in `.reference/`.

## Required documentation updates

A contribution that materially changes RE knowledge should update the appropriate ledgers/specs in the same change. Common targets:

- platform `function_map.csv` / `global_map.csv`;
- `reverse/ledger/findings.csv`;
- `reverse/ledger/open_questions.csv`;
- DOS↔Windows correspondence table;
- structure/field ledgers;
- format/subsystem docs;
- `docs/RESEARCH_LOG.md` and `docs/STATUS.md` for milestone-level discoveries.

## Validation

New confirmed pure transforms/game rules should receive synthetic regression tests where practical. If a change is based on local original-data comparison, describe the comparison while keeping the proprietary fixture outside Git.
