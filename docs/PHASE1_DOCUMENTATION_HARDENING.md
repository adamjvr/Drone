# Phase 1 Documentation Hardening Patch

## Why this patch exists

The first Phase 1 delivery contained enough notes to resume reverse engineering, but it did not yet meet the standard required for a long-term public decompilation/remaster project. Important facts were documented, while methodology, provenance, confidence lifecycle, validation gates, structure/correspondence tracking, and future-platform rules were too implicit.

This patch makes documentation part of the engineering system rather than a retrospective narrative.

## Added durable documentation

- master reverse-engineering handbook;
- canonical evidence provenance/identity table;
- complete documentation index;
- explicit current status matrix;
- testing and behavioral parity strategy;
- compatibility-versus-remaster policy;
- Linux/macOS/iPadOS/Windows platform plan;
- development conventions;
- source-license/original-rights decision record;
- Ghidra import/naming/export workflow;
- DOS↔Windows correspondence method;
- engine subsystem map;
- structure-recovery procedure;
- open-question queue;
- chronological research log;
- known Windows Wise installer stream-format specification;
- milestone documentation template.

## Added machine-readable research state

- complete metadata-only DOS corpus manifest (187 files);
- complete metadata-only Windows installed corpus manifest (192 files);
- richer cross-platform function ledger;
- richer global ledger;
- findings ledger with stable IDs;
- open-question ledger with stable IDs/priorities;
- DOS↔Windows correspondence table;
- structure/field ledgers.

## Documentation regression checks

`scripts/check_research_docs.py` is now included in CTest when Python 3 is available. It validates:

- local Markdown links;
- unique stable IDs in research ledgers;
- expected corpus-manifest row counts and SHA-256 syntax;
- known Wise stream count (207) and installed-path count (192).

This gives documentation/research metadata the same basic anti-regression treatment as code.

## Behavioral documentation improvement

`ORIGINAL_BEHAVIOR.md` now captures the supplied v1.01 shareware manual as a black-box specification, including controls, targeting/probe/stinger rules, HUD states, scoring, joystick behavior, the nine-lives/high-score cheat, and the documented Tab “vertical retrace sync” toggle.

The Tab behavior is now explicitly tied to a Phase 2 timing question: the Win32 executable has a confirmed pacing guard, but the project does **not** yet claim the guard global is the same user setting until the input writer is mapped.

## Validation

This patch was validated with:

```text
python3 scripts/check_research_docs.py
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

At packaging time both the synthetic format test and research-metadata test pass.

## Ongoing rule

Every future reverse-engineering phase must patch documentation **as discoveries are made**. Stable findings should never depend on reconstructing old chat context.
