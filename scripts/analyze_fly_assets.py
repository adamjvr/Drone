#!/usr/bin/env python3
"""Summarize canonical Drone FLY trajectory files without copying payload data."""
from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path

LOADER_COUNTS = {
    "frisbee1.fly": 937,
    "frisbee2.fly": 426,
    "leftdive.fly": 119,
    "leftdrop.fly": 200,
    "loop.fly": 380,
    "newcurly.fly": 232,
    "rightdiv.fly": 119,
    "ritedrop.fly": 200,
    "swarm.fly": 950,
    "swoop.fly": 190,
}


def summarize(path: Path) -> dict[str, str | int]:
    values = [int(v) for v in path.read_text(encoding="ascii").split()]
    lower = path.name.lower()
    counted = bool(values and values[0] >= 0 and len(values) == 1 + 3 * values[0])
    start = 1 if counted else 0
    payload = values[start:]
    if len(payload) % 3:
        raise ValueError(f"{path}: numeric payload is not divisible by three")
    triples = [payload[i:i + 3] for i in range(0, len(payload), 3)]
    physical = len(triples)
    loader_count = values[0] if counted else LOADER_COUNTS.get(lower)
    ranges = [(min(t[i] for t in triples), max(t[i] for t in triples)) for i in range(3)] if triples else [(0, 0)] * 3
    mismatch = ""
    if loader_count is not None and loader_count != physical:
        mismatch = f"loader requests {loader_count}; file physically contains {physical}"
    return {
        "filename": path.name,
        "encoding": "counted-current" if counted else "raw-trajectory",
        "physical_records": physical,
        "loader_records": "" if loader_count is None else loader_count,
        "x_min": ranges[0][0], "x_max": ranges[0][1],
        "y_min": ranges[1][0], "y_max": ranges[1][1],
        "aux_min": ranges[2][0], "aux_max": ranges[2][1],
        "bytes": path.stat().st_size,
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "note": mismatch,
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("directory", type=Path, help="Directory containing canonical *.fly files")
    ap.add_argument("--csv", type=Path)
    args = ap.parse_args()

    rows = [summarize(p) for p in sorted(args.directory.glob("*.fly"), key=lambda p: p.name.lower())]
    if not rows:
        raise SystemExit("no .fly files found")
    fields = list(rows[0])
    if args.csv:
        args.csv.parent.mkdir(parents=True, exist_ok=True)
        with args.csv.open("w", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=fields)
            writer.writeheader(); writer.writerows(rows)
    else:
        writer = csv.DictWriter(__import__("sys").stdout, fieldnames=fields)
        writer.writeheader(); writer.writerows(rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
