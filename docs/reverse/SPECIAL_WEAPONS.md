# Probe / Stinger Special-Weapon Reconstruction — Win32 Shareware

This document records the special-weapon system controlled by **Down Arrow** (load/cycle) and **Up Arrow** (launch). It is separate from the Ctrl rapid-fire missile pool and the Space-key shield.

The most important structural result is that Probe and Stinger are **not separate projectile object types** in the Win32 build. They are two frame choices of one common `0x154`-byte entity rooted at **`0x0045A148`**.

## Entity identity and asset mapping

Startup calls the common entity initializer `0x00401780` for root `0x0045A148` with:

```text
x      = 146
y      = 182
width  = 3
height = 8
```

Later asset initialization loads two separate JBA sheets into two frame slots of that same entity:

| entity field | value | source |
|---|---:|---|
| `+0x140` / `0x0045A288` | `0` | `probe.jba`, frame slot 0 |
| `+0x140` / `0x0045A288` | `1` | `redprobe.jba`, frame slot 1 |
| `+0x141` / `0x0045A289` | `2` | established frame count |
| `+0x142` / `0x0045A28A` | lifecycle state | common entity activity/state byte |
| `+0x143` / `0x0045A28B` | edge/out-of-bounds flag | common entity field |

This resolves several Phase 1/early-Phase-2 “special globals”: `0x45A288..0x45A28B` are simply the known tail of `WIN-STRUCT-002` at root `0x45A148`.

The supplied README independently states that blue probes target/disarm Drones and red stingers home on hostile ships, so the asset/frame mapping and documented weapon names agree.

Metadata-only hashes for the recovered 3×8 frames are recorded in `manifests/recovered_sprite_frames.csv`; no original pixels are stored in the repository.

## Established lifecycle states

The common entity `+0x142` byte has the following high-confidence meanings in this subsystem:

| value | semantic name | direct evidence |
|---:|---|---|
| `0` | inactive / unloaded | reset/init paths; Down-load gate requires zero |
| `1` | loaded / target-tracking | Down from state 0 enters 1; targeting/render path consumes 1 |
| `2` | blue probe attached / decoding | launched frame-0 collision with `drone.jba` entity enters 2 and starts decode timers |
| `3` | launched / homing | Up from state 1 writes 3; movement/collision paths gate on 3 |
| `4` | hole interaction | a launched state-3 projectile colliding with active `hole.jba` under recovered gates writes 4; renderer co-draws hole + projectile |
| `10` | impact consumed / terminal | many enemy collision handlers write 10; common next-update dispatch stops `probe3.wav` and resets state to 0 |

State 4's broad meaning is established as the visible `hole.jba` interaction, and the downstream collision entity rooted at `0x00472598` is now established as the Mothership core/target common entity. The clean special-projectile state name remains deliberately conservative (`HoleInteraction`) because the complete Mothership subassembly mechanics are still being reconstructed.

## Load, cycle and launch input

### Load — Down Arrow from inactive

The keyboard path tests `VK_DOWN (0x28)`; the parallel mapped-input path reaches the same block. With an active player and special state 0, the original:

```text
play ignite2.wav
special.state = 1
special.y = player.y + 7
special.+0x143 = 0
special.+0x36 = 0
```

It leaves `+0x140` (the selected Probe/Stinger frame) unchanged. Therefore the first Down action **loads the currently selected kind**, rather than necessarily changing weapon kind.

On the following state-1 update the projectile X/Y are re-anchored from the player before the common upward/homing step:

```text
special.x = player.x + 14
special.y = player.y + 7
```

### Cycle — Down Arrow while loaded

Special scratch word `+0x36` (`0x0045A17E`) increments while state is 1 until it reaches special word `+0x3C` (`0x0045A184`). Canonical gameplay reset initializes:

```text
+0x36 = 0
+0x3C = 12
```

Only when the two are equal can another Down action cycle the selected frame:

```text
play ignite2.wav
current_frame++
if current_frame == 2:
    current_frame = 0
+0x36 = 0
```

Thus the original selector is exactly a two-state ring:

```text
0 -> blue Probe
1 -> red Stinger
0 -> ...
```

This also demonstrates that common-entity offsets `+0x32/+0x36` are **object-specific scratch/control fields**, not universally trajectory-only fields. Trajectory-owned entities give those offsets trajectory semantics, while the special projectile reuses them for different control state.

### Launch — Up Arrow

With player active and special state 1, `VK_UP (0x26)`:

```text
play probe3.wav
special.state = 3
```

No switch-progress threshold is required for launch. The original uses `probe3.wav` for this common launch transition regardless of selected frame.

## Homing movement

For established movable states 1 and 3, the common update subtracts two pixels from Y per update:

```text
special.y -= 2
```

Horizontal homing is intentionally slow and discrete: X changes by **exactly one pixel per update** toward the chosen target coordinate.

### Red Stinger (frame 1)

When `current_frame == 1`, Win32 `0x0040DF47..0x0040E04E` updates the shared target pointer `0x004D8508` with a stateful priority chain. The exact order is:

1. active Mothership panel gate `0x00446DD2` -> hole entity `0x00433700`;
2. Gemini, using active body states to choose a head; when both bodies are active the head whose **X origin** is nearest to player X wins, and an equal-distance tie selects head B;
3. Lid/Top top/root, only while top state is 1 and the lid current frame is greater than 3;
4. Spidey;
5. registered boss slot 2;
6. Bomber;
7. one still-unidentified active common-entity owner at `0x00459F90`, whose current dynamic target geometry is reached through `0x00495CE8`.

If no branch qualifies, the executable does **not** install a fallback target in this movement block: `0x004D8508` retains its previous pointer. A successful special load separately resets that pointer to dummy entity `0x0045A708`, explicitly writes dummy X=160, and its zero/BSS width makes the initial desired X exactly 160. This means a newly loaded Stinger with no eligible hostile homes toward screen center rather than toward the Drone.

Once a target object is chosen, desired X is its horizontal center:

```text
desired_x = target.x + target.width / 2

if special.x < desired_x: special.x++
if special.x > desired_x: special.x--
```

This directly corroborates the README's statement that red stingers home toward the locked hostile target.

### Blue Probe (frame 0)

The canonical normal blue-probe branch targets entity **`0x00446080`**, which is independently initialized as a 15×38 common entity and populated from **`drone.jba`**. Its normal desired X is:

```text
desired_x = drone.x + 4
```

The literal `+4` is preserved even though it is not the geometric center of the 15-pixel-wide Drone sprite.

An alternate conditional branch involves entity `0x00433700`, populated from `hole.jba`. When a launched state-3 special collides with that active hole under the recovered gameplay gates, the original writes **state 4**. The renderer then draws both the hole entity and the special projectile through the normal transparent blitter. A later collision against `0x00472598`, now established as the Mothership core/target common entity, ends state 4. For Stinger/frame 1 it can advance motor/subassembly state and, once the prerequisite motor states are satisfied, set the Mothership target `+0x142` activity state to `2`, beginning the destruction sequence. The full subassembly mechanics remain partial; see [`MOTHERSHIP.md`](MOTHERSHIP.md).

If the special projectile X leaves `0..319`, the original clears activity to state 0 and raises `+0x143`.

## Targeting reticle behavior

The renderer contains a separate target/reticle object rooted at `0x004666E8`. When special state is 1 or 3, it positions that reticle relative to the selected target and blits it through the standard transparent sprite path.

For frame 0 the target selection used by the reticle is the Drone entity. For frame 1 it uses the current hostile-target pointer. This is independent corroboration that current frame is weapon kind and that state 1 already performs lock/tracking before launch.

The special projectile itself is also sent through `0x00401660` for nonzero states except the separately handled state-4 path.

## Blue Probe attachment and decoding

A high-value collision path at `0x0040F655` tests a state-3 special entity against the `drone.jba` entity at `0x00446080`.

When the colliding weapon frame is **0 (blue Probe)**, the original:

```text
special.state = 2
phase1_elapsed = 0
phase2_elapsed = 0
```

and initializes two decode thresholds.

For the normal live-game branch, both thresholds incorporate `0x0042B1A8`. That byte is independently established by the options/menu code as the **difficulty level**:

```text
1 = BEGINNER
2 = INTERMEDIATE
3 = ADVANCED
```

The recovered live-game threshold formulas are:

```text
phase1_threshold = (rand() % 70 + 450) * difficulty
phase2_threshold = (rand() % 70 + 300) * difficulty
```

Demo playback mode (`0x00440594 == 1`) uses fixed thresholds **210** and **150** instead of the live randomized/difficulty-scaled values. This is now understood as part of the original deterministic-replay strategy rather than an alternate difficulty/input mode.

The separate status byte at `0x0045BEEA` is now established exactly:

```text
0 = phase 1 decoding
3 = phase 2 disarming
1 = complete
```

On a successful blue-Probe collision the game first awards **+10** to both total score and extra-life progress, enters special state 2, clears both elapsed counters and initializes the thresholds above. While state 2 is active and status is not 1, the decoder advances **once per gameplay update** (not only on a four-phase animation tick).

At the exact phase-1 threshold the original writes status 3 and resets phase-2 elapsed to zero. The control flow deliberately falls through into the status-3 block in the **same update**, so phase-2 elapsed becomes 1 immediately. At the exact phase-2 threshold it:

- emits the completion effect at the Drone;
- consumes one `rand()%60 + 40` value for an effect/sound parameter;
- awards **+500** to total score and extra-life progress;
- writes status 1 (complete).

The normal Drone owner later observes status 1 on the same gameplay update, releases the Y=45 hold, commits the normal outcome at Y=201, and clears the completed decoder status/counters after the Drone passes Y=230. This completes the executable-backed normal attach/decode/disarm chain.

## Trajectory collision and Stinger display

The late special block has a trajectory path distinct from the Drone collision. After bomb processing, the original tests whether the special is state 3 and enters the late block. Inside that captured block the Drone is tested first. Even if the Drone collision then changes the special activity (for example a blue Probe attaches), the later trajectory scan still executes with the retained projectile coordinates.

That scan tests the special point against each active trajectory actor through `0x00401F60` and the actor's 0.85-derived hitbox. A hit directly destroys the actor; it is not a +damage threshold path. The actor scan does not re-read special activity between actors, so a blue Probe that is set inactive on the first direct trajectory hit can still destroy later overlapping actors in the same scan. Direct destruction also does not explicitly clear the actor damage byte, which matters for persistent group-0 actors that may later be replenished.

For a live red Stinger, a direct trajectory hit keeps the projectile launched and activates the separate six-frame `stinger.jba` entity at `0x00434C10`, centered at projectile X-30/Y-26. The display's current frame is not explicitly reset on activation. While active, display frames 3..5 use `0x00402FC0` with its centered 51x45 hitbox against trajectory actors' full sprite rectangles and add +15 damage. That AoE destruction path is the `0x0040EFD1` site that increments encounter alien hits only. Enemy-bomb destruction of a red Stinger can also activate this display; because the display/AoE stage occurs earlier in the update, that new activation cannot damage trajectories until a later tick.

## Enemy-bomb knockoff / decoder interruption

The late bomb collision loop at `0x0040F35F..0x0040F4B8` tests every active bomb against any nonzero Probe/Stinger state through `0x00402000`. That primitive adds 9 to the bomb Y coordinate and tests against the special entity hitbox. Because the special entity is 3x8 and common initialization uses 0.85-derived collision extents, the inclusive special hitbox is 2x6.

A hit deactivates the bomb and consumes the special weapon. The decoder side effect is deliberately conditional: only an attached state-2 Probe with status not complete **and `phase2_elapsed > 0`** is interrupted back to status 0 with both elapsed counters cleared. A Probe knocked off during phase 1 is consumed but its phase-1 elapsed scalar is not explicitly cleared by this branch. A completed status-1 decoder likewise is not reset by this collision branch. Probe and Stinger then take distinct impact-effect/audio paths.

This exact vulnerability path is now owned by `GameSession`; hostile spawn/steering selection remains a separate encounter producer.

## Red Stinger impacts

Numerous state-3 collision handlers distinguish frame 0 from frame 1. Common behavior is to enter state **10** on impact. The common special-state dispatcher handles value 10 by stopping/resetting `probe3.wav` and immediately writing state 0, establishing state 10 as a one-update consumed/impact terminal state. Frame-1 branches invoke stinger/damage/explosion-specific effects, including the reusable `stinger1.wav` voice pool at `0x00464B40`.

This establishes that frame 1 is the damaging Stinger branch and that generic projectile consumption is now modeled cleanly. Individual enemy damage values and target-specific consequences remain enemy-system work rather than being generalized prematurely.

## Audio identities

| address | source | established use |
|---|---|---|
| `0x00446C2C` | `ignite2.wav` | load / weapon-cycle action |
| `0x0042EFE4` | `probe3.wav` | state-1 launch transition |
| `0x00464B40` | `stinger1.wav` | reusable Stinger impact/effect pool |

## Clean reconstruction

Narrow, independently written state helpers live in:

- `include/drone/gameplay/special_weapon.hpp`
- `src/gameplay/special_weapon.cpp`
- synthetic regression cases in `tests/test_formats.cpp`

The clean module currently implements only behavior with direct evidence:

- frame 0/1 kind mapping;
- state 0 -> 1 load;
- 12-update load/cycle progress gate;
- Probe/Stinger cycle;
- state 1 -> 3 launch;
- state-1 player anchoring;
- Y `-= 2` movement;
- one-pixel X homing toward a caller-selected target;
- exact normal Stinger and Probe target-X formulas;
- frame-0 state-3 -> state-2 Drone attachment through the recovered inclusive point-hitbox producer;
- exact +10 attachment award and live/demo threshold initialization;
- exact decoder status protocol 0 -> 3 -> 1, including same-update phase-2 tick 1;
- exact +500 completion award and completion-effect PRNG consumption;
- state-2 horizontal pin to `Drone.x + 5`;
- enemy-bomb knockoff through the recovered Y+9 inclusive hitbox, including the exact phase-2-only decoder interruption reset;
- state-3 -> state-4 hole-interaction transition;
- state-10 -> state-0 terminal settlement.

The clean implementation also carries the exact MSVC CRT 15-bit PRNG used by this path (`original_random.*`) so live threshold draws are deterministic when seeded. Red-Stinger target selection/retention is now clean-integrated through `stinger_targeting.*`; candidate geometry for actor families whose movement is not yet session-owned remains an explicit encounter fact. The implementation intentionally does **not** yet encode the complete Mothership subassembly/core state machine reached through the state-4 path. That remains an active reverse-engineering target.


## Presentation cross-reference

The exact late-gameplay HUD geometry and presentation-only status/meter behavior are documented in [`HUD_PRESENTATION.md`](HUD_PRESENTATION.md).
