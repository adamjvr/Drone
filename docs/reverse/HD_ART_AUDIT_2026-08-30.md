# Drone 12x HD-art runtime audit — 2026-08-30

## Purpose

A live test reported that a window identified as `HD:599` still looked essentially the
same as Classic mode. The committed Phase-5 renderer and the generated 12x corpus were
audited separately so a loader/compositor failure would not be confused with an
upscale that contains little new information.

## Runtime findings

The HD presenter does not downsample the 12x asset to the 320x200 simulation buffer.
Fullscreen/world PNGs are decoded and resized directly to the X11 presentation size
(`320 * scale` by `200 * scale`). Dynamic indexed-framebuffer differences are then
composited over that high-resolution background, followed by transparent isolated HD
sprite frames where a recovered mapping exists.

The original implementation had one real fallback defect: classic pixels were erased
for every planned HD sprite even if the PNG lookup later failed. That could make actors
disappear. The fixed compositor first verifies the exact PNG mapping; missing HD frames
now retain their classic pixels.

The front end now exposes actual replacement telemetry on VIDEO SETTINGS: whether the
last background was HD and how many planned sprites were successfully replaced versus
missing.

## Corpus findings

The generated full-screen/world backgrounds are extremely conservative upscales. A
pixel-space comparison against the original art enlarged to the same 6x presentation
size found only small RGB changes for representative backgrounds, while isolated
sprites differed substantially more:

| Asset | Mean absolute RGB difference | 95th percentile |
| --- | ---: | ---: |
| `TITLESH` | ~1.05 / 255 | 4 |
| `RIVERTOP` | ~2.33 / 255 | 7 |
| `SHIP_00` | ~7.31 / 255 | 45 |

This explains why switching the entire game between Classic and HD can appear subtle
in scenery-heavy frames even when the HD renderer is functioning. The ship/enemy
isolated frames provide a much stronger A/B difference than the current title/river
backgrounds.

## Consequence for remastering

Do not "fix" this by changing gameplay scale, collision geometry, or by filtering the
classic framebuffer. The presentation path is now correctly separated from the
320x200 simulation. If a stronger visual remaster is desired, the next art pass should
focus on regenerated full-screen and scenery images with genuinely reconstructed
high-frequency detail while preserving original composition and pixel boundaries.
The runtime can consume those replacements without another gameplay rewrite.

## Broken-checkpoint follow-up (`04f5fcbd`)

The later cloud checkpoint confirmed that the visual complaint was not only weak source
upscaling. The first compositor made filesystem existence queries while determining HD
sprite coverage for individual logical pixels. That caused load-dependent slowdown as
formations/effects accumulated. Sprite paths are now preindexed once at startup.

A second defect flattened all HD sprites after the complete indexed framebuffer, which
violated the original render order and could place remastered actors over HUD/pause
text. Ordered compositor boundaries plus an explicit classic UI-write mask now preserve
modal/HUD authority.

The runtime mapping audit against the exact 12x delivery reports:

```text
PNG files:                599
isolated sprite PNGs:     398
runtime sprite mappings:  396 / 396
missing runtime mappings: 0
```

Corrected mapping details include Gemini2's global frame numbering (15..29) and the
DEBRIS1/DEBRIS2A recovered-category paths. `--hd-self-test` validates this mapping
contract in addition to decoding TITLESH, RIVERTOP and SHIP_00.

Performance validation used identical Xvfb 1920x1200, 4x-HD gameplay samples. The
broken checkpoint consumed about 10.9 CPU-seconds during the sample; the repaired host
consumed about 1.7 CPU-seconds. This is a presentation-only optimization: deterministic
gameplay remains on the recovered 70.0863 Hz update clock.
