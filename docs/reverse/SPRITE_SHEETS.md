# Win32 JBA Sprite-Sheet Extraction

The Win32 port uses the same decoded 320×200 JBA image representation for both screen-sized artwork and sprite sheets. Phase 1's provisional `load_fullscreen_jba` name was therefore too narrow; the clean API and ledgers now use `load_jba_320x200`.

## Pixel-container lifetime

Three routines establish the relevant JBA sheet-container field:

| address | provisional name | behavior |
|---|---|---|
| `0x00401900` | `allocate_jba_sheet_pixels` | allocates `0xFA01` bytes and stores the pointer at container `+0x4494` |
| `0x004012B0` | `load_jba_320x200` | decodes the JBA's 64,000 indexed pixels into the pointer at `+0x4494` |
| `0x00401450` | `free_jba_sheet_pixels` | frees the pointer at container `+0x4494` |

`0xFA01` is 64,001 bytes: the original reserves one byte beyond the 320×200 pixel payload. No semantic use of the extra byte has yet been proven.

The container itself is larger than the pixel pointer field (`+0x4494`). The purpose of the preceding bytes is still open, so the clean implementation does not reproduce that original layout.

## Frame extraction — `0x00401860`

This routine has five stack arguments:

```text
extract_guttered_sprite_frame(
    sheet_container,
    target_entity_0x154,
    frame_slot,
    cell_x,
    cell_y)
```

The target entity supplies `sprite_width` at `+0x20` and `sprite_height` at `+0x22`. The routine allocates `width * height + 1` bytes and stores the new frame pointer into:

```text
target.frame_pixels[frame_slot]   // target + 0x40 + frame_slot*4
```

The source sheet origin is:

```text
source_x = cell_x * (sprite_width  + 1) + 1
source_y = cell_y * (sprite_height + 1) + 1
```

It then copies exactly `sprite_width` bytes for exactly `sprite_height` rows, advancing the source by the fixed JBA row stride of 320 bytes.

The arithmetic proves that sprite cells are laid out on the decoded sheet with a **one-pixel gutter** between/around cells.

## Canonical call-site validation

The recovered calling convention is independently confirmed by the shareware asset initialization.

### `Debris1.jba`

`0x0040843A` initializes the target entity as 25×18. Later, `0x00409CF9..0x00409D12` calls the extractor eight times:

```text
frame_slot = 0..7
cell_x     = 0..7
cell_y     = 0
```

Therefore frame 0 starts at sheet `(1,1)`, frame 1 at `(27,1)`, and frame 7 at `(183,1)`.

### `Debris2a.jba`

`0x00408454` initializes the target entity as 27×17. The loader extracts sixteen frames in two rows:

```text
slots 0..7   -> cells (0..7, 0)
slots 8..15  -> cells (0..7, 1)
```

The second row begins at decoded Y=19 because `1 * (17 + 1) + 1 = 19`.

`manifests/recovered_sprite_frames.csv` records dimensions, cells, source origins, byte counts, and SHA-256 hashes of the reconstructed indexed frame payloads. It contains metadata only, not proprietary pixels. `scripts/analyze_sprite_sheets.py` regenerates that manifest from a local reference installation.

## Clean implementation

- `include/drone/fidelity/sprite_sheet.hpp`
- `src/fidelity/sprite_sheet.cpp`
- `drone_inspect jba-grid-frame ...`

The clean helper returns an exact `width × height` buffer rather than copying the original allocator's unused extra byte. This is a memory-management cleanup, not a behavioral difference in the copied sprite pixels.

## Relationship to collision

`0x00401FA0` samples the current extracted frame for pixel-perfect collision and considers palette index 0 transparent. The extraction and collision reconstructions therefore form one testable chain:

```text
JBA bytes
  -> recovered 10-lane decode
  -> recovered guttered frame extraction
  -> current-frame pixel buffer
  -> opaque-pixel collision
```

This chain is suitable for reference-backed validation without executing the original game.

## Trajectory enemy sheet layouts — long-pass correction

A host-side review of the canonical Win32 loader found an important presentation bug in the
first playable reconstruction. The clean frame extractor itself was correct, but the host
incorrectly converted a linear frame index into `(cell_x, cell_y)` by assuming every sheet
used all geometrically possible columns across 320 pixels. The original does not do that:
its loader explicitly issues frame-slot/cell-coordinate calls for each enemy sheet.

The recovered call sequences around `0x0040A8CD..0x0040B13F` establish these row widths:

| sheet | extracted frame rows |
|---|---|
| `Blade.jba` | `4 + 4 + 4 + 3 = 15` |
| `Arrow.jba` | `4 + 4 + 4 + 4 = 16` |
| `Bat.jba` | `8 + 8 = 16` |
| `Hydra.jba` | `5 + 5 + 6 = 16` |
| `Saddle.jba` | `8 + 8 + 8 + 8 = 32` |
| `Frisbee.jba` | `8 + 8 + 8 + 8 = 32` |
| `Sloop.jba` | `8 + 8 + 8 + 8 = 32` |
| `Flake.jba` | `8 + 8 = 16` |
| `Skate.jba` | `8 + 8 + 8 + 8 = 32` |

This matters behaviorally because palette index 0 is transparent. The old host's inferred
packing selected completely blank cells for several valid animation indices, making an
otherwise active enemy disappear and later reappear while its trajectory state continued.
The playable X11 host now follows the executable-backed row counts and rejects an unexpected
fully transparent trajectory frame at load time.
