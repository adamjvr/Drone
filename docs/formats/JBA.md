# JBA image families

Drone ships **two physically different file families with the `.JBA` extension**. They must not be auto-conflated by size-independent heuristics.

## 320×200 indexed JBA image/sheet

**Confidence: confirmed from both DOS and Win32 loaders.**

The common form is exactly 64,768 bytes:

```text
0x00000  768 bytes    256 palette entries × R,G,B
0x00300  64000 bytes  indexed pixels, lane-interleaved
```

Each palette component is a VGA-style 6-bit value. The Win32 loader shifts it left two bits when constructing its palette representation.

The pixel stream is not row-major on disk. The original loader performs:

```cpp
src = 768;
for (lane = 0; lane < 10; ++lane)
    for (dst = lane; dst < 64000; dst += 10)
        framebuffer[dst] = file[src++];
```

After deinterleaving, the logical image is row-major 320×200 indexed color.

Win32 loader: `0x004012B0`.

### Sprite-sheet use

The 64,768-byte form is not limited to literal full-screen presentation. The Windows build also loads sprite sheets through the same `0x004012B0` routine and then crops individual frames with `0x00401860`.

Those sheets remain decoded as ordinary 320×200 indexed images. Sprite cells use a one-pixel gutter and are extracted using entity-specific width/height. See [`../reverse/SPRITE_SHEETS.md`](../reverse/SPRITE_SHEETS.md).

For that reason the clean loader is named `load_jba_320x200`; Phase 1's provisional `load_fullscreen_jba` terminology is superseded.

## Windows small-JBA / embedded-PCX family

**Confidence: confirmed physical format from all three canonical Windows-shareware members. Runtime ownership is not established.**

The Windows shareware install contains three small `.JBA` files:

| file | bytes | prefix length | PCX offset | RLE bytes |
|---|---:|---:|---:|---:|
| `Sights/Logo.jba` | 7,494 | 64 | 65 | 6,533 |
| `Sights/River.jba` | 10,732 | 4 | 5 | 9,831 |
| `Sights/Screen.jba` | 8,923 | 38 | 39 | 7,988 |

Their container equation is exact:

```text
byte 0                       uint8 opaque_preamble_length = N
bytes 1 .. N                 N opaque preamble bytes
byte 1+N                     128-byte PCX header begins
following bytes              PCX RLE image stream
final 768 bytes              256 × RGB8 raw palette
```

Therefore:

```text
pcx_offset = 1 + file[0]
```

This relationship independently holds for all three files. The preamble payload is preserved as opaque evidence; no semantic field interpretation is currently justified.

### Embedded PCX contract

All three PCX headers agree exactly on the relevant geometry:

```text
manufacturer        0x0A
version             5
encoding            1 (PCX RLE)
bits per pixel      8
xmin,ymin           0,0
xmax,ymax           127,127
horizontal res      128
vertical res        128
planes              1
bytes per line      128
```

The RLE stream expands to exactly:

```text
128 × 128 = 16,384 palette indices
```

The byte immediately after the completed RLE stream is the first red component of a raw 768-byte RGB palette. **There is no conventional PCX `0x0C` 256-color palette marker.** This missing marker is part of the canonical small-JBA physical format, not a decoder omission.

Unlike the ordinary 320×200 JBA palette, these palette bytes are already full-range RGB8 values and are **not shifted left by two**.

All three canonical files contain the same raw palette:

```text
SHA-256 352df8085dc8872036538f831ccaeac2b26e6da7f45cee45d3ae18d45f32b9d9
```

Exact file-level and decoded-pixel hashes are recorded in [`../../manifests/small_jba_pcx.csv`](../../manifests/small_jba_pcx.csv).

### Runtime ownership

A direct ASCII-string inventory of canonical Win32 `Drone_sw.exe` contains the normal runtime JBA names such as `smallogo.jba`, scenery sheets, sprites, mission cards, and boss assets. It does **not** contain `Logo.jba`, `River.jba`, or `Screen.jba`.

Accordingly this milestone establishes their file format and lawful clean import path, but does not claim that the canonical game executable renders them. They may be shipped support/legacy artifacts. A runtime owner must be demonstrated separately if later evidence reveals one.

### Clean APIs

The two physical families remain intentionally distinct:

```cpp
load_jba_320x200(...);      // RGB6 + 10-lane 320x200 payload
load_small_jba_pcx128(...); // prefix + 128x128 PCX RLE + raw RGB8 palette
```

`drone_inspect jba-small-info` reports the small-container metadata and can emit a PPM preview. The clean decoder validates the self-described PCX offset, canonical geometry, exact 16,384-pixel RLE completion, and exact 768-byte raw palette trailer.
