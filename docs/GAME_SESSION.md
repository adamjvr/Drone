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
- high-score disqualification and Results/Ordering suppression state;
- Mothership completion state;
- mission-wide alien hit/total statistics used by post-game results, including the original live-accounting quirks plus later encounter folds.

### Encounter state

- player motion;
- native 42×38 player-death explosion singleton/activity/frame lifecycle;
- shield accumulator/activity;
- rapid-missile pool and cooldown;
- Probe/Stinger state plus exact attached-Probe decode/disarm counters/status and the persistent red-Stinger target-selection state;
- enemy-bomb pool and shared spawn/respawn gate;
- all 17 fixed trajectory groups and their inline actor state;
- encounter-local alien hit/total accounting (`0x0047EC3C` / `0x00466B04` semantic owners), initialized to 0/7 and updated on proven trajectory destruction/insertion paths;
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
- the persisted ten-entry high-score table lives at the `GameSession` level rather
  than inside campaign state, so a `FullCampaign` gameplay reset does not erase
  scores loaded by the host; the inline post-game modal runtime itself does reset.

The encounter reset also reactivates the player when lives remain. This is a
clean normalization required because the earlier narrow `PlayerLifecycleState`
helper combines persistent lives with the current player-active flag.

## Active-gameplay tick

`step_game_session()` executes only while `GameState::ActiveGameplay` is active.
Modal/menu states do not advance cooldowns, world scroll, projectiles or timing.

The integrated tick composes already-established atomic behavior:

1. advance the pre-phase Drone destruction countdown and, at exact count 99, trigger activity 2, commit outcome 2 and apply the dedicated -1000/progress-reset consequence;
2. advance the shared four-phase state-2 substep, then the logical Drone detonation tick (capped at 330) and phase-2 settlement scalar;
3. settle a completed Drone destruction when its phase-0 effect-side field is >70, running the mission interstitial first when more than one life remains; that interstitial snapshots encounter hit/miss/total/percentage, folds the full encounter pair into the mission pair, then the caller consumes the life and rebuilds/repositions the encounter when applicable;
4. advance an already-attached Probe decoder before normal Drone movement: status 0 phase-1 timing, same-update fallthrough into status 3 phase 2, status 1 completion, +500 score/progress and the required completion PRNG draw; phase-1 completion emits the exact `drone.wav` volume-60 control, phase-2 completion stops/rewinds that loop, and a completion marks the owned Drone disarmed immediately;
5. when immutable trajectory paths are supplied, run the native persistent group-0 replenisher on non-phase-2 substeps, preserving its initial RNG draw, forced-roll cases, first-inactive-slot selection, three-way entry position and activity-3 path reacquisition; successful insertion increments the encounter-local alien total;
6. run the native live phase-2 transient formation scheduler: advance/force the recovered interval, consume the exact CRT spawn roll, select a free fixed group with progression-dependent pool rules, apply runtime path/group/formation randomization, and activate mode 2 when all gates pass; successful first-actor insertion increments the same encounter-local total;
7. advance owned trajectory groups, stagger activation and mode-2 escape retirement; newly staggered non-primary actors increment both the encounter-local total and the mission-wide total immediately, preserving the original live double-count quirk before the later whole-encounter fold;
8. run the post-trajectory detonation updater on phase 0, including captured-center drift, four center-scatter plus four radial-ring explosion requests, the exact 17-draw CRT RNG sequence, tick-329 settlement reset and tick>329 settlement increments;
9. advance owned normal Drone travel/hover/settlement state together with its parameterized `drone.wav` control: Y=-117 starts the loop at volume 0, eligible phase-2 approach updates ramp the persistent scalar toward 80, completed decode releases the Y=45 hold in the same update, and the exact 4200-tick unresolved hover stops the loop and starts the internal destructive countdown; after Y>230 the session clears completed decode state as the original does;
10. execute the recovered normal Drone interstitial/encounter-only transition when Y>230 and settlement reaches exactly 60, snapshotting encounter hit/miss/total/percentage and folding the complete encounter pair into the mission pair before encounter-only reset;
11. derive the exact Drone Y=-200 boss boundary from owned position and select/advance Lid/Top or Gemini;
12. advance enemy-bomb spawn/respawn gate and rapid-missile cooldown, advance the native player-death explosion on substep phase 2, then settle a previously destroyed player only when the gate is above -356, that singleton is inactive, the player remains inactive, lives remain, and Drone activity is not 2;
13. apply semantic player movement and allocate rapid fire when allowed;
14. advance/load/cycle/launch/move Probe/Stinger state; successful load resets the red-Stinger target to the original X=160 dummy, while frame-1 movement runs the recovered Mothership > Gemini > Lid/Top > Spidey > registered-slot2 > Bomber > unresolved-dynamic-hostile priority chain and retains the prior target when no branch qualifies;
15. recharge/drain shield;
16. move/animate rapid missiles and existing enemy bombs, then retire offscreen projectiles;
17. run the exact per-slot late bomb collision loop: test Probe/Stinger first with bomb `(x,y+9)` against its inclusive 2x6 hitbox, then test that same bomb coordinate against the active player's inclusive 18x18 hitbox even when a special hit already cleared bomb activity; a phase-2 attached-Probe interruption restores `drone.wav` volume 80 before its Probe impact SFX; shielded player hits are absorbed, while unshielded hits auto-launch a merely loaded special, deactivate the player, request the death presentation and drive the shared bomb gate to -540;
18. test rapid missiles first and the launched Probe/Stinger second against the active Drone with the recovered inclusive point-vs-12×32-hitbox primitive; a missile/Stinger stops/rewinds `drone.wav` and starts the destruction countdown, while a blue Probe leaves the loop running, attaches, awards +10 and initializes exact live/demo decode thresholds;
19. produce trajectory weapon collisions natively: rapid missiles sample the actor current-frame opaque mask and add +3; the six-frame Stinger display applies +15 on frames 3..5 with local-only hit accounting; the captured late state-3 Probe/Stinger scan uses point-vs-actor-hitbox and direct destruction even after a same-update Drone collision changed special activity;
20. advance world scroll on phase 2;
21. convert at most one 500-point extra-life threshold, except after the explicit shareware EndRun transition;
22. publish a compact tick-result event summary.

`GameSession::original_audio` also carries the process-lifetime `0x00440278` Drone-volume scalar. It is not reset with encounter/campaign state; the exact Y=-117 start path writes zero before the first live ramp. Audio events remain semantic and asset-free: `SetVolume` carries the original 0..100 game-scale value while the eventual backend owns conversion/mixing.

This is the first continuous clean integration boundary. It does **not** yet
claim that every relative position of these narrow helper calls is a
line-for-line reconstruction of the original state-2 monolith. Proven
intra-subsystem order and shared phase cadence are preserved; remaining
encounter/collision producers are integrated in subsequent Phase-4 milestones.

## Explicit encounter context

The session accepts `GameSessionTargetContext` for facts produced by encounter systems that are deliberately kept outside mutable session ownership:

The processed-Drone bomb redirect is **not** one of those facts anymore: `0x00433B54` is the native mission processed-count, so `processed_count > 1 && ProbeAttachedDecoding` is derived internally.


- Stinger target-candidate activity/geometry facts for hostile actors whose movement/geometry is not yet session-owned; selection priority and retained target state themselves are owned by the session;
- immutable trajectory path samples;
- the registered-only Mothership destruction-state fact that suppresses the live trajectory producer until that encounter is session-owned;
- immutable current-frame trajectory sprite-mask pixels used by the native rapid-missile producer;
- immutable `top.jba` frame pixels used by native Lid/Top rapid-missile opaque collision;
- immutable Gemini body/head current-frame pixels used by native boss opaque-pixel Probe/Stinger collision;

This is intentional. Mutable trajectory lifecycle, encounter-local alien hit/total statistics plus their source-sensitive mission accounting/fold, the exact Probe attachment/decode/disarm chain, ordered enemy-bomb collision ownership for Probe/Stinger and player, lethal player entry plus the shared -540 quiet period, deferred life consumption/respawn/game-over settlement, normal Drone position/progression, rapid-missile/Stinger Drone-hit entry producers, the timeout-driven destructive countdown/detonation/life-loss state, and the persistent shareware boss lifecycle/score tails are now owned by the session. The Drone weapon producers do **not** use sprite masks: the executable calls `0x00401F60`, whose inclusive collision extent for the 15×38 Drone is 12×32. Immutable trajectory path/mask asset data remains external, but rapid-missile opaque-pixel, launched-special point-hitbox, and Stinger-display broad-phase trajectory collision production are now session-owned. Non-owned hostile actor geometry/AI and the separate render-time Drone-detonation radial-noise/framebuffer pass remain separate producers; update-side explosion requests and their CRT RNG consumption are native. The player-death singleton lifecycle is now native; only its immutable pixels plus randomized debris/audio presentation remain fidelity-side. Both shareware bosses are native: Lid/Top owns root geometry/movement, bomb attacks, lid animation/vulnerability and missile/Stinger hit validation; Gemini owns shared-root movement, opposing body animation, bomb attacks, native head/body Probe/Stinger collision, asymmetric damage thresholds and independent destruction tails. Live transient trajectory timing/template selection and its CRT-random path/offset variation are now native. Red-Stinger target **selection** is now native; only candidate facts for actors not yet owned by the session remain external. No shareware boss-destruction trigger remains in the session boundary; immutable boss mask pixels and presentation-side effects remain explicit fidelity inputs.

## Native post-game continuity

When campaign lives are non-positive, `step_game_session()` diverts before
ordinary gameplay/RNG advancement and materializes the recovered
`Win32PostGamePlan` from session-owned mission/statistics/eligibility state and
the persisted score table. `PostGameRuntimeState` then owns the synchronous
modal sequence without inventing a nonexistent Results `GameState`:

- Results remains inline under raw state 2;
- 58 completed present/vblank iterations occur before confirmation is polled;
- the first confirmation opportunity is a distinct following modal iteration;
- Ordering Information writes raw state 7;
- qualifying score handoff writes raw state 8;
- completion credits follow the recovered perfect-run predicate;
- the outer tail lands in the established state 1/state 4 result.

`step_game_session_post_game()` accepts only modal completion/input facts from
the host. It now also returns semantic audio start/stop events for the recovered
Results -> Ordering -> Credits ownership transitions. It does not render original
assets, execute the credits fade envelope, edit names or write the legacy `scores`
file. Those remain presentation/mixer/persistence boundaries rather than gameplay
control-flow inputs.

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

- loading immutable extracted trajectory frame masks from original assets for the now-native rapid-missile opaque-pixel producer;
- remaining non-trajectory enemy state and registered-only boss geometry/AI;
- exact presentation-side Gemini particle/beam RNG consumption, while gameplay movement/bombs/damage/destruction are native;
- randomized Lid/Top debris/audio presentation and immutable top.jba pixels for its rapid-missile opaque-mask test;
- remaining non-Drone special-weapon collision consequences and non-owned hostile actor geometry/AI feeding the now-native Stinger selector;
- render-time Drone detonation radial-noise/direct-framebuffer presentation; update-side explosion request geometry and its 17-draw RNG stream are already native;
- reconstruction of the player-death randomized debris/audio/pixel presentation around the now-native singleton explosion actor;
- post-game pixel presentation, exact credits fade execution, interactive high-score name editing and score-file persistence; the Results/Ordering/credits semantic audio lifetime and modal control flow are session-owned;
- construction of complete Phase-3 presentation inputs from the session.

Those are intentional post-Phase-4 boundaries: audio/presentation integration and shared-RNG trace parity continue through Phases 5–7, while registered-only hostile actors and bosses remain Phase-8 reconstruction work. They are not missing canonical-shareware gameplay semantics hidden inside the `GameSession` API.


## Audio

Phase 5 emits allocation-free semantic audio events from exact gameplay and post-game ownership boundaries. Ordered late-bomb special/player-hit calls are produced inside the per-slot collision loop; exact trajectory/Gemini/Lid-Top impact sites advance the process-global `0x0042EFD8` `explode2/explode2/explode3/explode4` cycle. The two shareware-reachable boss loops are now session-owned as well: Lid/Top starts `retro1.wav` at activation and stops it on the exposed-core Stinger destruction transition, while Gemini starts `gemini.wav` at activation and stops it only when the last activity-1 side crosses its damage threshold. Results emits the exact selected one-shot cue; Ordering Information and completion credits emit loop-owned start/stop events. The DirectSound reverse layer also catalogs 13 proven flags-1 call sites (8 literal + 5 register-proven). `GameSession::original_audio` survives campaign/encounter resets, matching the original process-global lifetime.
