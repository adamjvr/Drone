# Win32 Dynamic Palette Effects

Phase 3 resolves the late Win32 palette helpers that remained open under `Q-RENDER-001`. These routines are presentation state. They do not update player, enemy, projectile, mission, or scoring rules.

## Palette storage and upload

The Windows port keeps palette state in 256 records with a `0x44`-byte stride. Two tables are relevant:

- `0x00441C20` — base/original palette records populated by JBA-side loading paths;
- `0x00455B80` — mutable working palette records used by gameplay fades and dynamic effects.

Within either historical record, the RGB integers consumed by the upload path are at `+0x08`, `+0x0C`, and `+0x10`. The clean engine does **not** reproduce this packed record ABI; it uses an ordinary 256-entry semantic palette plus separate effect state.

### `0x004011E0` — `upload_directdraw_palette_range`

Arguments are an inclusive first index, inclusive last index, and source palette-record table. The function constructs DirectDraw `PALETTEENTRY` data and calls the palette object's SetEntries method for exactly `last-first+1` entries.

For source entry `i` it takes the low byte of the historical 32-bit R/G/B values at:

```text
source + i*0x44 + 0x08
source + i*0x44 + 0x0C
source + i*0x44 + 0x10
```

The fourth `PALETTEENTRY` byte is initialized to `4` by the original wrapper. DirectDraw surface lock/unlock bookkeeping around the call is host/platform state rather than simulation behavior.

This promotes the old provisional `set_palette_range` name to an established DirectDraw palette-upload primitive.

## `0x0041EFE0` — `initialize_gameplay_palette_effect_bands`

This routine initializes four purpose-built mutable palette bands. It consumes CRT `rand()` values in a stable order; the clean helper accepts an injected nonnegative random source so tests and replay tools do not depend on a C runtime PRNG implementation.

### Palette 110..112 — sparse white flashes

All three entries start at RGB `(1,1,1)`. Each receives an independent phase counter initialized as:

```text
rand() % 20
```

The corresponding updater advances those counters through a 65-step cycle.

### Palette 96..102 — red breathing band

For every entry the routine consumes `rand()%255` for red, then overrides three entries:

| palette index | initial R |
|---:|---:|
| 100 | 93 |
| 101 | 243 |
| 102 | 162 |

All entries use `G=24`, `B=24`. Red-channel step alternates by index:

```text
96 +4
97 -4
98 +4
99 -4
100 +4
101 -4
102 +4
```

### Palette 128..148 — yellow/olive timed band

Initial RGB is `(182,182,57)`. Each entry receives:

```text
toggle = rand() % 2
timer  = 0
period = rand() % 240 + 162
```

### Palette 103..109 — green timed band

Initial RGB is `(162,214,97)`. Each entry receives:

```text
toggle = rand() % 2
timer  = 0
period = rand() % 240 + 200
```

The state-2 caller executes this initializer once when the first-objective palette-settlement counter reaches 61, then advances that counter to 62.

## `0x0041EE90` — `advance_gameplay_palette_effect_bands`

This is the per-update companion to `0x0041EFE0`.

### 110..112 flash behavior

Each counter increments and wraps `65 -> 0`. RGB is first reset to `(1,1,1)`.

- index 111 becomes `(255,255,255)` only at phase 32;
- indices 110 and 112 become white at phase 32 **and** phase 0.

### 96..102 breathing behavior

Only red changes:

```text
R += signed step
if R > 255: R=255, step=-4
if R <   6: R=6,   step=+4
```

Green and blue remain as initialized by this subsystem.

### 128..148 timed behavior

The timer increments until it equals the per-entry period, then resets to zero and flips the stored toggle:

- toggle 0 -> RGB `(182,182,57)`, toggle becomes 1;
- toggle 1 -> RGB `(12,12,12)`, toggle becomes 0.

### 103..109 timed behavior

Same timer/toggle protocol, but the bright color is `(40,215,97)`:

- toggle 0 -> RGB `(40,215,97)`, toggle becomes 1;
- toggle 1 -> RGB `(12,12,12)`, toggle becomes 0.

The state-2 caller runs this updater only while the relevant settlement counter equals 61 and neither the Drone nor Mothership-core destruction state is active.

## `0x00403490` — `advance_generic_gameplay_palette_animation`

A separate late-game kernel visits exactly these mutable palette indices:

```text
64..170
192..213
224..233
```

The holes `171..191` and `214..223`, plus index 234, are not visited by this function.

For an active historical record the routine:

1. adds signed per-channel steps to R/G/B;
2. clamps negative channel results to zero;
3. uses the **updated blue channel** to select direction/termination:
   - `blue >= upper` -> all three channel steps become `-1`;
   - else `blue <= lower` -> all three steps become `+1`;
   - else `blue == stop` -> record becomes inactive.

The upper/lower tests therefore take priority over stop equality.

For an inactive record the routine consumes `rand()%100`; a result `<2` marks it active. No color update occurs on that same activation call.

The state-2 call site gates this generic kernel to gameplay phase 2, after the relevant late-settlement conditions, while the Mothership palette/resource flag is clear. It is distinct from the first-objective purpose-built band updater above.

## Phase-sliced DirectDraw uploads

Once the late state-2 palette path is settled, the executable avoids uploading all 256 palette entries every gameplay update. It distributes inclusive `0x004011E0` uploads by the four-phase gameplay scheduler:

| gameplay phase | uploaded inclusive ranges |
|---:|---|
| 0 | `32..42`, `64..110` |
| 1 | `111..156` |
| 2 | `157..170`, `192..213`, `224..234` |
| 3 | `157..170`, `192..213`, `224..234` |

When the settlement/destruction/progression gates are not satisfied, the state-2 path falls back to a full working-palette upload:

```text
0..255
```

This is a **presentation workload schedule**, not a gameplay scheduler. The four-phase value is shared with gameplay, but palette upload ranges do not alter simulation state.

## Other ownership anchors

The surrounding state-2 code reinforces the distinction between base and working palette state:

- an earlier transition block derives darkened working RGB values from the base palette while a settlement timer is `<=60`;
- pause/quit/cheat modal entry saves current working RGB and subtracts 40 per channel with a zero clamp, then uploads the full working palette;
- modal exit restores the saved working RGB and refreshes saved/reference values from the base palette;
- Mothership resource/palette setup owns a separate flag at `0x004D95D0`; while it is nonzero, the generic `0x00403490` kernel is skipped.

These are palette/presentation transitions. They remain outside clean gameplay state.

## Clean implementation

- `include/drone/fidelity/palette_effects.hpp`
- `src/fidelity/palette_effects.cpp`
- `tests/test_fidelity.cpp`

The clean model intentionally separates:

1. 256 RGB entries;
2. purpose-built gameplay-band state;
3. generic animation controls;
4. host palette-upload range planning.

It does not publish or depend on the original `0x44`-byte record layout.
