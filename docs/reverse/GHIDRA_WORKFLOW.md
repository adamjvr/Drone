# Ghidra Workflow

## Purpose

This procedure keeps local Ghidra projects reproducible without committing proprietary binaries or Ghidra database files.

## Canonical Win32 binary

Use only the executable whose identity matches `BIN-WIN-SW-01` in `docs/PROVENANCE.md` for the Phase 1 Win32 addresses.

Expected SHA-256:

```text
4fffc0406157b0def539b619fa53d1bb6c59537a44a6551df3aa3c060eaf3784
```

Expected local path after bootstrap:

```text
.reference/work/windows/Drone_sw.exe
```

## Import

1. Create a **local** Ghidra project outside tracked repository content or in a gitignored workspace.
2. Import the canonical `Drone_sw.exe` as PE32/x86 using the executable's normal image base (`0x00400000`).
3. Allow standard x86 analysis and PE import/thunk processing to run.
4. Preserve imported API names; they are valuable anchors for DirectX, Win32, WinMM, and C runtime boundaries.
5. Apply `reverse/ghidra/DroneWin32Labels.py` after initial analysis.

The labeling script is deliberately conservative: high-confidence functions/globals receive names; low-confidence candidates remain primarily in CSV/prose until stronger evidence exists.

## Canonical DOS binary

Expected SHA-256:

```text
e7de54f9cd158289df58a7eee4ecbe6f1b9b04972e7eae88b26bfe32515a0fc0
```

Expected local path:

```text
.reference/work/dos/DRONE_SW.EXE
```

The DOS binary is a Watcom C/C++ Linear Executable using DOS/4GW. Its current addresses and labels are recorded in `reverse/dos/function_map.csv`. Preserve the analysis mapping used by those recorded linear addresses; if a different loader/mapping changes address presentation, document the translation rather than silently rewriting old evidence.

## Per-function research checklist

For each function worth promoting:

- confirm exact entry address;
- note known callers/callees;
- identify strings/assets/constants;
- list significant global accesses;
- identify obvious runtime/library code;
- look for DOS/Windows correspondence;
- record a short behavior summary and confidence;
- update the platform function map;
- update `reverse/correspondence/dos_windows.csv` if a cross-build relationship is established;
- update `reverse/ghidra/DroneWin32Labels.py` only when the label is stable enough to be useful.

## Decompiler output policy

Decompiler output is a working aid, not project source. Do not paste large automatically decompiled functions into the public repository. Short pseudocode used to explain an algorithm should be independently written, limited to what is necessary to state the behavior, and tied to evidence addresses.

## Local project hygiene

Ghidra project databases and caches are local tooling artifacts. `.gitignore` excludes common Ghidra project forms. Scripts, symbol maps, independently authored notes, and small schemas are the durable/public representation of the research.

## Export/backup strategy

The project should remain reconstructable even if a local Ghidra project is lost. Therefore all stable knowledge must be exported into repository artifacts:

- function/global CSVs;
- correspondence ledger;
- structure/field ledger;
- label scripts;
- prose subsystem/format specs;
- research log.
