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
    if rows and "platform" in rows[0] and "address" in rows[0]:
        keys = [(row.get("platform", "").strip(), row.get("address", "").strip().lower()) for row in rows]
        if len(keys) != len(set(keys)):
            fail(f"duplicate platform/address in {rel}")
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



def check_fly_manifest() -> None:
    rows = read_csv("manifests/fly_trajectories.csv")
    if len(rows) != 12:
        fail(f"FLY manifest: expected 12 canonical Windows FLY files, got {len(rows)}")
    by_name = {row["filename"].lower(): row for row in rows}
    if by_name.get("current.fly", {}).get("encoding") != "counted-current":
        fail("FLY manifest: Current.fly must be counted-current")
    for name, physical, loader in (("rightdiv.fly", 118, 119), ("swarm.fly", 949, 950)):
        row = by_name.get(name)
        if not row:
            fail(f"FLY manifest: missing {name}")
        if int(row["physical_records"]) != physical or int(row["loader_records"]) != loader:
            fail(f"FLY manifest: unexpected canonical mismatch metadata for {name}")
    sha_re = re.compile(r"^[0-9a-f]{64}$")
    for row in rows:
        if not sha_re.fullmatch(row.get("sha256", "")):
            fail(f"FLY manifest: invalid sha256 for {row.get('filename', '')}")



def check_demo_manifest() -> None:
    rows = read_csv("manifests/demo_replays.csv")
    if len(rows) != 7:
        fail(f"demo manifest: expected 7 canonical Windows demos, got {len(rows)}")
    sha_re = re.compile(r"^[0-9a-f]{64}$")
    by_name = {row["filename"].lower(): row for row in rows}
    shared = {"demoa2.dat", "demoa4.dat", "demob1.dat", "demob3.dat"}
    for row in rows:
        if int(row["records"]) != 2101:
            fail(f"demo manifest: {row['filename']} must contain 2101 records")
        if not sha_re.fullmatch(row["sha256_windows"]):
            fail(f"demo manifest: invalid Windows hash for {row['filename']}")
    for name in shared:
        row = by_name.get(name)
        if not row or row["cross_build_identical"] != "1":
            fail(f"demo manifest: expected byte-identical DOS/Windows replay {name}")
        if row["sha256_windows"] != row["sha256_dos"]:
            fail(f"demo manifest: hash mismatch for shared replay {name}")


def check_sprite_manifest() -> None:
    rows = read_csv("manifests/recovered_sprite_frames.csv")
    if len(rows) != 91:
        fail(f"sprite manifest: expected 91 recovered frame metadata rows, got {len(rows)}")
    bomb = [r for r in rows if r["asset"].lower() == "sights/bomb.jba"]
    if len(bomb) != 3 or any((r["sprite_width"], r["sprite_height"]) != ("1", "9") for r in bomb):
        fail("sprite manifest: expected three recovered 1x9 Bomb.jba frames")
    gemini = [r for r in rows if r["asset"].lower() in {"sights/gemini1.jba", "sights/gemini2.jba"}]
    if len(gemini) != 30 or sorted(int(r["frame_slot"]) for r in gemini) != list(range(30)):
        fail("sprite manifest: expected contiguous Gemini body slots 0..29 across Gemini1/2")
    gemhead = [r for r in rows if r["asset"].lower() == "sights/gemhead.jba"]
    if len(gemhead) != 1 or (gemhead[0]["sprite_width"], gemhead[0]["sprite_height"]) != ("43", "34"):
        fail("sprite manifest: expected one recovered 43x34 Gemhead frame")



def check_world_scenery_manifest() -> None:
    rows = read_csv("manifests/world_scenery_roles.csv")
    if len(rows) != 15:
        fail(f"world scenery manifest: expected 15 family/role rows, got {len(rows)}")
    by_key = {(r["family"], r["role"]): r for r in rows}
    expected_present = {"river", "desert"}
    for family in ("river", "desert", "isle", "house", "night"):
        for role in ("top", "middle", "bottom"):
            row = by_key.get((family, role))
            if not row:
                fail(f"world scenery manifest: missing {family}/{role}")
            should = "1" if family in expected_present else "0"
            if row["present_in_windows_shareware"] != should:
                fail(f"world scenery manifest: unexpected presence for {family}/{role}")
            if should == "1" and not re.fullmatch(r"[0-9a-f]{64}", row["sha256"]):
                fail(f"world scenery manifest: invalid hash for {family}/{role}")


def check_boss_resource_manifest() -> None:
    rows = read_csv("manifests/boss_resource_roles.csv")
    if len(rows) != 27:
        fail(f"boss resource manifest: expected 27 rows, got {len(rows)}")
    sha_re = re.compile(r"^[0-9a-f]{64}$")
    for row in rows:
        present = row["present_in_windows_shareware"]
        if present == "yes":
            if not row["relative_path"] or int(row["size"]) <= 0 or not sha_re.fullmatch(row["sha256"]):
                fail(f"boss resource manifest: invalid present resource metadata for {row['family']} {row['relative_path']}")
        elif present == "no":
            if row["size"] or row["sha256"]:
                fail(f"boss resource manifest: absent resource must not carry size/hash: {row['relative_path']}")
    unknown = [r for r in rows if r["family"] == "RegisteredSlot2Unknown"]
    if len(unknown) != 1 or unknown[0]["processed_drone_slots"] != "2":
        fail("boss resource manifest: missing registered slot-2 unresolved marker")
    gemini = [r for r in rows if r["family"] == "Gemini"]
    if len(gemini) != 5 or any(r["present_in_windows_shareware"] != "yes" for r in gemini):
        fail("boss resource manifest: Gemini resources must be present in canonical shareware")
    for family in ("Spidey", "Bomber"):
        fam = [r for r in rows if r["family"] == family]
        if not fam or any(r["present_in_windows_shareware"] != "no" for r in fam):
            fail(f"boss resource manifest: {family} registered resources must be recorded absent")

def check_gameplay_call_inventory() -> None:
    rows = read_csv("reverse/windows/gameplay_call_inventory.csv")
    if len(rows) != 77:
        fail(f"gameplay call inventory: expected 77 direct-call targets, got {len(rows)}")
    by_addr = {row["target_address"].lower(): row for row in rows}
    expected = {"0x00421ed0": 56, "0x00401660": 49}
    for address, count in expected.items():
        row = by_addr.get(address)
        if not row or int(row["static_call_sites"]) != count:
            fail(f"gameplay call inventory: unexpected count for {address}")
    boss_targets = {
        "0x00403650": "update_bomber_boss",
        "0x00404350": "initialize_bomber_boss",
        "0x00405000": "update_gemini_boss",
        "0x00405ef0": "initialize_gemini_boss",
        "0x00406cc0": "update_registered_boss_slot2",
        "0x00407980": "initialize_registered_boss_slot2",
        "0x00414d80": "update_spidey_boss",
        "0x00415ac0": "initialize_spidey_boss",
        "0x00416700": "update_lid_top_boss",
        "0x00417220": "initialize_lid_top_boss",
    }
    for address, name in boss_targets.items():
        row = by_addr.get(address)
        if not row or row.get("provisional_name") != name or int(row["static_call_sites"]) != 1:
            fail(f"gameplay call inventory: boss mapping mismatch for {address} -> {name}")
    if "0x00403640" in by_addr:
        fail("gameplay call inventory: stale Bomber update address 0x00403640 must not appear")



def check_gameplay_pacing_manifests() -> None:
    win = read_csv("reverse/windows/state2_pacing_sites.csv")
    dos = read_csv("reverse/dos/gameplay_pacing_sites.csv")
    if len(win) != 9:
        fail(f"Win32 pacing manifest: expected 9 state-2 pacing sites, got {len(win)}")
    if len(dos) != 10:
        fail(f"DOS pacing manifest: expected 10 gameplay/setup pacing sites, got {len(dos)}")

    win_by = {r["address"].lower(): r for r in win}
    dos_by = {r["address"].lower(): r for r in dos}
    direct_vblank = [r for r in win if r["mechanism"] == "DirectDraw WaitForVerticalBlank"]
    if len(direct_vblank) != 8:
        fail(f"Win32 pacing manifest: expected 8 direct state-2 vblank sites, got {len(direct_vblank)}")
    if win_by.get("0x0041144d", {}).get("ordinary_update") != "yes":
        fail("Win32 pacing manifest: QPC state-2 tail must be the ordinary-update pacing site")
    if dos_by.get("0x0007dc92", {}).get("ordinary_update") != "yes":
        fail("DOS pacing manifest: 0x0007DC92 must be the ordinary-update retrace site")
    if win_by["0x0041144d"]["corresponding_dos"] != "0x0007DC89 / 0x0007DC92":
        fail("Win32 pacing manifest: ordinary tail DOS correspondence changed")
    for waddr, daddr in {
        "0x0040bb44": "0x000777C0",
        "0x0040bcb5": "0x00077919",
        "0x0040bcec": "0x00077945",
        "0x0040bd77": "0x000779DA",
        "0x0040bf55": "0x00077BC6",
        "0x0040c4b9": "0x00078461",
        "0x0040c7ff": "0x0007880E",
        "0x00410739": "0x0007CB9A",
    }.items():
        if win_by.get(waddr, {}).get("corresponding_dos", "").lower() != daddr.lower():
            fail(f"pacing manifest: lost DOS/Win32 special-wait correspondence {waddr} -> {daddr}")


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
    check_fly_manifest()
    check_demo_manifest()
    check_sprite_manifest()
    check_world_scenery_manifest()
    check_boss_resource_manifest()
    check_gameplay_call_inventory()
    check_gameplay_pacing_manifests()
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
