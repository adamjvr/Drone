# Reverse-Engineering Handbook

## 1. Purpose

The **Drone** project has two related but distinct engineering goals:

1. reconstruct the original game as accurately as practical from lawful reference copies of the DOS and Windows releases; and
2. build a maintainable, portable remaster for Linux, macOS, iPadOS, and Windows without making the modern codebase depend on DOS/Win32 implementation accidents.

A visually similar rewrite is not enough. The project is intended to preserve the archaeological record: what the original executables do, how that conclusion was reached, how confident we are, and how the clean implementation was validated.

This handbook defines the working rules that keep those goals compatible.

## 2. Repository zones

The repository is divided by purpose.

### `reverse/` — evidence-facing research

This tree may contain:

- original virtual addresses and offsets;
- short disassembly observations;
- function/global maps;
- structure hypotheses;
- DOS↔Windows correspondence tables;
- Ghidra scripts;
- small research pseudocode written to explain an observed algorithm;
- confidence/status ledgers.

It should **not** contain original proprietary binaries or bulk dumps of original code/data.

### `src/`, `include/` — clean implementation

This is maintainable project code. Code enters this tree only after behavior or a data contract is understood well enough to specify independently. Do not paste decompiler output into production source. Re-express the behavior in the project's own naming, types, tests, and architecture.

### `tools/`, `scripts/` — reproducible analysis and conversion

Tools should turn one-off discoveries into repeatable operations: unpacking known installers, inventorying evidence, decoding formats, producing comparison artifacts, and eventually collecting deterministic traces.

### `.reference/` — local original evidence

This directory is ignored by Git. It contains user-supplied original packages, extracted installations, local Ghidra projects if desired, screenshots/traces, and other material that should not be published with the repository.

## 3. Evidence hierarchy

Claims should state a confidence level. Confidence measures the strength of evidence for a specific claim, not how plausible the name sounds.

### Confirmed / high confidence

Use when one or more of the following apply:

- the exact behavior is directly visible in the binary and unambiguous;
- independent DOS and Windows implementations agree;
- a recovered algorithm reproduces original files byte-for-byte or pixel/sample-for-pixel/sample;
- an original document describes the behavior and runtime testing agrees;
- a controlled trace establishes the state transition or data contract.

### Medium confidence

Use when the code/data relationship is strong but one important semantic detail remains inferred. Example: a global is clearly the current framebuffer pitch because DirectDraw writes it and blitters consume it, while a few secondary uses remain unmapped.

### Low confidence / hypothesis

Use when a claim is useful for navigation but not established. Hypotheses must use provisional names and must not silently migrate into public API names or production architecture.

### Rejected

When evidence disproves a hypothesis, retain enough of the record to prevent rediscovery. Mark the old claim rejected/superseded in the relevant ledger or research log rather than simply deleting history.

## 4. Finding lifecycle

Every meaningful RE result should move through this sequence:

1. **Observation** — record address/offset/file and what is literally present.
2. **Hypothesis** — if needed, propose a semantic interpretation with low confidence.
3. **Cross-reference** — inspect callers/callees, data consumers, asset names, and the other platform build.
4. **Experiment** — decode, patch locally, trace, or compare reference outputs where useful.
5. **Specification** — document the established behavior without compiler-specific clutter.
6. **Clean implementation** — implement the specification in `drone_core` or a host layer.
7. **Validation** — prove the clean implementation matches the established reference behavior.
8. **Promotion** — update names/confidence in ledgers and Ghidra labels.

A discovery is not considered finished just because Ghidra has a pretty function name.

## 5. Binary identity before analysis

Never assume that two files called `Drone_sw.exe` are identical. Before importing or comparing a binary:

- calculate SHA-256;
- record byte size;
- record executable format and architecture;
- for PE, record image base/linker generation/timestamp when available;
- identify the source package/install path;
- compare against `docs/PROVENANCE.md` and `manifests/reference_binaries.sha256`.

Addresses in this repository are valid only for the exact hashed binary listed with them unless explicitly documented otherwise.

## 6. Ghidra naming rules

Prefer names that expose certainty:

- `load_fullscreen_jba` — acceptable when the algorithm and callers establish the function;
- `state_ui_update_candidate` — acceptable provisional semantic name;
- `game_state_raw` — preferred over `GameMode` while the protocol contains unexplained values;
- `unknown_0x34` / `field2` — preferred for unresolved structure fields.

Do not name a field `x`, `health`, `enemy_type`, or similar merely because the values look plausible in one sample.

When a name changes materially, update:

- the relevant CSV ledger;
- Ghidra labeling scripts;
- prose docs that use the old name;
- clean code only if the old name had already crossed into implemented code.

## 7. Function recovery procedure

For a candidate function:

1. record platform and exact entry address;
2. determine whether it is compiler/runtime/library/platform glue or game code;
3. inspect direct callers and callees;
4. identify global reads/writes;
5. identify literal strings/assets/constants;
6. identify loops and array strides;
7. search for an equivalent algorithm in the other build;
8. write a short behavioral summary;
9. assign confidence;
10. add it to the function map and, when useful, the DOS↔Windows correspondence table.

Large functions should be decomposed by **behavioral regions**, not arbitrarily by decompiler output. A giant update routine may initially remain one binary function while the clean implementation becomes several subsystems.

## 8. Structure recovery procedure

Recover structures from access patterns, not desired architecture.

For each candidate structure:

- establish allocation size or array stride;
- collect every observed offset and access width;
- distinguish pointer, integer, byte flag, and floating/fixed-point operations where possible;
- find initialization and destruction paths;
- find iteration limits/sentinels;
- compare parallel DOS/Windows layouts;
- add offsets to `reverse/structures/fields.csv` before assigning semantic names.

It is acceptable for a recovered structure to remain `UnknownEntity20` with `unknown_00`, `unknown_04`, etc. for several phases.

The modern engine does **not** have to preserve original memory layout unless layout itself affects file compatibility or behavior.

## 9. Data-format recovery procedure

A format is considered structurally confirmed when the loader/writer behavior and corpus agree. Whenever possible:

- describe byte/record layout;
- identify integer width/signedness/endian behavior;
- document valid sizes/count invariants;
- reconstruct conversion algorithm;
- provide a synthetic regression fixture;
- compare clean decoder output against original behavior or a second build.

Unknown fields remain unknown even if their distribution suggests a meaning.

## 10. DOS↔Windows comparison

The two releases are treated as independent witnesses to shared game behavior.

A correspondence entry may be based on:

- identical asset format algorithms;
- common strings/load order;
- matching constants;
- equivalent call graph position;
- matching structure stride/access pattern;
- matching black-box behavior.

Compiler/runtime-specific routines are explicitly excluded from gameplay equivalence. DirectDraw/DirectSound/DirectInput and HMI/DOS/VGA code should become platform contracts, not be cloned into the clean simulation.

## 11. Clean implementation boundary

The clean implementation is a behavioral reconstruction, not a prettified decompiler listing.

Before implementing a reconstructed feature, answer:

- What is the observable contract?
- Which details are known to affect behavior?
- Which details are merely original compiler/platform mechanics?
- What deterministic test proves parity?
- Is the feature part of the fidelity core or a host/remaster layer?

Compiler temporaries, original stack layout, original class organization, and incidental buffer ownership should not be reproduced unless they have observable consequences.

## 12. Behavioral validation

The strongest long-term validation target is deterministic comparison against the originals. Planned evidence includes:

- same input stream → same player/entity state at defined ticks;
- same demo data → same progression/events;
- framebuffer region or full-frame hashes at checkpoints;
- palette update comparisons;
- score/lives/shield transitions;
- audio event IDs/timestamps and decoded sample comparisons;
- level-script/FLY progression;
- collision outcomes at boundary cases.

See `docs/TESTING_VALIDATION.md`.

## 13. Compatibility versus remaster features

The compatibility core establishes the rules. Remaster presentation may improve scaling, interpolation, artwork, UI, input ergonomics, accessibility, save handling, and audio presentation **without changing simulation results** unless an explicitly labeled optional rule change is introduced.

Original 320×200 indexed rendering remains a reference path even after a high-resolution renderer exists.

## 14. Phase documentation contract

Every milestone commit should update, as applicable:

- `docs/STATUS.md`;
- `docs/ROADMAP.md`;
- the phase document;
- `docs/RESEARCH_LOG.md`;
- function/global/structure/correspondence ledgers;
- format specifications;
- `docs/OPEN_QUESTIONS.md`;
- tests and validation notes;
- Ghidra label scripts.

A phase ZIP is not complete if the code moved forward but the durable research record did not.

## 15. What not to commit

Do not commit:

- original game executables;
- original Pixelsplash images/audio/levels/data;
- installer payloads;
- bulk decompiler/disassembly exports copied from proprietary binaries;
- local Ghidra project databases;
- screenshots or decoded assets unless their redistribution status has been independently cleared.

Hashes, offsets, names, small factual metadata, independently written specifications, and original project code belong in the repository.

## 16. Current primary research priorities

At the end of Phase 1 the highest-value unresolved tasks are:

1. determine intended simulation cadence by recovering the DOS timer path and validating the Win32 15,000-QPC-count limiter;
2. split and classify the state-2 gameplay region rooted at `0x0040BA50`;
3. recover entity pools/structure offsets and update order;
4. identify input aggregation and player controls;
5. recover FLY field semantics from consumers;
6. recover demo DAT field semantics from playback/recording consumers;
7. identify software blitters/HUD paths and framebuffer semantics;
8. use these contracts to create the first modern fidelity host.
