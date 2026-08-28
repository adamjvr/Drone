#!/usr/bin/env python3
"""Emit metadata-only statistics for canonical Drone demo recordings.

No replay payload is copied into the repository. The output contains hashes,
record counts and event/control counts useful for regression/provenance work.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path

FIELDS = 14
CANONICAL_WINDOWS = (
    "Demoa1.dat", "Demoa2.dat", "Demoa3.dat", "Demoa4.dat",
    "Demob1.dat", "Demob2.dat", "Demob3.dat",
)


def read_records(path: Path) -> list[list[int]]:
    values = [int(v) for v in path.read_text(encoding="ascii").split()]
    if len(values) % FIELDS:
        raise ValueError(f"{path}: value count {len(values)} not divisible by {FIELDS}")
    return [values[i:i + FIELDS] for i in range(0, len(values), FIELDS)]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("windows_data", type=Path, help="canonical Windows Data directory")
    ap.add_argument("--dos-root", type=Path, help="optional canonical DOS extraction root")
    ap.add_argument("--output", type=Path)
    args = ap.parse_args()

    rows: list[dict[str, object]] = []
    for name in CANONICAL_WINDOWS:
        path = args.windows_data / name
        records = read_records(path)
        win_hash = sha256(path)
        dos_path = args.dos_root / name.upper() if args.dos_root else None
        dos_present = bool(dos_path and dos_path.exists())
        dos_hash = sha256(dos_path) if dos_present else ""
        trajectory = [r for r in records if r[6] < 99]
        bombs = [r for r in records if r[9] != 0]
        rows.append({
            "filename": name,
            "records": len(records),
            "sha256_windows": win_hash,
            "dos_present": int(dos_present),
            "sha256_dos": dos_hash,
            "cross_build_identical": int(dos_present and dos_hash == win_hash),
            "left_frames": sum(r[0] != 0 for r in records),
            "right_frames": sum(r[1] != 0 for r in records),
            "special_launch_frames": sum(r[2] != 0 for r in records),
            "special_load_cycle_frames": sum(r[3] != 0 for r in records),
            "shield_frames": sum(r[4] != 0 for r in records),
            "rapid_missile_frames": sum(r[5] != 0 for r in records),
            "trajectory_group_events": len(trajectory),
            "bomb_spawn_events": len(bombs),
            "trajectory_group_slots": ";".join(map(str, sorted({r[6] for r in trajectory}))),
            "trajectory_path_families": ";".join(map(str, sorted({r[8] for r in trajectory if r[8] < 99}))),
        })

    fields = list(rows[0])
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        out = args.output.open("w", newline="", encoding="utf-8")
    else:
        import sys
        out = sys.stdout
    try:
        writer = csv.DictWriter(out, fieldnames=fields)
        writer.writeheader(); writer.writerows(rows)
    finally:
        if args.output:
            out.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
