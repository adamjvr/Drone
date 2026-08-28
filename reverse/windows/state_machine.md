# Win32 game-state protocol

**Status:** user-facing state protocol established for raw values `0..8`, `13`, and `99`.

The controlling scalar is `game_state_raw` at `0x0042B188`. The original does not use it as a simple closed top-level enum: `game_dispatch_update` at `0x0040BA50` directly dispatches only raw values `0..5`, while modal/menu/gameplay code consumes several larger transition values before control returns to that dispatcher.

## Direct dispatcher

`0x0040BA50` compares the raw value with 5 and uses a six-entry jump table:

| raw state | direct target | established role |
|---:|---:|---|
| 0 | `0x0040BA6A` | exit/shutdown transition presentation |
| 1 | `0x0040BA98` | main-menu entry after full session reset |
| 2 | `0x0040BAB9` | active gameplay orchestrator |
| 3 | `0x00411D86` | no-op return at top-level; instructions are consumed inside the menu/modal path |
| 4 | `0x00411D5A` | main-menu re-entry without the state-1 pre-reset |
| 5 | `0x00411D4A` | fallback transition to state 1 if pause reaches the top-level dispatcher |

The direct-table shape explains why raw values greater than 5 return immediately here without making those values meaningless. They are deliberately interpreted by subordinate paths.

## Established state meanings

| raw | semantic name | evidence-backed behavior |
|---:|---|---|
| `0` | exit/shutdown transition | the visible `EXIT DRONE` menu selection writes 0; the menu path performs cleanup before returning to the state-0 presentation path |
| `1` | main-menu reset entry | dispatcher calls `0x00417F50(1)` before `run_main_menu` (`0x00418AC0`) |
| `2` | active gameplay | `START GAME` writes 2; state-2 owns the large gameplay update/render region |
| `3` | instructions modal | `INSTRUCTIONS` writes 3; `0x0041C4B0` loads `instr01.jba` through `instr09.jba`, loops while state remains 3, and exits through state 4 |
| `4` | main-menu re-entry | modal paths return through 4; dispatcher calls `run_main_menu` without the state-1 full-session reset |
| `5` | pause overlay | gameplay `P` path writes 5 and renders `GAME PAUSED` / `<R> RESUMES`; resume returns to 2 |
| `6` | quit-game confirmation | gameplay Escape path writes 6 and renders `QUIT GAME?`, `<Y> TO QUIT`, and `<R> RESUMES` |
| `7` | ordering-information modal | `ORDERING INFORMATION` writes 7 and the menu immediately invokes `0x0041B730` |
| `8` | high-score modal | `HIGH SCORES` writes 8 and the menu invokes established `run_high_score_table` (`0x0041AFB0`) |
| `13` | demo-launch sentinel | `PLAY DEMO` writes 13; menu-entry dispatcher paths consume it by setting demo playback, resetting replay index to 0, and writing state 2 |
| `99` | nine-lives/disqualification notice | hidden `L` cheat writes lives=9, sets the high-score-disqualification flag, enters 99, renders the cheat/disqualification notice, and resumes to 2 with `R` |

A raw `-1` comparison also exists inside menu control flow. It is not required to explain the canonical user-facing state set above and remains unnamed rather than being forced into the clean enum.

## Main-menu transition table

`run_main_menu` renders seven visible choices and dispatches the selected index through the table at `0x00419CCE`:

| menu index | visible action | state/result |
|---:|---|---|
| 0 | `START GAME` | state 2 |
| 1 | `INSTRUCTIONS` | state 3 |
| 2 | `ORDERING INFORMATION` | state 7 |
| 3 | `HIGH SCORES` | state 8 |
| 4 | `CONFIGURE JOYSTICK` | synchronous call to `0x00420160`; no replacement game-state assignment |
| 5 | `PLAY DEMO` | state 13 |
| 6 | `EXIT DRONE` | state 0 |

After menu selection, selection-specific handlers consume the modal states. This is why state 7 or 8 can be semantically real despite being outside the six-entry top-level jump table.

## State 1 versus state 4

The two menu-entry states are intentionally distinct:

- **state 1** calls `0x00417F50(1)` and then `run_main_menu`;
- **state 4** calls `run_main_menu` directly.

This is an original lifecycle distinction, not duplicate enum noise. Clean code therefore names them `MainMenuResetEntry` and `MainMenuReentry` rather than collapsing both to `Menu`.

## Demo-launch consumption

Both menu-entry dispatcher paths recognize state 13 after `run_main_menu`. At `0x00411D68` the original performs the same transition:

```text
demo_playback_mode = 1
demo_frame_index = 0
game_state_raw = 2
```

This links the menu protocol directly to the independently recovered replay clock.

## Clean implementation

The clean core captures only the proven protocol surface in:

- `include/drone/gameplay/game_state.hpp`
- `src/gameplay/game_state.cpp`
- `tests/test_gameplay.cpp`

It intentionally does not reproduce the original global-variable architecture or assume that every original internal sentinel has been found.


### State 7 scrolling background

`run_ordering_information` (`0x0041B730`) temporarily owns `world_scroll_row` (`0x004D9590`): it resets the row to zero, advances a local three-step counter, decrements/wraps the row on local phase 2, and calls `0x00403560` (`compose_ordering_information_world_background`) each modal iteration. The compositor fills only framebuffer rows 35..179 from the cyclic 320x600 scenery stack. This local phase is independent of state-2's four-phase gameplay substep.


## Post-game return states

Post-game processing is inline within the state-2 dispatcher body rather than a new raw state. `player_lives <= 0` branches to the result region at `0x004115BE`. After optional Results/Ordering Information, score qualification invokes state 8 as an insertion modal. No qualifying score ultimately selects state 1; ordinary qualifying insertion leaves state 4; the six-disarm Mothership-completion path runs credits and normalizes to state 1. See [`../../docs/reverse/POST_GAME_FLOW.md`](../../docs/reverse/POST_GAME_FLOW.md).
