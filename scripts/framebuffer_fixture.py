#!/usr/bin/env python3
"""Fingerprint and verify local Drone indexed-framebuffer snapshots.

The public repository stores only metadata/hashes. Original-runtime .drfb
snapshots stay in .reference/ or another user-controlled evidence location.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
from typing import Iterable

MAGIC = b"DRONEFB1"
WIDTH = 320
HEIGHT = 200
PIXEL_BYTES = WIDTH * HEIGHT
PALETTE_BYTES = 256 * 3
HEADER = struct.Struct("<8sHHHHII")
FILE_BYTES = HEADER.size + PIXEL_BYTES + PALETTE_BYTES


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_snapshot(path: Path) -> tuple[bytes, bytes, bytes]:
    data = path.read_bytes()
    if len(data) != FILE_BYTES:
        raise ValueError(f"snapshot must be exactly {FILE_BYTES} bytes, got {len(data)}")
    magic, width, height, entries, reserved, pixels_len, palette_len = HEADER.unpack_from(data)
    if magic != MAGIC:
        raise ValueError("invalid DRONEFB1 magic/version")
    if (width, height, entries, reserved, pixels_len, palette_len) != (
        WIDTH, HEIGHT, 256, 0, PIXEL_BYTES, PALETTE_BYTES
    ):
        raise ValueError("unsupported DRONEFB1 geometry/layout")
    start = HEADER.size
    pixels = data[start:start + PIXEL_BYTES]
    palette = data[start + PIXEL_BYTES:]
    return data, pixels, palette


def rgba_bytes(pixels: bytes, palette: bytes) -> bytes:
    out = bytearray(PIXEL_BYTES * 4)
    for i, index in enumerate(pixels):
        p = index * 3
        o = i * 4
        out[o:o + 3] = palette[p:p + 3]
        out[o + 3] = 255
    return bytes(out)


def parse_region(text: str) -> tuple[str, int, int, int, int]:
    try:
        name, x, y, width, height = text.split(":", 4)
        x_i, y_i, w_i, h_i = map(int, (x, y, width, height))
    except Exception as exc:
        raise argparse.ArgumentTypeError(
            "region must be NAME:X:Y:WIDTH:HEIGHT") from exc
    if not name:
        raise argparse.ArgumentTypeError("region name may not be empty")
    if w_i <= 0 or h_i <= 0 or x_i < 0 or y_i < 0:
        raise argparse.ArgumentTypeError("region coordinates must be non-negative and dimensions positive")
    if x_i + w_i > WIDTH or y_i + h_i > HEIGHT:
        raise argparse.ArgumentTypeError("region exceeds 320x200 framebuffer")
    return name, x_i, y_i, w_i, h_i


def region_bytes(plane: bytes, stride: int, pixel_size: int, x: int, y: int, width: int, height: int) -> bytes:
    out = bytearray(width * height * pixel_size)
    cursor = 0
    row_bytes = width * pixel_size
    for row in range(y, y + height):
        start = (row * stride + x) * pixel_size
        out[cursor:cursor + row_bytes] = plane[start:start + row_bytes]
        cursor += row_bytes
    return bytes(out)


def fingerprint(
    path: Path,
    fixture_id: str,
    source_build: str,
    scenario: str,
    tick: int | None,
    regions: Iterable[tuple[str, int, int, int, int]],
    notes: str,
) -> dict:
    data, pixels, palette = load_snapshot(path)
    rgba = rgba_bytes(pixels, palette)
    result = {
        "schema": "drone.framebuffer-fixture.v1",
        "fixture_id": fixture_id,
        "source_build": source_build,
        "scenario": scenario,
        "tick": tick,
        "geometry": {"width": WIDTH, "height": HEIGHT, "pixel_format": "indexed8", "palette": "256xRGB8"},
        "snapshot_bytes": len(data),
        "snapshot_sha256": sha256(data),
        "pixels_sha256": sha256(pixels),
        "palette_sha256": sha256(palette),
        "rgba_sha256": sha256(rgba),
        "regions": [],
        "notes": notes,
    }
    for name, x, y, width, height in regions:
        index_region = region_bytes(pixels, WIDTH, 1, x, y, width, height)
        rgba_region = region_bytes(rgba, WIDTH, 4, x, y, width, height)
        result["regions"].append({
            "name": name,
            "x": x,
            "y": y,
            "width": width,
            "height": height,
            "pixels_sha256": sha256(index_region),
            "rgba_sha256": sha256(rgba_region),
        })
    return result


def cmd_fingerprint(args: argparse.Namespace) -> int:
    regions = args.region or []
    meta = fingerprint(
        args.snapshot,
        args.fixture_id,
        args.source_build,
        args.scenario,
        args.tick,
        regions,
        args.notes,
    )
    text = json.dumps(meta, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8")
        print(f"wrote {args.output}")
    else:
        sys.stdout.write(text)
    return 0


def cmd_verify(args: argparse.Namespace) -> int:
    expected = json.loads(args.metadata.read_text(encoding="utf-8"))
    if expected.get("schema") != "drone.framebuffer-fixture.v1":
        raise ValueError("unsupported framebuffer fixture metadata schema")
    regions = [
        (row["name"], int(row["x"]), int(row["y"]), int(row["width"]), int(row["height"]))
        for row in expected.get("regions", [])
    ]
    actual = fingerprint(
        args.snapshot,
        str(expected.get("fixture_id", "")),
        str(expected.get("source_build", "")),
        str(expected.get("scenario", "")),
        expected.get("tick"),
        regions,
        str(expected.get("notes", "")),
    )
    checked = ["snapshot_sha256", "pixels_sha256", "palette_sha256", "rgba_sha256"]
    mismatches = [key for key in checked if expected.get(key) != actual.get(key)]
    if expected.get("regions", []) != actual.get("regions", []):
        mismatches.append("regions")
    if mismatches:
        print("framebuffer fixture verification FAILED: " + ", ".join(mismatches), file=sys.stderr)
        return 1
    print(
        f"framebuffer fixture OK: {expected.get('fixture_id', '<unnamed>')} "
        f"snapshot={actual['snapshot_sha256']}"
    )
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    fp = sub.add_parser("fingerprint", help="emit publishable hash metadata for a local .drfb snapshot")
    fp.add_argument("snapshot", type=Path)
    fp.add_argument("--fixture-id", required=True)
    fp.add_argument("--source-build", required=True)
    fp.add_argument("--scenario", required=True)
    fp.add_argument("--tick", type=int)
    fp.add_argument("--region", type=parse_region, action="append", help="NAME:X:Y:WIDTH:HEIGHT")
    fp.add_argument("--notes", default="")
    fp.add_argument("--output", type=Path)
    fp.set_defaults(func=cmd_fingerprint)

    verify = sub.add_parser("verify", help="verify a local .drfb snapshot against committed metadata")
    verify.add_argument("metadata", type=Path)
    verify.add_argument("snapshot", type=Path)
    verify.set_defaults(func=cmd_verify)
    return parser


def main() -> int:
    try:
        args = build_parser().parse_args()
        return args.func(args)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
