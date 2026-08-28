# Reference Framebuffer Validation

Phase 3 now has a public, copyright-safe workflow for framebuffer parity. The repository stores comparison code and metadata only; user-supplied original-runtime framebuffer snapshots remain local evidence.

## Canonical local snapshot — `DRONEFB1`

A `.drfb` file is a deliberately simple local interchange format:

```text
offset  size    meaning
0x00    8       ASCII "DRONEFB1"
0x08    2 LE    width = 320
0x0A    2 LE    height = 200
0x0C    2 LE    palette entries = 256
0x0E    2 LE    reserved = 0
0x10    4 LE    pixel bytes = 64000
0x14    4 LE    palette bytes = 768
0x18    64000   indexed framebuffer, row-major
...     768     palette, 256 consecutive RGB8 triples
```

Total file size is **64,792 bytes**.

This is a clean-project interchange format, not an assertion about an original Drone file format. It exists so a reference runtime capture and clean-engine output can be compared through the same representation.

`.drfb` files are ignored by Git. Original-runtime captures should normally live below `.reference/framebuffers/` or another user-controlled evidence directory.

## C++ comparison tool

`drone_framecheck` provides deterministic local inspection:

```bash
# Inspect format/geometry.
./build-debug/drone_framecheck info frame.drfb

# Exact full-frame comparison.
./build-debug/drone_framecheck compare original.drfb clean.drfb

# Compare one rectangular region without weakening the global oracle.
./build-debug/drone_framecheck compare-region \
  original.drfb clean.drfb 0 0 320 32

# Wrap raw 64,000-byte index and 768-byte RGB8 palette dumps.
./build-debug/drone_framecheck from-raw \
  frame.indices frame.palette.rgb frame.drfb

# Produce a local visual preview.
./build-debug/drone_framecheck to-ppm frame.drfb frame.ppm
```

Full comparison reports four independent quantities:

- **indexed pixel mismatches** — differing palette indices;
- **rendered RGB mismatches** — differing visible RGB after each frame's own palette is applied;
- **palette entry mismatches** — palette slots with at least one differing channel;
- **palette channel mismatches** — total differing R/G/B channels.

When indexed pixels differ, the tool also reports the smallest rectangle containing every differing pixel. This makes an off-by-one blit or late HUD difference diagnosable without publishing the original frame.

Region comparison intentionally checks both index bytes and rendered RGB. A region can therefore have identical indices yet still fail because a palette animation changed one of the colors it uses.

## Clean-engine snapshot generation

The deterministic gameplay probe remains backward-compatible and can optionally emit a snapshot in addition to its PPM:

```bash
./build-debug/drone_gameplay_probe \
  .reference/work/windows \
  /tmp/drone-probe.ppm \
  120 \
  /tmp/drone-probe.drfb
```

The snapshot is generated from the clean indexed framebuffer and its active palette. This is useful for regression work without changing the established PPM probe oracle.

## Publishable fingerprint metadata

`scripts/framebuffer_fixture.py` computes SHA-256 metadata without placing original framebuffer bytes in Git:

```bash
python3 scripts/framebuffer_fixture.py fingerprint \
  .reference/framebuffers/win32-demo1-tick420.drfb \
  --fixture-id win32-shareware-demo1-tick420 \
  --source-build windows-shareware-1999 \
  --scenario demo1 \
  --tick 420 \
  --region hud:0:0:320:32 \
  --region world:0:32:320:168 \
  --output .reference/framebuffers/win32-demo1-tick420.json
```

The metadata contains:

- whole `.drfb` SHA-256;
- 64,000-byte index-plane SHA-256;
- 768-byte palette SHA-256;
- fully resolved RGBA-frame SHA-256;
- optional region index and RGBA SHA-256 values;
- source/scenario/tick labels supplied by the researcher.

A local snapshot can later be checked against that metadata:

```bash
python3 scripts/framebuffer_fixture.py verify \
  reference-metadata.json \
  .reference/framebuffers/win32-demo1-tick420.drfb
```

Only hashes and descriptive metadata are intended for publication. The empty schemas in `manifests/reference_framebuffers.csv` and `manifests/reference_framebuffer_regions.csv` are ready for lawful captures when we have them; **no original-frame hash has been fabricated in advance**.

## Capture provenance requirements

A framebuffer reference is useful only if its provenance is strong. For every original-runtime capture record:

1. exact executable hash;
2. host/emulator/runtime version;
3. scenario or replay identity;
4. deterministic tick/frame landmark if known;
5. whether capture is the software indexed framebuffer or a post-presentation surface;
6. active palette source and encoding;
7. capture method/tool version;
8. any known timing/input uncertainty.

For Win32 fidelity, the preferred oracle is the **software 320×200 indexed framebuffer plus current working palette immediately before final presentation**, because the recovered state-2 order proves all indexed drawing is complete before palette upload and the final framebuffer copy.

## Comparison policy

Do not weaken a failing full-frame comparison globally. Use smaller regions only when there is a documented reason, such as an independently unresolved HUD layer or host-only presentation artifact. Record the reason and retain the full mismatch result.

The remaster renderer is never substituted for this oracle. Fidelity framebuffer validation and remaster visual validation remain separate products of the architecture.
