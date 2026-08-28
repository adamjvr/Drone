# Gameplay Input Aggregation — DOS / Win32 Shareware

This document records the recovered boundary between physical input APIs and the semantic actions consumed by Drone gameplay. The key architectural result is negative as well as positive: **neither original build exposes one canonical packed gameplay-action bitfield**. Each build normalizes its physical devices into per-action tests, then state 2 selects either live input or demo replay for the six recorded controls.

That evidence supports a clean portable `GameplayInputFrame` in `drone_core`, while keeping DirectInput, DOS game-port timing, keyboard APIs, controller APIs, and touch mapping in platform hosts.

## Canonical semantic actions

The behaviorally established gameplay-facing actions are:

| semantic action | Win32 live source | demo channel | replay behavior |
|---|---|---:|---|
| move left | `VK_LEFT` OR normalized joystick-left | 1 | replay replaces live |
| move right | `VK_RIGHT` OR normalized joystick-right | 2 | replay replaces live |
| special launch | `VK_UP` OR mapped joystick button | 3 | replay replaces live |
| special load/cycle | `VK_DOWN` OR mapped joystick button | 4 | replay replaces live |
| shield | Space OR mapped joystick button | 5 | replay replaces live |
| rapid fire | Ctrl OR mapped joystick button | 6 | replay replaces live |
| move up | `A` OR normalized joystick-up | — | remains live during replay |
| move down | `Z` OR normalized joystick-down | — | remains live during replay |
| pause | `P` | — | live/meta control |
| quit confirmation | Escape | — | live/meta control |
| nine-lives cheat | `L` | — | live/meta control |
| vertical-retrace toggle | Tab | — | live/meta control |

The clean representation keeps every direction as an independent Boolean. It must **not** collapse Left/Right or Up/Down into a single signed axis before gameplay consumes them: the recovered player code processes opposing directions sequentially, and the later direction can leave observable state even when net position delta is zero.

## Win32 physical normalization

### Keyboard

State 2 obtains the Win32 key-state function through global `0x0053E2C4` and checks the high bit of the returned state for individual virtual keys. There is no intermediate keyboard action bitfield.

Established active-gameplay key mappings include:

- `VK_LEFT` (`0x25`) -> move left;
- `VK_RIGHT` (`0x27`) -> move right;
- `A` (`0x41`) -> player Y - 1;
- `Z` (`0x5A`) -> player Y + 1;
- Ctrl (`0x11`) -> rapid fire;
- Space (`0x20`) -> shield;
- Up Arrow (`0x26`) -> special launch;
- Down Arrow (`0x28`) -> special load/cycle;
- `P` (`0x50`) -> pause state 5 at the recovered state-2 transition gate;
- Escape (`0x1B`) -> quit-confirmation state 6 at the same gate;
- `L` (`0x4C`) -> nine-lives/disqualification state 99;
- Tab (`0x09`) -> negate the +1/-1 retrace-sync scalar.

### DirectInput joystick polling

`0x00406AC0` is established as `poll_directinput_joystick_state`. When the joystick device exists, it polls/acquires the device and requests a `0x50`-byte state into `0x0053C428`. If the device object is absent, the same state buffer is zeroed.

`0x00420090` is established as `normalize_joystick_gameplay_actions`. When joystick input is active (`0x004D95A0 != 0`), it calls the poller and converts the raw state into independent action bytes.

Axis dead zone is exactly **±360** (`0x168`):

| raw state | comparison | normalized action byte |
|---|---|---|
| `lX` | `< -360` | `0x00438C20` move left |
| `lX` | `> +360` | `0x004D6430` move right |
| `lY` | `< -360` | `0x00464B30` move up |
| `lY` | `> +360` | `0x004D850C` move down |

The four configurable button mappings are byte indices into the DirectInput button array beginning at raw-state offset `+0x30`:

| mapping-index global | normalized action byte | meaning |
|---|---|---|
| `0x004D95A4` | `0x004D95A8` | rapid fire |
| `0x0042B168` | `0x004D95AC` | special load/cycle |
| `0x0042B16C` | `0x004D95B0` | special launch |
| `0x0042B170` | `0x004D95B4` | shield |

These bytes are physical-source normalization only. They are not a complete engine action structure and should not leak into the portable simulation ABI.

## Live-source OR behavior

For the six ordinary recorded controls, Win32 state 2 checks keyboard first and then the corresponding normalized joystick byte. Semantically that is an OR of already-normalized live sources. Player movement similarly treats keyboard/joystick directions as independent requests.

The clean helper `merge_live_gameplay_input()` therefore performs field-wise OR on semantic frames. A modern host can use the same contract for keyboard + gamepad, keyboard + touch, or another combination without recreating the old APIs.

## Demo playback source selection

Demo playback is **replacement**, not another OR source, for the six recorded channels.

Conceptually the original state-2 blocks perform:

```text
if demo_playback_mode == 0:
    action = keyboard_action OR joystick_action
else:
    action = replay_channel
```

for left, right, launch, load/cycle, shield, and rapid fire.

A subtle but important exception is vertical player movement. Win32 `A`/`Z` checks occur after the replay-gated Left/Right blocks and have no `demo_playback_mode` test. Therefore live vertical input can still move the player while a demo is playing. DOS likewise consumes its two live vertical-movement bytes outside the six-channel replay substitution region.

The portable `apply_demo_playback_input()` intentionally replaces only the six recorded controls and leaves vertical/meta controls untouched.

## DOS convergence

The DOS build reaches the same gameplay-facing six-control set through different hardware paths.

### Game-port buttons

DOS `0x0006BAD0` is `normalize_gameport_button_actions`. It clears four Boolean action bytes, samples port `0x201`, compares the result with four configured mappings, and produces:

| DOS byte | semantic action |
|---|---|
| data `0x54` | rapid fire |
| data `0x55` | special load/cycle |
| data `0x56` | special launch |
| data `0x57` | shield |

The gameplay consumers independently confirm these meanings. For example, shield checks live Space or byte `0x57` before falling back to replay channel 5; rapid fire similarly combines live Ctrl/hardware input with byte `0x54` before replay channel 6.

### Game-port directions

DOS `0x0006BB4C` and `0x0006BC10` normalize horizontal and vertical joystick axes respectively. The horizontal normalized bytes include:

- `0x00083883` -> left;
- `0x00083885` -> right.

When the game-port path is unavailable, the horizontal normalizer falls back to the DOS extended Left/Right keyboard states. The vertical helper has the analogous hardware/fallback role. Exact historical keyboard-table aliasing outside the six replayed actions is not required by the clean host contract and is not overclaimed here.

### Same six replay channels

DOS gameplay independently performs the same live-vs-replay source selection for:

- channel 1: left;
- channel 2: right;
- channel 3: special launch;
- channel 4: special load/cycle;
- channel 5: shield;
- channel 6: rapid fire.

That cross-build convergence is the strongest evidence for the semantic boundary used by `drone_core`.

## Clean-core contract

The independently written implementation is:

- `include/drone/gameplay/input.hpp`;
- `src/gameplay/input.cpp`;
- regression coverage in `tests/test_gameplay.cpp`.

`GameplayInputFrame` exposes:

```text
movement.left
movement.right
movement.up
movement.down
rapid_fire
shield
special_launch
special_load_cycle
pause
quit
nine_lives
toggle_sync
```

The host owns physical mapping. The simulation owns semantic consumption.

This is intentionally **not** described as an original binary structure: it is a portable clean representation justified by the recovered DOS/Win32 convergence.

## Portability consequence

Final Linux, macOS, iPadOS, and Windows products should all feed the same semantic frame into `drone_core`:

```text
keyboard ─┐
gamepad  ─┼─> platform host normalization ─> GameplayInputFrame ─> drone_core
touch    ─┘                                      ^
                                                   |
                                       six-channel demo override
```

No target needs to emulate DirectInput object layout, `GetAsyncKeyState`, DOS port `0x201`, or game-port calibration to obtain gameplay fidelity. Those are historical physical-input implementations, not simulation rules.
