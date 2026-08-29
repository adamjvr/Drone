#!/usr/bin/env python3
"""Verify the durable Drone Phase-3 renderer/world exit contract.

Phase 3 closes when the original presentation pipeline is evidence-backed and
portable enough for deterministic composition/reference-frame comparison.
Actual original-runtime trace parity belongs to Phase 6, complete game
simulation to Phase 4, retail-only content to Phase 8, and production platform
hardening to Phase 12.
"""

from __future__ import annotations

import csv
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"phase3 exit check FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


with (ROOT / "reverse/ledger/open_questions.csv").open(newline="", encoding="utf-8") as f:
    questions = {row["id"]: row for row in csv.DictReader(f)}

for qid in ("Q-RENDER-001", "Q-JBA-002"):
    row = questions.get(qid)
    if row is None:
        fail(f"required research question missing: {qid}")
    if row.get("status", "").strip().lower() != "resolved":
        fail(f"Phase-3 blocker is not resolved: {qid}")

required_paths = [
    "include/drone/fidelity/indexed_framebuffer.hpp",
    "include/drone/fidelity/palette_effects.hpp",
    "include/drone/fidelity/presentation_order.hpp",
    "include/drone/fidelity/scaled_overlay_presentation.hpp",
    "include/drone/fidelity/world_presentation_subpasses.hpp",
    "include/drone/fidelity/hud_presentation.hpp",
    "include/drone/fidelity/framebuffer_snapshot.hpp",
    "include/drone/fidelity/host_capture.hpp",
    "docs/reverse/PALETTE_EFFECTS.md",
    "docs/reverse/PRESENTATION_ORDER.md",
    "docs/reverse/SCALED_OVERLAYS.md",
    "docs/reverse/WORLD_PRESENTATION_SUBPASSES.md",
    "docs/reverse/HUD_PRESENTATION.md",
    "docs/FRAMEBUFFER_VALIDATION.md",
    "docs/LINUX_FIDELITY_HOST.md",
    "docs/PHASE3.md",
    "docs/PHASE4.md",
]
missing = [rel for rel in required_paths if not (ROOT / rel).is_file()]
if missing:
    fail("required renderer/world contracts missing: " + ", ".join(missing))

presentation_header = (ROOT / "include/drone/fidelity/presentation_order.hpp").read_text(encoding="utf-8")
if "canonical_win32_presentation_pass_count = 19" not in presentation_header:
    fail("corrected 19-pass Win32 presentation contract is not active")

roadmap = (ROOT / "docs/ROADMAP.md").read_text(encoding="utf-8")
if "## Phase 3 — Rendering & World Reconstruction — COMPLETE" not in roadmap:
    fail("ROADMAP.md does not mark Phase 3 complete")
if "## Phase 4 — Complete Game Simulation" not in roadmap:
    fail("ROADMAP.md has not advanced beyond Phase 3")

status = (ROOT / "docs/STATUS.md").read_text(encoding="utf-8")
if "**Current engineering phase:** Phase 2 — Gameplay Reconstruction" in status or \
   "**Current engineering phase:** Phase 3 — Rendering & World Reconstruction" in status:
    fail("STATUS.md has not advanced beyond Phase 3")

phase3 = (ROOT / "docs/PHASE3.md").read_text(encoding="utf-8")
if "**Status:** complete." not in phase3:
    fail("PHASE3.md is not marked complete")

print(
    "phase3 exit OK: renderer/world contracts recovered; "
    "19-pass composition corrected; framebuffer validation boundary present; "
    "roadmap advanced to Phase 4"
)
