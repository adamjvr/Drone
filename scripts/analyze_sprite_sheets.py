#!/usr/bin/env python3
"""Reproduce selected Win32 sprite-sheet frame extractions as metadata.

The original Win32 routine at 0x00401860 crops width x height frames from a
320x200 decoded JBA sheet. Cells have a one-pixel gutter:

    source_x = cell_x * (width + 1) + 1
    source_y = cell_y * (height + 1) + 1

This tool emits hashes only; it does not copy proprietary pixels into the repo.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
from dataclasses import dataclass
from pathlib import Path

WIDTH = 320
HEIGHT = 200
PALETTE_BYTES = 768
PIXELS = WIDTH * HEIGHT
JBA_BYTES = PALETTE_BYTES + PIXELS

@dataclass(frozen=True)
class SheetSpec:
    relative_path: str
    sprite_width: int
    sprite_height: int
    columns: int
    rows: int
    frame_count: int
    frame_slot_base: int = 0

# Established from sprite_entity_init (0x00401780) + extract calls
# (0x00401860) in the canonical Win32 shareware executable.
SPECS = (
    SheetSpec("Sights/Debris1.jba", 25, 18, 8, 1, 8),
    SheetSpec("Sights/Debris2a.jba", 27, 17, 8, 2, 16),
    # player_ship_entity (0x00466B18), initialized as 22x22 and populated
    # by 15 calls to 0x00401860: four cells on rows 0..2 and three on row 3.
    SheetSpec("Sights/Ship.jba", 22, 22, 4, 4, 15),
    # rapid-fire missile pool (0x0042F200): eight 1x9 entities sharing
    # three frames extracted from missile.jba cells (0,0), (1,0), (2,0).
    SheetSpec("Sights/Missile.jba", 1, 9, 3, 1, 3),
    # enemy bomb pool (0x004651A0): initialized as 1x9 common entities and
    # populated by three calls to 0x00401860 from bomb.jba cells (0..2,0).
    SheetSpec("Sights/Bomb.jba", 1, 9, 3, 1, 3),
    # special projectile entity (0x0045A148): frame/current-weapon 0 is the
    # 3x8 blue probe and frame/current-weapon 1 is the 3x8 red stinger.
    SheetSpec("Sights/Probe.jba", 3, 8, 1, 1, 1),
    SheetSpec("Sights/Redprobe.jba", 3, 8, 1, 1, 1),
    # canonical Drone target entity (0x00446080), initialized 15x38 and
    # populated from drone.jba cell (0,0).
    SheetSpec("Sights/Drone.jba", 15, 38, 1, 1, 1),
    # alternate special-interaction entity (0x00433700), initialized 19x13
    # and populated from hole.jba. It participates in the established special
    # weapon HoleInteraction path.
    SheetSpec("Sights/Hole.jba", 19, 13, 1, 1, 1),
    # game-over banner entity (0x004671E8), initialized as 117x20 and
    # populated from gameover.jba cell (0,0). 0x0041E420 slides it on screen.
    SheetSpec("Sights/Gameover.jba", 117, 20, 1, 1, 1),
    # Lid/Top boss family: 0x004406F8 is a 36x40 common entity populated
    # with nine lid.jba frames (5 on row 0, 4 on row 1); 0x00446E00 is a
    # 68x56 common entity populated with top.jba cell (0,0).
    SheetSpec("Sights/Lid.jba", 36, 40, 5, 2, 9),
    SheetSpec("Sights/Top.jba", 68, 56, 1, 1, 1),
    # Gemini boss: two 56x41 body entities share one 30-frame table.
    # Gemini1 supplies slots 0..14 and Gemini2 supplies slots 15..29, each
    # in a 5x3 guttered grid. Two 43x34 head entities share Gemhead slot 0.
    SheetSpec("Sights/Gemini1.jba", 56, 41, 5, 3, 15, 0),
    SheetSpec("Sights/Gemini2.jba", 56, 41, 5, 3, 15, 15),
    SheetSpec("Sights/Gemhead.jba", 43, 34, 1, 1, 1, 0),
)

def decode_jba(path: Path) -> bytes:
    raw = path.read_bytes()
    if len(raw) != JBA_BYTES:
        raise ValueError(f"{path}: expected {JBA_BYTES} bytes, got {len(raw)}")
    out = bytearray(PIXELS)
    src = PALETTE_BYTES
    for lane in range(10):
        for dst in range(lane, PIXELS, 10):
            out[dst] = raw[src]
            src += 1
    return bytes(out)

def extract(pixels: bytes, width: int, height: int, cell_x: int, cell_y: int) -> bytes:
    x0 = cell_x * (width + 1) + 1
    y0 = cell_y * (height + 1) + 1
    if x0 + width > WIDTH or y0 + height > HEIGHT:
        raise ValueError("recovered frame cell exceeds JBA sheet")
    frame = bytearray()
    for y in range(height):
        off = (y0 + y) * WIDTH + x0
        frame += pixels[off:off + width]
    return bytes(frame)

def rows(root: Path):
    for spec in SPECS:
        path = root / spec.relative_path
        pixels = decode_jba(path)
        slot = 0
        for cy in range(spec.rows):
            for cx in range(spec.columns):
                if slot >= spec.frame_count:
                    break
                frame = extract(pixels, spec.sprite_width, spec.sprite_height, cx, cy)
                yield {
                    "asset": spec.relative_path,
                    "frame_slot": spec.frame_slot_base + slot,
                    "sprite_width": spec.sprite_width,
                    "sprite_height": spec.sprite_height,
                    "cell_x": cx,
                    "cell_y": cy,
                    "source_x": cx * (spec.sprite_width + 1) + 1,
                    "source_y": cy * (spec.sprite_height + 1) + 1,
                    "pixel_bytes": len(frame),
                    "sha256_indexed_pixels": hashlib.sha256(frame).hexdigest(),
                }
                slot += 1

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("windows_root", type=Path, help="extracted canonical Windows shareware root")
    ap.add_argument("--output", type=Path)
    args = ap.parse_args()
    data = list(rows(args.windows_root))
    fields = list(data[0]) if data else []
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open("w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=fields)
            w.writeheader(); w.writerows(data)
    else:
        w = csv.DictWriter(__import__('sys').stdout, fieldnames=fields)
        w.writeheader(); w.writerows(data)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
