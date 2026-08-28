# Phase 4 — Complete Game Simulation

**Status:** in progress.

Phase 4 assembles the independently recovered Phase-2 gameplay contracts and
Phase-3 presentation contracts into the complete clean game simulation. The
focus is not new renderer archaeology and not remaster features: it is making
one portable session own the original gameplay state and drive the validated
fidelity presentation boundary continuously.

## Inherited baseline

Phase 4 begins with:

- the user-facing game-state protocol and four-phase gameplay scheduler;
- semantic keyboard/joystick/demo input boundary;
- player movement/lifecycle, rapid missiles, shield and Probe/Stinger behavior;
- enemy bombs, collision helpers, scoring, lives and high scores;
- 17 trajectory-group templates and normal trajectory lifecycle;
- mission/Drone outcome progression and post-game control flow;
- recovered boss-family dispatch and multiple encounter slices;
- 320x200 indexed fidelity framebuffer, world compositor, sprite/scaled paths,
  dynamic palettes, HUD, effects and corrected 19-pass presentation order;
- `DRONEFB1` framebuffer validation and Linux display-free capture tooling.

## Current Phase-4 work

1. **Continuous session ownership — ESTABLISHED FIRST CONTRACT** — `GameSession` now separates campaign and encounter state, implements full-vs-encounter reset, and continuously composes the recovered player/weapon/shield/bomb/cadence/scroll/extra-life helpers. A deterministic asset-free 120-update session oracle is under CTest.
2. **Whole-frame simulation integration — IN PROGRESS** — the session now owns all 17 trajectory groups, persistent group-0 replenishment, live transient formation timing/selection/randomization, encounter-local trajectory hit/total accounting plus the recovered source-sensitive mission-statistics double counts and interstitial fold, blue-Probe attachment and exact two-stage decode/disarm timing, the per-slot enemy-bomb late collision loop for Probe/Stinger and player, shield absorption, lethal player-destruction entry, the -540 bomb quiet period and deferred player life-settlement/respawn gate, rapid-missile/Stinger Drone-hit countdown producers, the canonical normal Drone objective position/travel/hover/disarm/settlement path, and the destructive timeout/countdown/detonation/life-loss path, plus automatic Y=-200 shareware boss dispatch and the recovered lifecycle/score tails for Lid/Top and Gemini. It executes both normal and detonated count-1 Gemini continuation, count-2 shareware Results/EndRun transitions, the exact detonation tick/effect-settlement gates, and original life-decrement ordering. The session also owns the exact stateful red-Stinger target-priority/retention chain and load-time X=160 dummy reset. The player death-effect activity remains an explicit presentation-side fact; trajectory sprite-mask collision producers, non-owned hostile geometry/AI feeding the Stinger selector, direct death/detonation visuals, boss movement/attacks and boss-local hit validation remain explicit producers while the remaining non-trajectory actors are integrated.
3. **Encounter completeness** — fill the remaining shareware-reachable enemy,
   scripted-event and boss interactions required for continuous play while
   keeping registered-only unknowns in Phase 8.
4. **Death/restart/game-over continuity — PARTIAL** — player bomb death entry and deferred life-consumption/respawn/game-over gating are session-integrated; reconstruct the death presentation producer itself and complete remaining results/restart execution rather than leaving those paths as standalone helpers.
5. **Fidelity-present handoff** — produce the Phase-3 semantic presentation
   inputs from session state every frame and keep renderer/host concerns outside
   the simulation core.

## Non-goals

- Exact original-runtime frame/audio/state trace parity (Phase 6).
- Declaring end-to-end shareware parity before discrepancy work (Phase 7).
- Registered-only content without a lawful full-release evidence set (Phase 8).
- Modern renderer/UX/platform production features (Phases 11–14).

## Exit direction

Phase 4 should leave the clean engine able to execute the complete reconstructed
shareware simulation continuously—from game start through encounters,
death/restart and game-over/results—without calling the original executable.
Later phases measure exact parity, finish audio, and drive discrepancies toward
zero.

## First Phase-4 integration milestone

The evolving `GameSession` contract is documented in [`GAME_SESSION.md`](GAME_SESSION.md). The session now owns trajectory encounter state, the exact Probe attach/decode/disarm chain plus enemy-bomb knockoff/phase-2 decoder interruption, rapid-missile/Stinger Drone-hit producers, normal and destructive Drone objective execution through life loss/restart or EndRun, automatic shareware boss dispatch, and the established Lid/Top/Gemini lifecycle/score tails. Trajectory sprite-mask hits, non-owned hostile geometry/AI feeding the now-native Stinger selector, direct detonation framebuffer presentation, boss movement/attacks, exact boss-local collision validation and broader post-game continuity remain separate recovered producers or later Phase-4 milestones; persistent primary replenishment, live transient trajectory template/timing randomness, and encounter-to-mission alien-statistics accounting are now session-owned.
