#!/usr/bin/env python3
"""Emit metadata-only boss resource ownership/presence for canonical Win32 evidence."""
from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path

# family, slot(s), function evidence, kind, canonical relative path
RESOURCES = (
    ("LidTop", "0;4", "load 0x00417350", "image", "Sights/Lid.jba"),
    ("LidTop", "0;4", "load 0x00417350", "image", "Sights/Top.jba"),
    ("LidTop", "0;4", "load 0x00417350", "audio", "Sounds/Retro1.wav"),
    ("LidTop", "0;4", "load 0x00417350", "audio", "Sounds/Level1.wav"),
    ("Gemini", "1", "load 0x00405FB0", "image", "Sights/Gemini1.jba"),
    ("Gemini", "1", "load 0x00405FB0", "image", "Sights/Gemini2.jba"),
    ("Gemini", "1", "load 0x00405FB0", "image", "Sights/Gemhead.jba"),
    ("Gemini", "1", "load 0x00405FB0", "audio", "Sounds/Gemini.wav"),
    ("Gemini", "1", "load 0x00405FB0", "audio", "Sounds/Level2.wav"),
    ("Spidey", "3", "load 0x00415C80", "image", "Sights/Spidey1.jba"),
    ("Spidey", "3", "load 0x00415C80", "image", "Sights/Spidey2.jba"),
    ("Spidey", "3", "load 0x00415C80", "image", "Sights/Spidey3.jba"),
    ("Spidey", "3", "load 0x00415C80", "image", "Sights/Spidey01.jba"),
    ("Spidey", "3", "load 0x00415C80", "image", "Sights/Spidey02.jba"),
    ("Spidey", "3", "load 0x00415C80", "image", "Sights/Spidey03.jba"),
    ("Spidey", "3", "load 0x00415C80", "image", "Sights/Spideydo.jba"),
    ("Spidey", "3", "load 0x00415C80", "audio", "Sounds/Spidey.wav"),
    ("Spidey", "3", "load 0x00415C80", "audio", "Sounds/Level4.wav"),
    ("Bomber", "5", "load 0x004044B0", "image", "Sights/Bombmid.jba"),
    ("Bomber", "5", "load 0x004044B0", "image", "Sights/Bombleft.jba"),
    ("Bomber", "5", "load 0x004044B0", "image", "Sights/Bombrite.jba"),
    ("Bomber", "5", "load 0x004044B0", "image", "Sights/Bombtop.jba"),
    ("Bomber", "5", "load 0x004044B0", "audio", "Sounds/Bomber1.wav"),
    ("Bomber", "5", "load 0x004044B0", "audio", "Sounds/Squish1.wav"),
    ("Bomber", "5", "load 0x004044B0", "audio", "Sounds/Absorb.wav"),
    ("Bomber", "5", "load 0x004044B0", "audio", "Sounds/Level3.wav"),
)

# Installed-file spelling can vary in case; canonical manifests preserve Windows names.
def resolve_case_insensitive(root: Path, relative: str) -> Path | None:
    cur = root
    for part in Path(relative).parts:
        if not cur.is_dir():
            return None
        matches = [p for p in cur.iterdir() if p.name.casefold() == part.casefold()]
        if not matches:
            return None
        cur = matches[0]
    return cur


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def build_rows(root: Path):
    for family, slots, evidence, kind, rel in RESOURCES:
        path = resolve_case_insensitive(root, rel)
        present = path is not None and path.is_file()
        yield {
            "family": family,
            "processed_drone_slots": slots,
            "kind": kind,
            "relative_path": rel,
            "present_in_windows_shareware": "yes" if present else "no",
            "size": path.stat().st_size if present else "",
            "sha256": sha256(path) if present else "",
            "evidence": evidence,
        }

    # The slot-2 combat/release implementation survives, but no dedicated
    # loader/resource literals can be tied to it in the canonical shareware PE.
    yield {
        "family": "RegisteredSlot2Unknown",
        "processed_drone_slots": "2",
        "kind": "unresolved",
        "relative_path": "",
        "present_in_windows_shareware": "unknown/not identified",
        "size": "",
        "sha256": "",
        "evidence": "init 0x00407980; update 0x00406CC0; release 0x00407AB0; no dedicated loader identified",
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("windows_root", type=Path)
    ap.add_argument("--output", type=Path)
    args = ap.parse_args()
    rows = list(build_rows(args.windows_root))
    fields = list(rows[0])
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        out = args.output.open("w", newline="")
    else:
        import sys
        out = sys.stdout
    with out if args.output else _NoClose(out) as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader(); w.writerows(rows)
    return 0


class _NoClose:
    def __init__(self, f): self.f = f
    def __enter__(self): return self.f
    def __exit__(self, *args): return False


if __name__ == "__main__":
    raise SystemExit(main())
