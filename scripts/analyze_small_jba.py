#!/usr/bin/env python3
"""Inspect Drone's Windows-only small-JBA/PCX physical container family.

This tool emits metadata only. It does not write decoded proprietary image data.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path

NAMES = ("Logo.jba", "River.jba", "Screen.jba")
PIXELS = 128 * 128
PALETTE_BYTES = 256 * 3
HEADER_BYTES = 128


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def le16(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset:offset + 2], "little")


def analyze(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    if len(data) < 1 + HEADER_BYTES + PALETTE_BYTES:
        raise ValueError(f"{path}: too short")
    preamble = data[0]
    pcx = 1 + preamble
    h = data[pcx:pcx + HEADER_BYTES]
    if len(h) != HEADER_BYTES:
        raise ValueError(f"{path}: truncated PCX header")
    if h[:4] != bytes((0x0A, 0x05, 0x01, 0x08)):
        raise ValueError(f"{path}: unexpected PCX signature/version/encoding")
    if (le16(h, 4), le16(h, 6), le16(h, 8), le16(h, 10)) != (0, 0, 127, 127):
        raise ValueError(f"{path}: unexpected 128x128 bounds")
    if h[65] != 1 or le16(h, 66) != 128:
        raise ValueError(f"{path}: unexpected PCX plane/stride")

    palette_start = len(data) - PALETTE_BYTES
    pos = pcx + HEADER_BYTES
    pixels = bytearray()
    while len(pixels) < PIXELS:
        if pos >= palette_start:
            raise ValueError(f"{path}: truncated RLE")
        token = data[pos]
        pos += 1
        if token & 0xC0 == 0xC0:
            count = token & 0x3F
            if count == 0 or pos >= palette_start:
                raise ValueError(f"{path}: invalid RLE run")
            value = data[pos]
            pos += 1
            pixels.extend([value] * count)
        else:
            pixels.append(token)
        if len(pixels) > PIXELS:
            raise ValueError(f"{path}: RLE overflow")
    if pos != palette_start:
        raise ValueError(f"{path}: unexpected bytes before raw palette")

    return {
        "path": str(path),
        "file_bytes": len(data),
        "sha256": sha256(data),
        "preamble_bytes": preamble,
        "pcx_offset": pcx,
        "rle_bytes": palette_start - (pcx + HEADER_BYTES),
        "width": 128,
        "height": 128,
        "bits_per_pixel": h[3],
        "planes": h[65],
        "bytes_per_line": le16(h, 66),
        "palette_bytes": PALETTE_BYTES,
        "palette_sha256": sha256(data[palette_start:]),
        "decoded_pixels_sha256": sha256(bytes(pixels)),
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("sights", type=Path, help="installed Windows Sights directory")
    ap.add_argument("--csv", action="store_true", help="emit CSV")
    args = ap.parse_args()
    rows = [analyze(args.sights / name) for name in NAMES]
    if args.csv:
        writer = csv.DictWriter(__import__("sys").stdout, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    else:
        for row in rows:
            print(", ".join(f"{k}={v}" for k, v in row.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
