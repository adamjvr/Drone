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
- all 17 fixed trajectory groups and their inline actor state;
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

The integrated tick composes already-established atomic behavior:

1. advance the shared four-phase state-2 substep;
2. apply an explicitly selected live transient trajectory-group activation, when requested;
3. advance owned trajectory groups, stagger activation and mode-2 escape retirement;
4. advance enemy-bomb spawn/respawn gate;
5. advance rapid-missile cooldown;
6. apply semantic player movement;
7. allocate rapid fire when allowed;
8. advance/load/cycle/launch/move Probe/Stinger state;
9. recharge/drain shield;
10. move/animate rapid missiles;
11. move/animate existing enemy bombs;
12. retire offscreen missiles/bombs;
13. dispatch already-proven trajectory hit events through damage, destruction, score and group teardown;
14. advance world scroll on phase 2;
15. convert at most one 500-point extra-life threshold;
16. publish a compact tick-result event summary.

This is the first continuous clean integration boundary. It does **not** yet
claim that every relative position of these narrow helper calls is a
line-for-line reconstruction of the original state-2 monolith. Proven
intra-subsystem order and shared phase cadence are preserved; remaining
encounter/collision producers are integrated in subsequent Phase-4 milestones.

## Explicit encounter context

The session accepts `GameSessionTargetContext` for facts produced by encounter systems that are deliberately kept outside mutable session ownership:

- Drone X for Probe homing/pinning;
- optional Stinger target geometry;
- the independently recovered but still unnamed condition that redirects enemy
  bombs toward an attached Probe;
- immutable trajectory path samples;
- an already-selected transient trajectory template to activate during the formation stage;
- exact sprite-mask collision hits already proven by the collision producer.

This is intentional. Mutable trajectory lifecycle is now owned by the session, while random/template selection, immutable asset data and opaque-pixel collision detection remain separate producers until their own recovered encounter contracts are integrated. This avoids substituting rectangle collisions or guessed spawn policy for the original behavior.

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

- the live random/template-selection producer that chooses which inactive trajectory group to activate;
- exact opaque-pixel collision detection feeding trajectory hit events;
- encounter-specific non-trajectory enemy/boss state ownership;
- Drone resolution/encounter transition execution;
- death-effect/respawn continuity;
- post-game/results execution;
- construction of complete Phase-3 presentation inputs from the session.

Those are the remaining Phase-4 workstreams, not deficiencies hidden inside the
first `GameSession` API.
