# Win32 Framebuffer and Presentation Pipeline

The 1999 Windows port does not render gameplay sprites directly into the locked DirectDraw primary surface. It maintains a separate software indexed framebuffer and explicitly presents it.

## Software framebuffer allocation

During game initialization near `0x00407C06`:

1. the executable allocates `0xFF00` bytes;
2. clears that allocation;
3. stores the pointer at `0x004D9594`;
4. advances the stored logical pointer by `0x280` bytes.

The arithmetic is revealing:

```text
0xFF00 / 320 = 204 rows
0x0280 / 320 =   2 rows
0xFA00         = 64,000 bytes = 320 * 200
```

Therefore the allocation contains 204 320-byte rows, while the logical 320×200 framebuffer begins two rows into it. This leaves two guard rows before logical row 0 and two guard rows after logical row 199.

The shutdown path subtracts `0x280` from the logical pointer before freeing it, independently confirming that `0x004D9594` is an adjusted pointer rather than the original allocation base.

## Locked DirectDraw surface

DirectDraw Lock paths populate:

- `0x004D9584` — locked surface pixel pointer;
- `0x004DA780` — locked surface byte pitch.

These are platform surface state, distinct from the software framebuffer.

## Present copy — `0x00406B80`

Callers consistently provide a byte count of `0xFA00` for full-frame presentation.

If the locked pitch is exactly 320, the routine performs a contiguous copy. Otherwise it copies exactly 320 bytes per row for 200 rows and advances the destination by the DirectDraw pitch.

This yields the established direction:

```text
software_framebuffer (0x004D9594)
        |
        | 0x00406B80
        v
locked_surface_pixels (0x004D9584), pitch 0x004DA780
```

`0x00406BE0` provides the reverse pitched copy and is used by screen snapshot/restore paths. `0x00406C40` fills the locked surface while respecting pitch.

## Vertical blank and present

Several gameplay/menu paths follow the explicit sequence:

```text
render into software framebuffer
        -> 0x004018F0 / 0x004061E0 WaitForVerticalBlank
        -> 0x00406B80 copy software framebuffer to DirectDraw surface
```

This is the structural Win32 counterpart to the DOS VGA retrace/presentation model. It is independent of the separate QPC 15,000-count busy-wait.

## Bitmap text renderer — `0x00401470`

The same software framebuffer is passed to the recovered bitmap-text renderer. On first use it loads `font2.jba`, builds a glyph-descriptor table at `0x00466C90`, then indexes glyphs by `ASCII - 0x20`.

Call sites pass literal strings including:

- `START GAME`;
- `INSTRUCTIONS`;
- `ORDERING INFORMATION`;
- `HIGH SCORES`;
- `BEGINNER`, `INTERMEDIATE`, `ADVANCED`;
- `VERTICAL RETRACE`;
- `SYNC OFF` / `SYNC ON`;
- `<R> RESUMES`.

That makes `0x00401470` an established UI text renderer rather than a generic unknown blitter. Phase 2 now also establishes the exact cache layout: 64 descriptors × `0x14` bytes at `0x00466C90`, each holding mutable X/Y, 7×5 dimensions, and a glyph-mask pointer. The source sheet is a 16×4 grid with 8×6 cells and one-pixel top/left gutters, and DOS independently implements the same layout. See [`BITMAP_FONT.md`](BITMAP_FONT.md).

## Clean-engine consequence

The modern implementation keeps a core-owned 320×200 indexed framebuffer and treats host presentation as an adapter. It does not need to reproduce the original guard rows unless later parity work proves that an original edge behavior depends on them. The recovered guard layout remains documented because it can explain otherwise puzzling boundary reads and should be available to a future strict-compatibility mode.

## Transparent sprite blitter — `0x00401660`

The common `0x154` entity is drawn into the software framebuffer by `0x00401660`.

Established behavior:

- reads signed X/Y from entity `+0x00/+0x04`;
- reads signed width/height from `+0x20/+0x22`;
- selects `frame_pixels[current_frame]` through `+0x40..+0xBF` and `+0x140`;
- clips negative X/Y by advancing the source frame pointer;
- treats palette index `0` as transparent;
- writes all non-zero indices directly into the software framebuffer;
- uses a 320-byte destination row stride.

The original horizontal right-edge branch compares against literal `319`, then computes `copy_width = 319 - x`. This means the final logical column is excluded by this particular sprite path in cases that hit the right clip branch. The clean compatibility helper `blit_transparent_original` intentionally preserves and tests that quirk rather than silently replacing it with conventional `320 - x` clipping.

The vertical branch uses the current framebuffer-height global, whose canonical initialized value at `0x0042B140` is exactly `200`.

Clean semantic reconstruction:

- `include/drone/fidelity/sprite_blit.hpp`
- `src/fidelity/sprite_blit.cpp`

## Scaled transparent sprite path — `0x00403460` / `0x00413940`

A second sprite path is now classified. Wrapper `0x00403460` reads entity X/Y plus signed words `+0x24/+0x26` and constructs a destination rectangle:

```text
[x, y, x + render_width, y + render_height]
```

`0x00413940` then scales the entity's current source frame into that rectangle with 16.16 fixed-point nearest-neighbor source stepping. It clips to 320×200 and, like the ordinary `0x00401660` blitter, treats palette index zero as transparent.

This promotes common-entity `+0x24/+0x26` to established `render_width` / `render_height` fields. The ordinary unscaled path still uses source `+0x20/+0x22` dimensions.

The state-2 render block calls the wrapper for active special entities when their family-specific render-enable condition is satisfied. This path belongs to fidelity presentation; it does not alter gameplay state.


## Dynamic palette presentation

Phase 3 establishes that the late `0x00403490`, `0x0041EE90`, and `0x0041EFE0` helper cluster is palette presentation state rather than an unknown sprite/HUD subsystem. The Windows port owns a base palette upload source and a separate mutable working palette, then uses `0x004011E0` to push inclusive ranges into the DirectDraw palette object.

Once state 2 is settled, palette traffic is phase-sliced rather than fully uploaded every update; transition/destruction paths fall back to a full `0..255` upload. The clean fidelity layer reproduces the recovered palette algorithms and upload-range plan independently of DirectDraw.

See [`PALETTE_EFFECTS.md`](PALETTE_EFFECTS.md) for exact animated bands, random initialization, generic animation controls, gating, and upload ranges.

## Canonical gameplay presentation order

Phase 3 now records the complete ordinary state-2 presentation ordering from the world compositor at `0x004100D8` through the final framebuffer copy at `0x004115A5`. In particular, debris-particle pixels and Drone detonation radial noise are inserted between separate ordinary-sprite spans; scaled overlays follow those world/effect spans; HUD/shield/status layers are later still; palette mutation and host palette upload happen only after indexed-framebuffer drawing is complete.

See [`PRESENTATION_ORDER.md`](PRESENTATION_ORDER.md) and `drone::fidelity::canonical_win32_gameplay_presentation_order()`.
