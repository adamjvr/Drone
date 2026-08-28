#!/usr/bin/env python3
"""Run the Linux fidelity host headlessly, capture one DRONEFB1 landmark, and fingerprint it.

The .drfb capture remains local evidence. The generated JSON contains hashes and
metadata only and can be published when its provenance is appropriate.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


def parse_region(text: str) -> str:
    parts = text.split(":")
    if len(parts) != 5:
        raise argparse.ArgumentTypeError("region must be NAME:X:Y:WIDTH:HEIGHT")
    return text


def run_checked(command: list[str]) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"command failed ({result.returncode}): {' '.join(command)}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", type=Path, help="drone_fidelity_host executable")
    parser.add_argument("input", type=Path, help="clean .drfb snapshot or full-screen .jba")
    parser.add_argument("capture_dir", type=Path, help="local directory for .drfb evidence")
    parser.add_argument("--landmark", required=True)
    parser.add_argument("--sequence", type=int, required=True)
    parser.add_argument("--fixture-id", required=True)
    parser.add_argument("--source-build", required=True)
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--tick", type=int)
    parser.add_argument("--region", type=parse_region, action="append")
    parser.add_argument("--notes", default="")
    parser.add_argument("--metadata", type=Path, required=True)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.sequence < 0:
        raise ValueError("sequence must be non-negative")
    args.capture_dir.mkdir(parents=True, exist_ok=True)

    host_result = run_checked([
        str(args.host),
        str(args.input),
        "--headless",
        "--capture-dir", str(args.capture_dir),
        "--landmark", args.landmark,
        "--sequence", str(args.sequence),
    ])
    captured_lines = [line for line in host_result.stdout.splitlines() if line.startswith("captured ")]
    if len(captured_lines) != 1:
        raise RuntimeError("fidelity host did not report exactly one capture path")
    snapshot = Path(captured_lines[0][len("captured "):])
    if not snapshot.is_file():
        raise RuntimeError(f"reported capture does not exist: {snapshot}")

    fixture_tool = Path(__file__).with_name("framebuffer_fixture.py")
    command = [
        sys.executable, str(fixture_tool), "fingerprint", str(snapshot),
        "--fixture-id", args.fixture_id,
        "--source-build", args.source_build,
        "--scenario", args.scenario,
        "--output", str(args.metadata),
    ]
    if args.tick is not None:
        command += ["--tick", str(args.tick)]
    for region in args.region or []:
        command += ["--region", region]
    if args.notes:
        command += ["--notes", args.notes]
    fingerprint_result = run_checked(command)

    print(f"snapshot={snapshot}")
    print(f"metadata={args.metadata}")
    if fingerprint_result.stdout:
        print(fingerprint_result.stdout.rstrip())
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(2)
