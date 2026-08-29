#!/usr/bin/env python3
"""Verify the durable Drone Phase-4 complete-game-simulation exit contract.

Phase 4 closes on the canonical shareware gameplay architecture, not on exact
renderer/audio trace parity or registered-only content. Immutable asset data is
allowed at the session boundary; caller-supplied gameplay-semantic triggers are
not.
"""

from __future__ import annotations

import csv
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"phase4 exit check FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


with (ROOT / "reverse/ledger/open_questions.csv").open(newline="", encoding="utf-8") as f:
    questions = {row["id"]: row for row in csv.DictReader(f)}

bomb_question = questions.get("Q-BOMB-001")
if bomb_question is None:
    fail("required Q-BOMB-001 evidence row is missing")
if bomb_question.get("status", "").strip().lower() != "resolved":
    fail("Q-BOMB-001 still leaves shareware bomb steering externally gated")

# Open high-priority questions may remain only when their own ledger explicitly
# says they do not block canonical shareware simulation.
for row in questions.values():
    if row.get("status", "").strip().lower() == "resolved":
        continue
    if row.get("priority", "").strip().lower() != "high":
        continue
    blocking = row.get("blocking", "").strip().lower()
    if "retail" not in blocking and "full" not in blocking:
        fail(f"open high-priority question may still block shareware simulation: {row['id']}")

required_paths = [
    "include/drone/gameplay/game_session.hpp",
    "include/drone/gameplay/enemy_bomb.hpp",
    "include/drone/gameplay/trajectory_spawn.hpp",
    "include/drone/gameplay/trajectory_collision.hpp",
    "include/drone/gameplay/lid_top_boss.hpp",
    "include/drone/gameplay/gemini_boss.hpp",
    "include/drone/gameplay/player_death_effect.hpp",
    "include/drone/gameplay/post_game.hpp",
    "src/gameplay/game_session.cpp",
    "tests/test_game_session.cpp",
    "tests/test_game_session_post_game.cpp",
    "docs/GAME_SESSION.md",
    "docs/PHASE4.md",
    "docs/PHASE5.md",
]
missing = [rel for rel in required_paths if not (ROOT / rel).is_file()]
if missing:
    fail("required complete-simulation contracts missing: " + ", ".join(missing))

session_header = (ROOT / "include/drone/gameplay/game_session.hpp").read_text(encoding="utf-8")
context_start = session_header.index("struct GameSessionTargetContext")
context_end = session_header.index("};", context_start) + 2
session_target_context = session_header[context_start:context_end]
legacy_semantic_fields = [
    "redirect_bombs_to_attached_probe",
    "boss_destruction_triggers",
    "player_death_effect_inactive",
    "trajectory_hits",
    "trajectory_spawn_group",
    "trajectory_spawn_x_offset",
    "trajectory_spawn_y_offset",
]
for token in legacy_semantic_fields:
    if token in session_target_context:
        fail(f"legacy host gameplay-semantic field reintroduced in GameSessionTargetContext: {token}")

session_cpp = (ROOT / "src/gameplay/game_session.cpp").read_text(encoding="utf-8")
if "enemy_bombs_target_attached_probe(" not in session_cpp:
    fail("processed-count-dependent attached-Probe bomb steering is not session-owned")
if "campaign.mission.processed_count" not in session_cpp:
    fail("session does not expose native processed Drone count to gameplay integration")

findings = (ROOT / "reverse/ledger/findings.csv").read_text(encoding="utf-8")
if "WIN-BOMB-007" not in findings:
    fail("WIN-BOMB-007 processed-count steering evidence is missing")

roadmap = (ROOT / "docs/ROADMAP.md").read_text(encoding="utf-8")
if "## Phase 4 — Complete Game Simulation — COMPLETE" not in roadmap:
    fail("ROADMAP.md does not mark Phase 4 complete")
if "## Phase 5 — Audio Reconstruction — IN PROGRESS" not in roadmap:
    fail("ROADMAP.md has not advanced to Phase 5")

phase4 = (ROOT / "docs/PHASE4.md").read_text(encoding="utf-8")
if "**Status:** complete." not in phase4:
    fail("PHASE4.md is not marked complete")

status = (ROOT / "docs/STATUS.md").read_text(encoding="utf-8")
if "**Current engineering phase:** Phase 5 — Audio Reconstruction" not in status:
    fail("STATUS.md has not advanced the current engineering phase to Phase 5")

print(
    "phase4 exit OK: canonical shareware gameplay semantics are session-owned; "
    "legacy host trigger fields absent; Q-BOMB-001 resolved; roadmap advanced to Phase 5"
)
