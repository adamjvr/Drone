# JBA full-screen image format

**Confidence: confirmed from both DOS and Win32 loaders.**

Full-screen JBA files are exactly 64,768 bytes:

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

Several smaller Windows-only `.JBA` files are a different container case and are not accepted by the clean full-screen decoder yet; at least three contain embedded 128×128 8-bit PCX resources.
