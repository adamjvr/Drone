# FONT2 bitmap font

## Status

The `font2.jba` text subsystem is **established across both DOS and Win32**. An early Phase 1 note treated a repeated `0x14`-byte stride as a possible gameplay/cleanup record family. Phase 2 consumer tracing proves that stride belongs instead to the lazily initialized FONT2 glyph-descriptor cache.

This resolves `Q-ENTITY-001`: the record is presentation/UI state, not a gameplay entity.

## Win32 cache

Win32 `draw_bitmap_text` at `0x00401470` lazily loads `font2.jba`, then initializes **64 records** beginning at `0x00466C90`. The loop advances by `0x14` bytes and stops at `0x00467190`, giving an exact `64 * 0x14 = 0x500`-byte descriptor table.

For descriptor index `i` in `0..63`:

```text
column   = i % 16
row      = i / 16
source_x = 1 + column * 8
source_y = 1 + row    * 6
width    = 7
height   = 5
```

The source image is the decoded 320x200 `font2.jba` JBA sheet. The one-pixel source offsets and 8x6 cell pitch are the same gutter convention already recovered for sprite-sheet extraction.

### Original 0x14-byte descriptor

| offset | width | established meaning |
|---:|---:|---|
| `+0x00` | 4 | mutable X coordinate |
| `+0x04` | 4 | mutable Y coordinate |
| `+0x08` | 4 | glyph width = 7 |
| `+0x0C` | 4 | glyph height = 5 |
| `+0x10` | 4 | pointer to contiguous glyph-mask pixels |

During cache construction, X/Y hold the source-sheet crop origin. During text rendering, the same fields are overwritten with the destination draw position before the glyph renderer is called. They are therefore deliberately documented simply as `x` and `y`, rather than falsely splitting one physical field into two structures.

`0x00401630` allocates `width * height + 1`, which is 36 bytes for a normal FONT2 glyph, and stores the pointer at `+0x10`. `0x004015D0` copies exactly 35 meaningful bytes as five 7-byte rows from a source stride of 320.

`0x00401570` consumes the cached mask: zero mask pixels are transparent; each non-zero mask pixel writes the caller-selected palette index to the software framebuffer. This makes FONT2 a binary mask font even though its source lives in an indexed JBA image.

`draw_bitmap_text` indexes the table as:

```text
glyph = table[character - 0x20]
```

and advances the destination X coordinate by 8 pixels after each character. The established table therefore covers byte values `0x20..0x5F` (space through underscore). Original callers use compatible uppercase/digit/punctuation strings. The clean API bounds-checks this domain rather than reproducing an unsafe out-of-range memory access for unsupported bytes.

## DOS counterpart

The DOS binary independently contains the same subsystem.

The `FONT2.JBA` internal relocation reaches DOS `0x000809B0`, which performs the same lazy initialization:

- 64 descriptors;
- `0x14`-byte descriptor stride;
- 16 columns x 4 rows;
- 7x5 glyph masks;
- source origin `1 + 8*column`, `1 + 6*row`;
- allocation of `width * height + 1` at descriptor `+0x10`;
- 320-byte-stride mask extraction;
- lookup by `(character - 0x20) * 0x14`.

The paired DOS helpers are:

| DOS address | semantic role |
|---|---|
| `0x000809B0` | `draw_bitmap_text` / lazy FONT2 cache initialization |
| `0x000694EC` | extract one FONT2 glyph mask |
| `0x0006958C` | allocate descriptor pixel buffer |
| `0x000695AC` | release descriptor pixel buffer helper |
| `0x00083CB0` | render one cached glyph mask |

The DOS table begins at data offset `0x00006F80`; its lazy-init flag is at data offset `0x00000100`.

This is a strong DOS↔Windows correspondence because the table geometry, descriptor size/fields, crop equations, allocation size, ASCII mapping, and mask-render behavior all agree despite different executables and platform presentation layers.

## Clean implementation

The clean engine does **not** reproduce the raw pointer-bearing 20-byte descriptor. Instead `include/drone/fidelity/font2.hpp` exposes the semantic FONT2 geometry and safe character/index mapping, while `src/fidelity/font2.cpp` reuses the established JBA guttered-frame extractor to obtain each 7x5 glyph mask.

That keeps original binary layout as evidence while preserving a memory-safe, platform-independent fidelity boundary for Linux, macOS, iPadOS, and Windows hosts.
