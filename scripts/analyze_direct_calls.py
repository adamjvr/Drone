#!/usr/bin/env python3
"""Create a static direct-call inventory for a PE/ELF range using objdump.

This is a research convenience tool. It records call *instruction sites* in the
selected address range; counts are not runtime call frequencies.
"""
from __future__ import annotations

import argparse
import csv
import re
import shutil
import subprocess
from collections import Counter
from pathlib import Path

CALL_RE = re.compile(r"^\s*[0-9a-fA-F]+:\s+(?:[0-9a-fA-F]{2}\s+)+call\s+(?:DWORD PTR )?(?:ds:)?0x([0-9a-fA-F]+)(?:\s|$)")


def parse_addr(value: str) -> int:
    return int(value, 0)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("binary", type=Path)
    ap.add_argument("start", type=parse_addr, help="inclusive start address, e.g. 0x40bab9")
    ap.add_argument("stop", type=parse_addr, help="exclusive stop address, e.g. 0x411d86")
    ap.add_argument("--csv", type=Path, required=True)
    ap.add_argument("--function-map", type=Path, help="optional CSV with address,provisional_name columns")
    args = ap.parse_args()

    objdump = shutil.which("objdump")
    if objdump is None:
        raise SystemExit("objdump not found in PATH")
    if not args.binary.is_file():
        raise SystemExit(f"binary not found: {args.binary}")
    if args.stop <= args.start:
        raise SystemExit("stop must be greater than start")

    proc = subprocess.run(
        [
            objdump,
            "-d",
            "-Mintel",
            f"--start-address={args.start}",
            f"--stop-address={args.stop}",
            str(args.binary),
        ],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )

    counts: Counter[int] = Counter()
    for line in proc.stdout.splitlines():
        match = CALL_RE.match(line)
        if match:
            counts[int(match.group(1), 16)] += 1

    names: dict[int, str] = {}
    if args.function_map:
        with args.function_map.open(newline="", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                try:
                    names[int(row["address"], 0)] = row.get("provisional_name", "")
                except (KeyError, ValueError):
                    continue

    args.csv.parent.mkdir(parents=True, exist_ok=True)
    with args.csv.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["target_address", "provisional_name", "static_call_sites", "scope_start", "scope_stop"])
        for target, count in sorted(counts.items(), key=lambda item: (-item[1], item[0])):
            w.writerow([f"0x{target:08X}", names.get(target, ""), count, f"0x{args.start:08X}", f"0x{args.stop:08X}"])

    print(f"wrote {len(counts)} direct-call targets to {args.csv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
