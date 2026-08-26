# Development Conventions

## Scope

These conventions apply to the clean implementation, analysis tools, and durable RE metadata. They are intentionally conservative while original behavior is still being recovered.

## Language and build

- Clean engine/tooling code: C++20 unless a phase deliberately introduces another language.
- Analysis/bootstrap utilities: Python 3 where it reduces friction.
- CMake is the source of truth for C++ target composition.
- Production code should compile with warnings enabled; new warnings should be fixed rather than globally suppressed.

## Integer and binary-data rules

- Use fixed-width integer types (`std::uint8_t`, `std::int16_t`, etc.) when width is part of a recovered format/behavior.
- Do not use C++ `char` when signedness matters.
- Keep file-format parsing explicit about sizes and bounds.
- Preserve integer truncation/overflow-sensitive semantics until parity testing proves an abstraction is safe.
- Do not replace recovered integer/fixed-point game math with floating point solely for convenience.

## Naming unknowns

Unknown values are allowed and expected.

Preferred forms:

```text
field0
unknown_0x14
game_state_raw
state_ui_update_candidate
```

Avoid speculative semantic names that become difficult to unwind later.

## Platform boundary

`drone_core` must not directly depend on:

- Win32/DirectX;
- DOS/DOS4GW/HMI;
- Cocoa/UIKit;
- SDL or another host library unless a narrowly defined abstraction decision explicitly changes this rule.

Platform/host layers translate OS facilities into core contracts.

## Error handling

Analysis tools should fail loudly on malformed/unexpected input instead of silently guessing. For canonical evidence operations, verify hashes before relying on hard-coded offsets or addresses.

## Tests

Any newly confirmed format transform or pure gameplay rule should receive a synthetic test when practical. Bugs discovered through reference comparison should get a regression test before/with the fix.

## Source comments

Comments should explain:

- recovered behavioral constraints;
- why a strange integer/truncation/order is intentional;
- evidence IDs or docs when useful;
- host/core boundary rationale.

Do not paste long decompiler output into comments.

## Research metadata

CSV ledgers should use stable IDs. Once an ID is published, do not recycle it for a different finding/question/structure even if the original is rejected.

## Commits

A reverse-engineering commit that changes established knowledge should update code, tests, and docs together where possible. Good commits tell a future reader both **what changed** and **why we believe it**.

Example commit subjects:

```text
Document Win32 input aggregation path
Recover FLY field0 as trajectory X delta
Add fidelity framebuffer host shell
Validate DOS timer cadence against Win32 build
```
