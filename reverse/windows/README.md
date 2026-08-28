# Canonical Win32 Analysis

Addresses in this directory refer to the canonical Windows shareware executable unless a file states otherwise.

SHA-256:

```text
4fffc0406157b0def539b619fa53d1bb6c59537a44a6551df3aa3c060eaf3784
```

Normal image base: `0x00400000`.

Use `reverse/ghidra/DroneWin32Labels.py` only after verifying the binary hash. `function_map.csv` and `global_map.csv` are the durable address ledgers; prose files explain major control paths.

## Phase 2 anchors

Phase 2 has added several high-confidence anchors to the durable function/structure ledgers, including the clipped transparent sprite blitter (`0x00401660`), common `0x154`-byte sprite/entity initializer (`0x00401780`), frame-pointer cleanup (`0x00401820`), and CRT PRNG anchors (`0x00421EC0` / `0x00421ED0`). See `docs/PHASE2.md` and `docs/reverse/STRUCTURE_RECOVERY.md`.


## Dynamic palette cluster

Phase 3 classifies `0x004011E0` as the inclusive DirectDraw palette-range upload wrapper, `0x00403490` as the generic gameplay palette animator, and `0x0041EFE0` / `0x0041EE90` as the purpose-built gameplay palette-band initializer/updater. Exact ranges and clean semantic implementations are documented in [`../../docs/reverse/PALETTE_EFFECTS.md`](../../docs/reverse/PALETTE_EFFECTS.md).

## Phase 3 renderer ownership

The state-2 world/effect paint tail now has a 28-subpass evidence catalog covering fixed boss composites, point particles, Gemini procedural work, explosion/debris routing, and trajectory/projectile/player ordering. See [`../../docs/reverse/WORLD_PRESENTATION_SUBPASSES.md`](../../docs/reverse/WORLD_PRESENTATION_SUBPASSES.md).
