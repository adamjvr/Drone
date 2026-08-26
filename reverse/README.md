# Reverse-Engineering Working Tree

`reverse/` contains durable, publishable metadata and independently written analysis about the original Drone binaries. It deliberately excludes the original binaries and bulk decompiler/disassembly exports.

- `windows/` — canonical Win32 shareware addresses and control-flow notes.
- `dos/` — canonical DOS shareware addresses and Watcom/DOS4GW notes.
- `ghidra/` — scripts that recreate stable labels in a local analysis project.
- `correspondence/` — DOS↔Windows cross-build relationships.
- `structures/` — recovered layout/field evidence.
- `ledger/` — findings and unresolved-question tracking.

Read `docs/RE_HANDBOOK.md` before promoting hypotheses to established names.
