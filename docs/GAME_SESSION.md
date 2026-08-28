# Continuous Game Session

**Phase:** 4 — Complete Game Simulation  
**Status:** first integration contract established

Phase 4 begins by replacing isolated gameplay-helper probes with one portable,
continuous session owner. `GameSession` is a clean architecture type; it is not
an ABI mirror of the original executable's global-memory layout.

## Ownership split

`GameSession` deliberately separates state that survives encounter transitions
from state rebuilt for each encounter.

### Campaign state

- lives / player lifecycle;
- total score and extra-life progress;
- six-Drone mission outcome ledger;
- high-score disqualification state;
- Mothership completion state;
- alien hit/total statistics used by post-game results.

### Encounter state

- player motion;
- shield accumulator/activity;
- rapid-missile pool and cooldown;
- Probe/Stinger state;
- enemy-bomb pool and shared spawn/respawn gate;
- four-phase gameplay substep;
- 320x600 world-scroll row;
- Drone settlement tick;
- encounter-local update count.

The session also carries a clean whole-run update counter for diagnostics. That
counter is not asserted to correspond to an original global.

## Reset semantics

`reset_game_session()` consumes the already-recovered
`GameplaySessionResetScope`:

- `FullCampaign` rebuilds both campaign and encounter state;
- `EncounterOnly` preserves score, lives, outcomes, post-game statistics and
  eligibility while rebuilding per-encounter state.

The encounter reset also reactivates the player when lives remain. This is a
clean normalization required because the earlier narrow `PlayerLifecycleState`
helper combines persistent lives with the current player-active flag.

## Active-gameplay tick

`step_game_session()` executes only while `GameState::ActiveGameplay` is active.
Modal/menu states do not advance cooldowns, world scroll, projectiles or timing.

The first integrated tick composes already-established atomic behavior:

1. advance the shared four-phase state-2 substep;
2. advance enemy-bomb spawn/respawn gate;
3. advance rapid-missile cooldown;
4. apply semantic player movement;
5. allocate rapid fire when allowed;
6. advance/load/cycle/launch/move Probe/Stinger state;
7. recharge/drain shield;
8. move/animate rapid missiles;
9. move/animate existing enemy bombs;
10. retire offscreen missiles/bombs;
11. advance world scroll on phase 2;
12. convert at most one 500-point extra-life threshold;
13. publish a compact tick-result event summary.

This is the first continuous clean integration boundary. It does **not** yet
claim that every relative position of these narrow helper calls is a
line-for-line reconstruction of the original state-2 monolith. Proven
intra-subsystem order and shared phase cadence are preserved; remaining
encounter/collision producers are integrated in subsequent Phase-4 milestones.

## Explicit encounter context

The session currently accepts `GameSessionTargetContext` for facts produced by
encounter systems that are not yet owned by the first integration milestone:

- Drone X for Probe homing/pinning;
- optional Stinger target geometry;
- the independently recovered but still unnamed condition that redirects enemy
  bombs toward an attached Probe.

This is intentional. Phase 4 will replace these inputs with reconstructed actor
collections as encounter integration proceeds, rather than inventing selection
or spawn rules early.

## Headless state oracle

`drone_session_probe` runs a deterministic 120-update semantic input script with
no original assets required:

```bash
./build-debug/drone_session_probe 120
```

The current oracle is:

```text
updates=120 total=120 phase=0 scroll=569 player=147,171,0 missiles=7 cooldown=7 fired=15 shield=2765900,0 shield_sfx=12 special=1,3,230,-34 bomb_gate=-330 lives=3 score=0,0
```

CTest compares this complete line exactly. Future Phase-4 integration that
intentionally changes session behavior must therefore update the oracle in the
same reviewed milestone rather than silently moving the whole-session state.

## Current boundary

Not yet session-integrated:

- trajectory-group actor collections and spawn producers;
- encounter-specific enemy/boss state ownership;
- collision/destruction dispatch across those collections;
- Drone resolution/encounter transition execution;
- death-effect/respawn continuity;
- post-game/results execution;
- construction of complete Phase-3 presentation inputs from the session.

Those are the remaining Phase-4 workstreams, not deficiencies hidden inside the
first `GameSession` API.
