# Win32 post-game / results flow

**Status:** Phase 2 control-flow contract established for the canonical Win32 shareware executable.

This document partitions the inline post-game region beginning at `0x004115BE` inside `game_dispatch_update` (`0x0040BA50`). The region is entered directly from active state 2 when `player_lives <= 0`; it is not a separate top-level game state.

The clean semantic model is implemented in:

- `include/drone/gameplay/post_game.hpp`
- `src/gameplay/post_game.cpp`
- `tests/test_gameplay.cpp`
- `include/drone/gameplay/game_session.hpp`
- `src/gameplay/game_session.cpp`
- `tests/test_game_session_post_game.cpp`

The Phase-2 planner composes the already-established mission-outcome and high-score modules rather than reproducing the original global-memory UI implementation. Phase 4 now also binds that plan into `GameSession` through a persistent modal runtime, so the host no longer decides whether/when Results, Ordering Information, high scores or completion credits follow a depleted-life run.

## Entry and presentation suppression

Near the beginning of state 2, the original reads `player_lives` (`0x0042B1AC`) and branches to `0x004115BE` when the value is non-positive.

Byte `0x0042F1FC` is an independent **post-game presentation-suppression flag**. Normal session initialization clears it. Explicit gameplay-abort paths, including the confirmed quit path and demo terminal handling, set it before routing toward the post-game tail.

When the flag is nonzero, the post-game region skips:

1. the result-screen presentation; and
2. the shareware Ordering Information modal.

It does **not** participate in the later high-score eligibility test. A non-demo, non-disqualified aborted session can therefore still be examined by the high-score qualifier even when results and Ordering Information were suppressed.

## Outcome reduction and result presentation

The six-entry Drone outcome ledger is reduced exactly as documented in [`DRONE_OUTCOMES.md`](DRONE_OUTCOMES.md). The resulting disarmed count selects `disarm0.jba` through `disarm6.jba`, or `disarm6m.jba` for the established six-disarm Mothership-completion case. Results music follows the established hiphop/moon/suspense/choral branch order.

The result screen renders six numeric statistics in this exact order:

1. **Alien Ships Hit** = `alien_ships_hit` (`0x0044084C`)
2. **Missed** = `alien_ships_total - alien_ships_hit`
3. **Total** = `alien_ships_total` (`0x00446078`)
4. **Percentage Hit** = integer `(alien_ships_hit * 100) / alien_ships_total`
5. **Score** = live score
6. **Disarmed Drones** = reduced disarmed-outcome count

The original assumes a nonzero alien-ship total before the percentage division. The clean planner rejects zero or internally inconsistent hit/total inputs instead of manufacturing semantics for an impossible validated session state.

## Exact confirmation lock

The result-screen confirmation counter is initialized to `0x3A`. At `0x00411AF4` the original tests the positive counter *before* calling `confirm_input_pressed`; while positive it skips input polling, decrements the counter and performs the presentation loop. Therefore it performs **exactly 58 presentation/vblank iterations with confirmation polling disabled**, and the first confirmation opportunity is the following modal iteration after the counter is already zero.

After the lock expires, `0x004174A0` (`confirm_input_pressed`) accepts either:

- Enter (`VK_RETURN`); or
- an accepted joystick event when joystick input is active.

The clean constant is:

```text
win32_results_confirm_lock_presentations = 58
```

## Shareware Ordering Information handoff

After ordinary result confirmation, the canonical shareware path transitions through the established Ordering Information modal:

```text
result confirmation
    -> transition/wipe helper
    -> game_state_raw = 7
    -> run_ordering_information(1)
    -> game_state_raw = 1
```

The presentation-suppression flag skips this modal together with the result screen.

## High-score qualification bridge

After the result/ordering portion, the post-game tail gates score qualification only on:

1. `demo_playback_mode == 0`; and
2. `high_score_disqualified == 0`.

Qualification then uses the established lowest-to-highest ten-entry table and strict `new_score > existing_score` rule.

For a qualifying candidate, the caller writes state 8 and invokes `run_high_score_table` (`0x0041AFB0`) as:

```text
run_high_score_table(1, qualifying_index, disarmed_count)
```

The normal main-menu display invokes the same routine as:

```text
run_high_score_table(0, 0, 0)
```

The third argument is therefore not an opaque display parameter: it carries the current disarmed-Drone count into post-results insertion/completion behavior.

## Original slot-zero insertion quirk

A qualifying insertion at index 0 follows a distinct original path:

- candidate data is written directly into slot 0;
- the name is initialized to the literal `ENTER YOUR NAME`;
- the interactive name editor is **not** entered;
- `save_high_scores_file` is **not** called on that branch;
- execution falls into the ordinary state-8 score display loop.

For insertion indices greater than zero, the original shifts the table, enters interactive name input, and then saves the high-score file.

This is retained as a fidelity contract rather than silently corrected. The lower-level clean `insert_high_score()` remains a semantic table operation; `win32_post_game_plan()` exposes the original UI/persistence distinction through `HighScoreInsertionInteraction`.

## Final state behavior

The post-game tail has three established outcomes:

| condition | post-game result |
|---|---|
| no qualifying high score | state `1` (`MainMenuResetEntry`) |
| ordinary qualifying high score | state `4` (`MainMenuReentry`) |
| six disarmed + Mothership completion | credits, then state `1` |

The high-score routine itself can leave ordinary insertion through state 4. The outer result path preserves that state. Otherwise it normalizes the run to state 1.

## Completion credits

`0x00404720` is `run_completion_credits`. Its resource references include:

- `credits.wav`
- `credit1.jba`
- `credit2.jba`

The post-game tail calls it iff the same established completion predicate used by special result art is true:

```text
disarmed_count == 6
AND Mothership core target activity/state == 2
```

Thus the compiled perfect endgame is not merely a special result card/music choice: it proceeds into the actual credits sequence after high-score handling.

## Clean contract

`win32_post_game_plan()` records the established control decisions without bundling original artwork, sample bytes, global addresses, keyboard polling, or framebuffer code. It returns:

- the mission result reduction;
- all six numeric result statistics;
- whether Results and Ordering Information are presented;
- whether score insertion is eligible and where it lands;
- whether the original path prompts/saves or exhibits the slot-zero quirk;
- whether completion credits run; and
- the resulting menu-entry state.

`Q-RESULT-001` is therefore resolved at the gameplay/control-flow level. Phase 4 additionally owns this sequence in `GameSession`: lives<=0 diverts before ordinary gameplay/RNG advancement, `PostGameRuntimeState` enforces the exact locked-presentation/confirmation ordering, and subordinate raw-state handoffs use state 7 for Ordering Information and state 8 for qualifying high-score presentation before the established final state 1/4 result.

Phase 5 now owns the associated **semantic audio lifetime** as well: Results starts its selected one-shot flags-0 cue, confirmation stops/releases that ownership before Ordering starts its flags-1 `thunder2.wav` cue, Ordering completion stops that cue, and perfect-completion credits start/stop their flags-1 `credits.wav` cue at the corresponding modal boundaries. The original credits routine keeps its local scalar at 100 during the visual sequence, then after the final tracked credit reaches Y <= -40 decrements before each DirectSound volume call, yielding 99..0 across exactly 100 presentation iterations. The presentation-audio host now owns that envelope; the existing modal completion boundary supplies the following stop/rewind before release. Pixel-perfect presentation, interactive name entry and actual legacy score-file persistence remain rendering/UI/platform work rather than simulation-control inputs.
