#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "scripts/framebuffer_fixture.py"
HEADER = struct.Struct("<8sHHHHII")
WIDTH = 320
HEIGHT = 200
PIXELS = WIDTH * HEIGHT
PALETTE = 256 * 3


def write_snapshot(path: Path, mutate: bool = False) -> None:
    pixels = bytearray((x + y) % 256 for y in range(HEIGHT) for x in range(WIDTH))
    if mutate:
        pixels[10 * WIDTH + 20] ^= 7
    palette = bytearray()
    for i in range(256):
        palette.extend((i, (i * 3) & 0xFF, 255 - i))
    path.write_bytes(
        HEADER.pack(b"DRONEFB1", WIDTH, HEIGHT, 256, 0, PIXELS, PALETTE)
        + pixels
        + palette
    )


def run(*args: str, expected: int = 0) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        [sys.executable, str(TOOL), *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != expected:
        raise AssertionError(
            f"command returned {result.returncode}, expected {expected}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="drone-frame-fixture-") as td:
        root = Path(td)
        snapshot = root / "frame.drfb"
        changed = root / "changed.drfb"
        metadata = root / "frame.json"
        write_snapshot(snapshot)
        write_snapshot(changed, mutate=True)

        run(
            "fingerprint", str(snapshot),
            "--fixture-id", "synthetic-frame-001",
            "--source-build", "public-test",
            "--scenario", "synthetic",
            "--tick", "42",
            "--region", "hud:0:0:320:32",
            "--region", "probe:16:8:64:40",
            "--output", str(metadata),
        )
        doc = json.loads(metadata.read_text(encoding="utf-8"))
        assert doc["schema"] == "drone.framebuffer-fixture.v1"
        assert doc["snapshot_bytes"] == HEADER.size + PIXELS + PALETTE
        assert len(doc["snapshot_sha256"]) == 64
        assert len(doc["pixels_sha256"]) == 64
        assert len(doc["palette_sha256"]) == 64
        assert len(doc["rgba_sha256"]) == 64
        assert [r["name"] for r in doc["regions"]] == ["hud", "probe"]

        run("verify", str(metadata), str(snapshot))
        run("verify", str(metadata), str(changed), expected=1)

    print("framebuffer fixture tool tests passed")


if __name__ == "__main__":
    main()
