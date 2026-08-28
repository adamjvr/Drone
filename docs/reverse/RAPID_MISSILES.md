# Rapid-Fire Missile Reconstruction — Win32 Shareware

This document records the normal rapid-fire weapon controlled by **Ctrl** in the original Win32 shareware executable. It is distinct from the Space-key shield path and from the probe/stinger special-weapon system.

## Asset and pool identity

The rapid-fire projectile pool is rooted at **`0x0042F200`** and contains **eight** common `0x154`-byte sprite/entities. Static capacity global `0x0042B198` contains `8`.

Startup initializes each projectile as a **1×9** entity. `missile.jba` is decoded and exactly three frames are extracted from grid cells `(0,0)`, `(1,0)`, and `(2,0)`. The resulting frame pointers are shared across the eight pool entries.

The fire sound pool rooted at **`0x00446028`** is initialized from `missile.wav`, duplicated into the reusable 20-voice DirectSound pool pattern, and selected through `0x00420020`. The firing path additionally calls the recovered DirectSound frequency wrapper with `0x5622`.

## Fire gate and allocation

The keyboard path tests virtual key **`0x11` (Ctrl)**. A parallel joystick/control-record path reaches the same allocation block.

A normal rapid-fire shot can be allocated only when all established gates pass:

```text
active missile count < 8
fire cooldown == 8
player state byte == 1
fire action active
```

The allocator scans the pool in ascending slot order for the first entry whose activity byte is not `1`.

On spawn the original writes:

```text
missile.active = 1
missile.x      = player.x + 11
missile.y      = player.y - 3
missile.+0x143 = 0
active_count  += 1
fire_cooldown  = 0
```

The spawn path does **not** reset current frame `+0x140`; pool entries are initialized to frame zero once and reused entries retain whatever wrapped `0..2` frame they had when previously retired.

## Cooldown

Global **`0x004406F4`** is the recovered rapid-fire cooldown. Earlier in each active-gameplay iteration it increments by one while below `8` and then saturates:

```text
if cooldown < 8:
    cooldown++
```

A successful shot resets it to zero. This establishes the exact update-count gate but does not by itself establish real-time fire rate until the simulation cadence is solved.

## Update

The state-2 projectile update loop walks all eight entries.

For each active missile:

- on the recovered shared animation-tick condition, increment frame and wrap `3 -> 0`;
- subtract `3` from Y every update;
- if `y < 0`, set entity byte `+0x143` to `1`.

That edge flag is **not** the deactivation threshold.

## Cleanup and collision

The later missile collision pass starts by deactivating an active missile only when:

```text
y < -7
```

The same pass then tests the missile against several gameplay targets through the recovered hitbox and pixel-perfect collision primitives. Collision paths set activity to zero and frequently emit impact/explosion effects plus sound.

At the end of the iteration, if a missile that entered the collision pass active is now inactive, the original decrements global **`0x00440274`** (active missile count).

Several early targets in this collision chain are already tied to specific entity families (including the `gemhead.jba`/Gemini-related objects), but their complete enemy/boss semantics are still being decomposed before clean gameplay names are promoted.

## Rendering

The render path iterates the same eight-entry pool and sends each activity-state-1 missile through transparent sprite blitter `0x00401660` into the logical software framebuffer.

This gives a fully connected evidence chain:

```text
missile.jba
   -> JBA decode
   -> 1x9 guttered frame extraction
   -> eight-entry entity pool
   -> Ctrl allocation
   -> Y -= 3 / three-frame animation
   -> collision / top cleanup
   -> transparent framebuffer blit
```

## Clean implementation and probe

The independent reconstruction lives in:

- `include/drone/gameplay/rapid_missile.hpp`
- `src/gameplay/rapid_missile.cpp`
- regression cases in `tests/test_formats.cpp`

`drone_gameplay_probe` additionally drives the recovered player + rapid-missile code against user-supplied `Ship.jba` and `Missile.jba`, then writes a PPM snapshot. This is a development/parity probe; it is not claimed to be a reconstructed full game loop.
