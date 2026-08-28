#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import os
import json
import struct
import subprocess
import sys
import tempfile

WIDTH = 320
HEIGHT = 200
PIXELS = WIDTH * HEIGHT
PALETTE = 256 * 3
HEADER = struct.Struct("<8sHHHHII")


def snapshot_bytes() -> bytes:
    pixels = bytes((x * 3 + y * 5) & 0xFF for y in range(HEIGHT) for x in range(WIDTH))
    palette = bytearray()
    for i in range(256):
        palette.extend((i, 255 - i, (i * 7) & 0xFF))
    return HEADER.pack(b"DRONEFB1", WIDTH, HEIGHT, 256, 0, PIXELS, PALETTE) + pixels + palette


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_linux_fidelity_host.py /path/to/drone_fidelity_host")
    host = Path(sys.argv[1])
    with tempfile.TemporaryDirectory(prefix="drone-linux-host-") as td:
        root = Path(td)
        source = root / "source.drfb"
        source.write_bytes(snapshot_bytes())
        captures = root / "captures"
        env = dict(os.environ)
        env.pop("DISPLAY", None)
        result = subprocess.run(
            [
                str(host), str(source), "--headless", "--capture-dir", str(captures),
                "--landmark", "linux headless / present", "--sequence", "17",
            ],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
            check=False,
        )
        if result.returncode != 0:
            raise AssertionError(
                f"headless fidelity host failed: {result.returncode}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}"
            )
        output = captures / "00000017-linux-headless-present.drfb"
        assert output.exists(), result.stdout
        assert output.read_bytes() == source.read_bytes()

        metadata = root / "linux-capture.json"
        wrapper = Path(__file__).resolve().parents[1] / "scripts" / "capture_linux_fidelity_host.py"
        wrapped = subprocess.run(
            [
                sys.executable, str(wrapper), str(host), str(source), str(captures),
                "--landmark", "clean probe tick 120",
                "--sequence", "120",
                "--fixture-id", "linux-clean-probe-120",
                "--source-build", "public-test",
                "--scenario", "synthetic-host",
                "--tick", "120",
                "--region", "hud:0:0:320:32",
                "--metadata", str(metadata),
            ],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, check=False,
        )
        if wrapped.returncode != 0:
            raise AssertionError(
                f"capture wrapper failed: {wrapped.returncode}\nstdout:\n{wrapped.stdout}\nstderr:\n{wrapped.stderr}"
            )
        wrapped_snapshot = captures / "00000120-clean-probe-tick-120.drfb"
        assert wrapped_snapshot.read_bytes() == source.read_bytes()
        doc = json.loads(metadata.read_text(encoding="utf-8"))
        assert doc["fixture_id"] == "linux-clean-probe-120"
        assert doc["tick"] == 120
        assert [region["name"] for region in doc["regions"]] == ["hud"]

    print("linux fidelity host headless capture tests passed")


if __name__ == "__main__":
    main()
