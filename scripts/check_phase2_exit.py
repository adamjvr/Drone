#!/usr/bin/env python3
"""Verify the durable Drone Phase-2 exit contract.

This gate intentionally checks architecture/research closure, not later parity.
Phase 2 is complete when no critical research question still blocks simulation
architecture, the recovered clean gameplay contracts are present, and roadmap
state has advanced to Phase 3. Rendering completeness, full-game simulation,
retail-only evidence, and trace parity belong to later phases.
"""

from __future__ import annotations

import csv
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"phase2 exit check FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


questions_path = ROOT / "reverse/ledger/open_questions.csv"
with questions_path.open(newline="", encoding="utf-8") as f:
    questions = list(csv.DictReader(f))

open_critical = [
    row for row in questions
    if row.get("priority", "").strip().lower() == "critical"
    and row.get("status", "").strip().lower() != "resolved"
]
if open_critical:
    ids = ", ".join(row.get("id", "<unknown>") for row in open_critical)
    fail(f"critical architecture questions remain open: {ids}")

required_paths = [
    "include/drone/gameplay/game_state.hpp",
    "include/drone/gameplay/input.hpp",
    "include/drone/gameplay/trajectory.hpp",
    "include/drone/gameplay/trajectory_templates.hpp",
    "include/drone/gameplay/gameplay_phase.hpp",
    "include/drone/gameplay/gameplay_update_order.hpp",
    "include/drone/gameplay/world_scroll.hpp",
    "include/drone/gameplay/collision.hpp",
    "include/drone/gameplay/mission_progression.hpp",
    "include/drone/gameplay/post_game.hpp",
    "docs/reverse/ENTITY_LAYOUT.md",
    "docs/PHASE2.md",
    "docs/PHASE3.md",
]
missing = [rel for rel in required_paths if not (ROOT / rel).is_file()]
if missing:
    fail("required recovered contracts missing: " + ", ".join(missing))

roadmap = (ROOT / "docs/ROADMAP.md").read_text(encoding="utf-8")
if "## Phase 2 — Gameplay Reconstruction — COMPLETE" not in roadmap:
    fail("ROADMAP.md does not mark Phase 2 complete")
if "## Phase 3 — Rendering & World Reconstruction — IN PROGRESS" not in roadmap:
    fail("ROADMAP.md does not mark Phase 3 in progress")

status = (ROOT / "docs/STATUS.md").read_text(encoding="utf-8")
if "**Current engineering phase:** Phase 3 — Rendering & World Reconstruction" not in status:
    fail("STATUS.md has not advanced the current engineering phase to Phase 3")

phase2 = (ROOT / "docs/PHASE2.md").read_text(encoding="utf-8")
if "**Status:** complete." not in phase2:
    fail("PHASE2.md is not marked complete")

print(
    "phase2 exit OK: no unresolved critical architecture questions; "
    "core gameplay contracts present; roadmap advanced to Phase 3"
)
