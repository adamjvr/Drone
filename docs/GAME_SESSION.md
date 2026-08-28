# Continuous Game Session

**Phase:** 4 — Complete Game Simulation  
**Status:** continuous trajectory, Probe decode/Drone weapon entry, normal/destructive Drone objective, and shareware boss integration established

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
- Probe/Stinger state plus exact attached-Probe decode/disarm counters/status;
- enemy-bomb pool and shared spawn/respawn gate;
- all 17 fixed trajectory groups and their inline actor state;
- shareware-reachable boss lifecycle state for Lid/Top and Gemini;
- Drone objective X/Y, activity, completed-disarm status and Y=45 hold counter;
- pre-detonation countdown, logical detonation tick, captured detonation center and phase-0 destruction-settlement counter;
- four-phase gameplay substep;
- 320x600 world-scroll row;
- Drone settlement tick;
- encounter-local update count.

The session also carries a clean whole-run update counter for diagnostics and a seedable process-lifetime `OriginalRandomState` implementing the exact Win32/MSVC 15-bit CRT generator. Runtime difficulty/demo options and PRNG state deliberately survive encounter-only rebuilds; neither is an original raw-global ABI mirror. The diagnostic update/draw counters are not asserted to correspond to original globals.

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
4. advance an already-attached Probe decoder before normal Drone movement: status 0 phase-1 timing, same-update fallthrough into status 3 phase 2, status 1 completion, +500 score/progress and the required completion PRNG draw; a completion marks the owned Drone disarmed immediately;
5. apply an explicitly selected live transient trajectory-group activation, when requested;
6. advance owned trajectory groups, stagger activation and mode-2 escape retirement;
7. run the post-trajectory logical detonation updater on phase 0, including captured-center drift, four presentation-spawn requests, tick-329 settlement reset and tick>329 settlement increments;
8. advance owned normal Drone travel/hover/settlement state; completed decode releases the Y=45 hold in the same update, and the exact 4200-tick unresolved hover starts the internal destructive countdown; after Y>230 the session clears completed decode state as the original does;
9. execute the recovered normal Drone interstitial/encounter-only transition when Y>230 and settlement reaches exactly 60;
10. derive the exact Drone Y=-200 boss boundary from owned position and select/advance Lid/Top or Gemini;
11. advance enemy-bomb spawn/respawn gate and rapid-missile cooldown;
12. apply semantic player movement and allocate rapid fire when allowed;
13. advance/load/cycle/launch/move Probe/Stinger state;
14. recharge/drain shield;
15. move/animate rapid missiles and existing enemy bombs, then retire offscreen projectiles;
16. test rapid missiles first and the launched Probe/Stinger second against the active Drone with the recovered inclusive point-vs-12×32-hitbox primitive; a missile/Stinger starts the destruction countdown while a blue Probe attaches, awards +10 and initializes exact live/demo decode thresholds;
17. dispatch already-proven trajectory hit events through damage, destruction, score and group teardown;
18. advance world scroll on phase 2;
19. convert at most one 500-point extra-life threshold, except after the explicit shareware EndRun transition;
20. publish a compact tick-result event summary.

This is the first continuous clean integration boundary. It does **not** yet
claim that every relative position of these narrow helper calls is a
line-for-line reconstruction of the original state-2 monolith. Proven
intra-subsystem order and shared phase cadence are preserved; remaining
encounter/collision producers are integrated in subsequent Phase-4 milestones.

## Explicit encounter context

The session accepts `GameSessionTargetContext` for facts produced by encounter systems that are deliberately kept outside mutable session ownership:

- optional Stinger target geometry;
- the independently recovered but still unnamed condition that redirects enemy
  bombs toward an attached Probe;
- immutable trajectory path samples;
- an already-selected transient trajectory template to activate during the formation stage;
- exact sprite-mask collision hits already proven for trajectory actors;
- already-validated Lid/Top or Gemini destruction transitions from their boss-local collision/damage producers.

This is intentional. Mutable trajectory lifecycle, the exact Probe attachment/decode/disarm chain, normal Drone position/progression, rapid-missile/Stinger Drone-hit entry producers, the timeout-driven destructive countdown/detonation/life-loss state, and the persistent shareware boss lifecycle/score tails are now owned by the session. The Drone weapon producers do **not** use sprite masks: the executable calls `0x00401F60`, whose inclusive collision extent for the 15×38 Drone is 12×32. Random trajectory/template selection, immutable asset data, trajectory opaque-pixel collision detection, attached-Probe vulnerability/enemy collision producers, hostile Stinger target selection, direct framebuffer detonation rendering, and boss movement/attack/hit-validation remain separate producers until their recovered contracts are integrated. Boss destruction triggers are therefore *validated transitions*, not invented broad-phase hits.

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
- the enemy collision path that can shoot an attached Probe off and its exact decoder-reset consequences;
- hostile Stinger target-selection priority and the remaining non-Drone special-weapon collision consequences;
- randomized/direct-framebuffer Drone detonation presentation emitted from the owned logical effect events;
- death-effect/respawn continuity;
- post-game/results execution;
- construction of complete Phase-3 presentation inputs from the session.

Those are the remaining Phase-4 workstreams, not deficiencies hidden inside the
first `GameSession` API.
