# Canonical DOS Analysis

Addresses in this directory refer to the canonical DOS shareware executable unless a file states otherwise.

SHA-256:

```text
e7de54f9cd158289df58a7eee4ecbe6f1b9b04972e7eae88b26bfe32515a0fc0
```

Identity: 32-bit x86 Watcom C/C++ Linear Executable using DOS/4GW.

Because LE loaders/tools can present addresses differently, preserve the mapping used by the existing `function_map.csv`. If a future analysis setup uses a different address presentation, document a translation instead of silently rewriting historical evidence.

## Phase 2 anchors

The reconstructed LE image now has a reproducible extractor in `scripts/extract_dos_le.py`. Phase 2 confirms a VGA vertical-retrace wait at linear address `0x0006940C`, VGA DAC read/write helpers near `0x00067E78`/`0x00067E94`, and a repeatedly indexed `0x14F`-byte object family that is being compared against the Win32 `0x154`-byte sprite/entity family.

## DOS retrace-sync protocol

Phase 2 establishes the DOS-side Tab implementation in the canonical linear disassembly. Data offset `0xD0` is initialized to `+1` at `0x00080BD6`, negated when key-state offset `0x0F` (Tab scan code) is active at `0x00078338..0x0007834C`, and consulted at `0x0007DC89` before calling `wait_vga_vertical_retrace` at `0x0006940C`. The same path renders `VERTICAL RETRACE`, `SYNC OFF`, and `SYNC ON` from object 4.

## LE internal-reference xrefs

Watcom's DOS/4GW LE image keeps many object-data references as loader fixups, so raw extracted-object disassembly displays object-relative immediates rather than final linear addresses. `scripts/find_dos_le_internal_xrefs.py` locates Drone's directly observed source-type-0x07 internal offset fixups for an exact target object/offset.

The Mothership loader correlation uses this tool to produce `reverse/dos/mothership_asset_xrefs.csv`. This is deliberately a narrow evidence scanner, not a general-purpose LE relocation implementation.

## Video mode anchor

Startup at `0x0006E005` passes mode value `0x13` to `0x00067E50`. The helper constructs BIOS `INT 10h, AH=0` through the DOS-extender real-mode interrupt bridge, proving standard 320x200x256 VGA/MCGA mode 13h rather than merely inferring the resolution from JBA assets. This matters for timing: the sync-enabled gameplay tail uses the VGA retrace wait against the nominal mode-13h display cadence. See `docs/TIMING.md`.
