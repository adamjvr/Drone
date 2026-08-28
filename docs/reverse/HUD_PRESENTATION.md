# Win32 Gameplay HUD Presentation

Phase 3 separates the late state-2 HUD from gameplay simulation. The original
writes these elements directly into the same 320x200 indexed framebuffer used
by world actors, but their geometry, colors, and small presentation timers are
presentation contracts rather than platform APIs.

Primary evidence is the ordinary state-2 tail at `0x00410F36..0x004113B3`,
with helpers `draw_bitmap_text` (`0x00401470`), `render_shield_meter`
(`0x0041EB70`), the normal transparent sprite blitter (`0x00401660`), and the
already-established target/shield entity roots.

## Score and lives

The score is rendered at Y=190 using FONT2 palette index 28. The X anchor is a
four-way right alignment chosen before text rendering:

| displayed score | X |
|---:|---:|
| 0..9 | 309 |
| 10..99 | 301 |
| 100..999 | 293 |
| 1000+ | 285 |

Lives are rendered at `(309,180)` with the same palette index 28. The gameplay
path clamps lives to 9 before formatting; score normalization/wrap behavior
remains owned by the scoring subsystem rather than this presentation module.

## Six-Drone mini-probe strip

A shared 11x16 common entity at `0x00433548` receives three frames from:

- `miniprg.jba` -> frame 0;
- `miniprb.jba` -> frame 1;
- `miniprr.jba` -> frame 2.

The renderer fixes X=3 and walks six vertical slots from Y=160 downward in
19-pixel steps:

```text
slot 0  y=160
slot 1  y=141
slot 2  y=122
slot 3  y=103
slot 4  y=84
slot 5  y=65
```

The raw renderer maps ledger values 1,2,3 to frames 0,1,2 and skips other
values. Canonical shareware gameplay currently establishes only outcome values
0 unresolved / 1 disarmed / 2 detonated; raw value 3 is retained as a renderer
capability because the third frame is loaded and explicitly selectable.

## Current Drone-outcome cursor

The formerly generic auxiliary HUD sprite at `0x0042F040` is loaded from
`square.jba`. The asset is a 13x18 green outline. Session initialization places
it at `(2,159)` and enables it through byte `0x0042F182`.

Its target Y is stored in the entity-family contextual word at `+0x1E`. The
initial target is 159. Every committed Drone outcome subtracts exactly 19 from
that target, matching the mini-probe strip spacing. During state-2 rendering,
only gameplay phase 2 moves current Y, one pixel upward while `y > target_y`.
The resulting cursor encloses the current unresolved outcome slot one pixel
above/left of the corresponding mini-probe anchor.

```text
processed 0 -> target y 159
processed 1 -> target y 140
processed 2 -> target y 121
processed 3 -> target y 102
processed 4 -> target y 83
processed 5 -> target y 64
```

When the sixth outcome is committed, `0x0042F182` is cleared and the cursor is
no longer rendered. The next mathematical target would be 45, but it is not a
visible seventh mission slot.

## Special-weapon target reticle

The 17x13 `target.jba` common entity at `0x004666E8` is rendered only while the
special projectile activity state is 1 or 3.

- Probe/frame 0 targets the Drone objective entity.
- Stinger/frame 1 uses the selected target pointer. If that target is inactive,
  the renderer substitutes a small presentation fallback centered near
  `(160,1)` rather than mutating gameplay ownership.

The reticle is centered as:

```text
x = target.x + target.width/2  - 8
y = target.y + target.height/2 - 8
```

The original then exhibits a one-pixel-inside edge quirk. It compares against
nominal right/bottom thresholds 302 and 186 but writes 301 and 185 when those
thresholds are exceeded. Negative coordinates clamp to zero.

## Probe/Stinger status text

The state-2 jump table at `0x00411E5C` maps special activity states 0..3 to
late FONT2 status presentation at `(5,190)`.

| activity | presentation |
|---:|---|
| 0 | `MISS` while presentation timer `<110`; timer increments while shown |
| 1 | `READY` |
| 2 | decode/disarm protocol below |
| 3 | `SEEKING` |

State 2 chooses:

1. `DECODING` while phase-1 elapsed is `>1` and `< phase-1 threshold`;
2. otherwise `DISARMING` while phase-2 elapsed is `>1` and `< phase-2 threshold`;
3. otherwise `DISARMED!` while its presentation hold timer is `<200`.

`DISARMING` resets the disarmed hold timer to 1. `DISARMED!` increments the
hold timer and uses palette index 57; the other strings use palette index 28.

The `MISS` timer at `0x0045A17A` is presentation state: the launched projectile
out-of-bounds path resets it to zero, while paths that should suppress the
message write 110. `0x0045A17C` is the corresponding disarmed-message hold
timer.

## Shield meter

`render_shield_meter` (`0x0041EB70`) reads `player_shield_energy >> 16` as the
number of displayed rows. The nominal clean gameplay maximum is 75 rows.
Each row is four pixels wide at X=313, starting at Y=138 and growing upward.

For zero-based row index `i`:

```text
y = 138 - i
width = 4
```

Palette bands are:

| row index | palette index |
|---:|---:|
| 0..10 | 27 |
| 11..25 | 57 |
| 26+ | 28 |

The active player-shield effect at `0x0041E6D0` remains a later overlay around
the player and is intentionally separate from the meter geometry.

## Clean implementation

The semantic presentation helpers live in:

- `include/drone/fidelity/hud_presentation.hpp`
- `src/fidelity/hud_presentation.cpp`

They do not duplicate the historical common-entity ABI and do not own score,
lives, mission outcome, or special-weapon simulation transitions. The same
module also exposes the `square.jba` outcome-cursor target/movement plan so the
late HUD can be reproduced without importing original globals. Their purpose
is to make the original HUD geometry and presentation-only timers testable for
future framebuffer comparisons.
