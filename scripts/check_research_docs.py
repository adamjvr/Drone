#!/usr/bin/env python3
"""Validate publishable Drone documentation/research metadata without original assets."""
from __future__ import annotations

import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(msg: str) -> None:
    raise RuntimeError(msg)


def check_markdown_links() -> int:
    pattern = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
    checked = 0
    for md in sorted(ROOT.rglob("*.md")):
        # Never walk build/reference trees if a developer happens to have them present.
        rel = md.relative_to(ROOT)
        if rel.parts and rel.parts[0] in {"build", ".reference"}:
            continue
        text = md.read_text(encoding="utf-8", errors="strict")
        for target in pattern.findall(text):
            target = target.strip().split("#", 1)[0]
            if not target or target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            # Markdown can wrap local paths in angle brackets.
            if target.startswith("<") and target.endswith(">"):
                target = target[1:-1]
            dest = (md.parent / target).resolve()
            try:
                dest.relative_to(ROOT.resolve())
            except ValueError:
                fail(f"link escapes repository: {rel}: {target}")
            if not dest.exists():
                fail(f"broken local markdown link: {rel}: {target}")
            checked += 1
    return checked


def read_csv(rel: str) -> list[dict[str, str]]:
    path = ROOT / rel
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def check_unique_ids(rel: str) -> int:
    rows = read_csv(rel)
    ids = [row.get("id", "").strip() for row in rows]
    if any(not x for x in ids):
        fail(f"empty id in {rel}")
    if len(ids) != len(set(ids)):
        fail(f"duplicate id in {rel}")
    return len(rows)


def check_corpus_manifests() -> None:
    expected = {
        "manifests/dos_shareware_files.csv": 187,
        "manifests/windows_shareware_files.csv": 192,
    }
    sha_re = re.compile(r"^[0-9a-f]{64}$")
    for rel, count in expected.items():
        rows = read_csv(rel)
        if len(rows) != count:
            fail(f"{rel}: expected {count} rows, got {len(rows)}")
        paths: set[str] = set()
        for row in rows:
            path = row.get("path", "")
            if not path or path in paths:
                fail(f"{rel}: empty/duplicate path {path!r}")
            paths.add(path)
            if int(row["bytes"]) < 0:
                fail(f"{rel}: negative byte count for {path}")
            if not sha_re.fullmatch(row.get("sha256", "")):
                fail(f"{rel}: invalid sha256 for {path}")


def check_wise_manifest() -> None:
    rows = read_csv("manifests/windows_shareware_wise_streams.csv")
    if len(rows) != 207:
        fail(f"Wise manifest: expected 207 streams, got {len(rows)}")
    installed = sum(bool(row.get("installed_path", "").strip()) for row in rows)
    if installed != 192:
        fail(f"Wise manifest: expected 192 installed paths, got {installed}")


def main() -> int:
    links = check_markdown_links()
    finding_rows = check_unique_ids("reverse/ledger/findings.csv")
    question_rows = check_unique_ids("reverse/ledger/open_questions.csv")
    function_rows = check_unique_ids("reverse/ledger/functions.csv")
    global_rows = check_unique_ids("reverse/ledger/globals.csv")
    correspondence_rows = check_unique_ids("reverse/correspondence/dos_windows.csv")
    structure_rows = check_unique_ids("reverse/structures/structure_ledger.csv")
    check_corpus_manifests()
    check_wise_manifest()
    print(
        "research metadata OK: "
        f"{links} local links, {finding_rows} findings, {question_rows} questions, "
        f"{function_rows} functions, {global_rows} globals, "
        f"{correspondence_rows} correspondences, {structure_rows} structures"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, ValueError, KeyError) as exc:
        print(f"research metadata FAILED: {exc}", file=sys.stderr)
        raise SystemExit(1)
