#!/usr/bin/env python3
"""Build an asset-safe DOS/Windows audio metadata crosswalk from checked-in manifests."""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path

AUDIO_EXTENSIONS = {".wav", ".clv", ".hmi"}


def read_audio_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return [
            row
            for row in csv.DictReader(handle)
            if row["extension"].lower() in AUDIO_EXTENSIONS
        ]


def stem(path: str) -> str:
    return Path(path).stem.lower()


def aliases(rows: list[dict[str, str]]) -> dict[str, list[str]]:
    by_hash: dict[str, list[str]] = defaultdict(list)
    for row in rows:
        by_hash[row["sha256"]].append(row["path"])
    return {digest: paths for digest, paths in by_hash.items() if len(paths) > 1}


def alias_note(row: dict[str, str] | None, duplicate_map: dict[str, list[str]], prefix: str) -> str:
    if row is None:
        return ""
    paths = [path for path in duplicate_map.get(row["sha256"], []) if path != row["path"]]
    if not paths:
        return ""
    return f"{prefix}_sha_alias=" + "|".join(sorted(paths, key=str.lower))


def build_rows(windows: list[dict[str, str]], dos: list[dict[str, str]]) -> list[dict[str, str]]:
    win_by_stem = {stem(row["path"]): row for row in windows}
    dos_by_stem: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in dos:
        dos_by_stem[stem(row["path"])].append(row)

    win_aliases = aliases(windows)
    dos_aliases = aliases(dos)
    names = sorted(set(win_by_stem) | set(dos_by_stem))
    output: list[dict[str, str]] = []

    for name in names:
        win = win_by_stem.get(name)
        dos_variants = sorted(dos_by_stem.get(name, []), key=lambda row: row["path"].lower())
        if not dos_variants:
            dos_variants = [None]
        for dos_row in dos_variants:
            if win is not None and dos_row is not None:
                relationship = "matched_stem"
            elif win is not None:
                relationship = "windows_only"
            else:
                relationship = "dos_only"

            notes = [
                note
                for note in (
                    alias_note(win, win_aliases, "windows"),
                    alias_note(dos_row, dos_aliases, "dos"),
                )
                if note
            ]
            output.append(
                {
                    "stem": name,
                    "relationship": relationship,
                    "windows_path": "" if win is None else win["path"],
                    "windows_bytes": "" if win is None else win["bytes"],
                    "windows_sha256": "" if win is None else win["sha256"],
                    "dos_path": "" if dos_row is None else dos_row["path"],
                    "dos_bytes": "" if dos_row is None else dos_row["bytes"],
                    "dos_sha256": "" if dos_row is None else dos_row["sha256"],
                    "notes": ";".join(notes),
                }
            )
    return output


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--windows", type=Path, default=Path("manifests/windows_shareware_files.csv"))
    parser.add_argument("--dos", type=Path, default=Path("manifests/dos_shareware_files.csv"))
    parser.add_argument("--output", type=Path, default=Path("manifests/audio_asset_crosswalk.csv"))
    args = parser.parse_args()

    windows = read_audio_rows(args.windows)
    dos = read_audio_rows(args.dos)
    rows = build_rows(windows, dos)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()), lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)

    counts = defaultdict(int)
    for row in rows:
        counts[row["relationship"]] += 1
    print(f"windows_audio={len(windows)} dos_audio={len(dos)} crosswalk_rows={len(rows)}")
    print(" ".join(f"{key}={counts[key]}" for key in sorted(counts)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
