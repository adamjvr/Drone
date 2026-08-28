# Continuous Game Session

**Phase:** 4 — Complete Game Simulation  
**Status:** continuous trajectory, normal/destructive Drone objective, and shareware boss integration established

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
- shareware-reachable boss lifecycle state for Lid/Top and Gemini;
- Drone objective X/Y, activity, completed-disarm status and Y=45 hold counter;
- pre-detonation countdown, logical detonation tick, captured detonation center and phase-0 destruction-settlement counter;
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

1. advance the pre-phase Drone destruction countdown and, at exact count 99, trigger activity 2, commit outcome 2 and apply the dedicated -1000/progress-reset consequence;
2. advance the shared four-phase state-2 substep, then the logical Drone detonation tick (capped at 330) and phase-2 settlement scalar;
3. settle a completed Drone destruction when its phase-0 effect-side field is >70, running the mission interstitial first when more than one life remains, then consuming the life and rebuilding/repositioning the encounter when applicable;
4. apply an explicitly selected live transient trajectory-group activation, when requested;
5. advance owned trajectory groups, stagger activation and mode-2 escape retirement;
6. run the post-trajectory logical detonation updater on phase 0, including captured-center drift, four presentation-spawn requests, tick-329 settlement reset and tick>329 settlement increments;
7. accept an exact completed-Probe-disarm event when supplied, then advance owned normal Drone travel/hover/settlement state; the exact 4200-tick unresolved hover starts the internal destructive countdown;
8. execute the recovered normal Drone interstitial/encounter-only transition when Y>230 and settlement reaches exactly 60;
9. derive the exact Drone Y=-200 boss boundary from owned position and select/advance Lid/Top or Gemini;
10. advance enemy-bomb spawn/respawn gate and rapid-missile cooldown;
11. apply semantic player movement and allocate rapid fire when allowed;
12. advance/load/cycle/launch/move Probe/Stinger state;
13. recharge/drain shield;
14. move/animate rapid missiles and existing enemy bombs, then retire offscreen projectiles;
15. dispatch already-proven trajectory hit events through damage, destruction, score and group teardown;
16. advance world scroll on phase 2;
17. convert at most one 500-point extra-life threshold, except after the explicit shareware EndRun transition;
18. publish a compact tick-result event summary.

This is the first continuous clean integration boundary. It does **not** yet
claim that every relative position of these narrow helper calls is a
line-for-line reconstruction of the original state-2 monolith. Proven
intra-subsystem order and shared phase cadence are preserved; remaining
encounter/collision producers are integrated in subsequent Phase-4 milestones.

## Explicit encounter context

The session accepts `GameSessionTargetContext` for facts produced by encounter systems that are deliberately kept outside mutable session ownership:

- the semantic “Probe decode/disarm completed” event;
- optional Stinger target geometry;
- the independently recovered but still unnamed condition that redirects enemy
  bombs toward an attached Probe;
- immutable trajectory path samples;
- an already-selected transient trajectory template to activate during the formation stage;
- exact sprite-mask collision hits already proven by the collision producer;
- already-validated Lid/Top or Gemini destruction transitions from their boss-local collision/damage producers.

This is intentional. Mutable trajectory lifecycle, normal Drone position/progression, the timeout-driven destructive countdown/detonation/life-loss state, and the persistent shareware boss lifecycle/score tails are now owned by the session. Random/template selection, immutable asset data, opaque-pixel collision detection, exact Probe decode production, the rapid-missile/Stinger collision producers that can also start the same Drone destruction countdown, direct framebuffer detonation rendering, and boss movement/attack/hit-validation remain separate producers until their recovered contracts are integrated. The boss destruction triggers are therefore *validated transitions*, not invented broad-phase hits. This avoids substituting rectangle collisions, guessed spawn policy or guessed vulnerability state for the original behavior.

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
- remaining non-trajectory enemy state and full boss geometry/movement/attack ownership;
- exact Lid/Top/Gemini boss-local collision/damage producers (the session consumes only their validated destruction transitions);
- exact Probe decode timing/producer that raises the completed-disarm event;
- exact rapid-missile/Stinger opaque-pixel collision producers that can start the already-owned Drone destruction countdown;
- randomized/direct-framebuffer Drone detonation presentation emitted from the owned logical effect events;
- death-effect/respawn continuity;
- post-game/results execution;
- construction of complete Phase-3 presentation inputs from the session.

Those are the remaining Phase-4 workstreams, not deficiencies hidden inside the
first `GameSession` API.
