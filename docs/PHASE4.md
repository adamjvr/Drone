# Phase 4 — Complete Game Simulation

**Status:** complete.

Phase 4 assembled the recovered Phase-2 gameplay contracts into a continuous portable simulation while consuming immutable data/presentation contracts from Phase 3. The exit boundary is intentionally gameplay-semantic: renderer/audio trace parity does not keep simulation architecture open.

## Exit contract achieved

`GameSession` now owns the canonical shareware run continuously, including:

- campaign-vs-encounter reset semantics and the original four-phase scheduler;
- player movement, shield, rapid missiles, Probe/Stinger lifecycle and exact Probe decode/disarm timing;
- enemy-bomb spawn gate, movement, processed-count-dependent attached-Probe steering, ordered special/player collision consequences, lethal entry, native death-effect lifecycle and deferred respawn/game-over settlement;
- all 17 trajectory groups, persistent primary replenishment, transient spawn timing/template/path randomness, native weapon collisions and encounter/mission alien accounting quirks;
- the six-Drone objective ledger, normal disarm settlement, timeout/destructive countdown, -1000 detonation commit, exact update-side 17-draw detonation effect RNG and life-loss/restart ordering;
- automatic shareware boss dispatch plus native Lid/Top and Gemini movement, attacks, vulnerabilities, damage, score and retirement;
- stateful Stinger target priority/retention for shareware-native candidates and the X=160 load-time dummy target;
- shareware level/encounter transitions and the exact processed-count-2 termination boundary;
- lives<=0 Results entry, the 58-presentation no-confirm lock, Ordering Information, high-score qualification/handoff, credits and final raw state outcomes.

The formerly external bomb steering fact is resolved by `WIN-BOMB-007`: `0x00433B54` is `drone_outcome_processed_count`, so bombs target an attached Probe only when `processed_count > 1` and the Probe is in activity 2.

## Intentional boundaries after Phase 4

These do **not** reopen complete-game simulation:

- immutable FLY/JBA-derived path or sprite-mask data supplied by the asset layer;
- registered-only boss/enemy families (Spidey, Bomber, dispatch slot 2, Mothership completion) — Phase 8 after lawful retail evidence;
- randomized player-death debris/audio/pixels, render-time Drone radial noise and Gemini presentation RNG — exact trace/presentation fidelity work in Phases 5–7;
- interactive score-name UI and legacy score-file persistence — host/UI/persistence work;
- exact original-runtime frame/audio/state trace comparison — Phase 6;
- end-to-end discrepancy closure — Phase 7.

## Durable exit gate

`scripts/check_phase4_exit.py` prevents roadmap regression. It requires the native session contracts, verifies Q-BOMB-001 is resolved, rejects reintroduction of legacy host semantic fields (`redirect_bombs_to_attached_probe`, `boss_destruction_triggers`, `player_death_effect_inactive`, external `trajectory_hits`/trajectory spawn selection), and requires the roadmap/status to advance to Phase 5. The complete CTest suite, research metadata checks, Phase-2/3/4 exits, Linux fidelity capture and exact session oracle remain the executable validation boundary.
