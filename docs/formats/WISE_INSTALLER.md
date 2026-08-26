# Windows Wise Installer Reconstruction

**Evidence:** canonical Windows shareware installer `BIN-WIN-INSTALL-01`.  
**Status:** extraction method confirmed for this exact known installer.  
**Important scope:** this document describes the recovered layout needed to reproduce the known Drone installer payload. It is not a claim that all Wise installers use this exact stream organization.

## Why the installer is not executed

The project treats historical executables as evidence. The Windows game files can be reconstructed directly, making installer execution unnecessary for routine analysis and reducing host-environment risk and nondeterminism.

## Recovered payload organization

The known installer contains a chain of **raw DEFLATE** streams. The repository manifest records, per stream:

- stream index;
- compressed byte offset in the installer;
- compressed length;
- expected uncompressed length;
- expected CRC-32;
- installed path when the stream corresponds to an installed file.

`manifests/windows_shareware_wise_streams.csv` currently contains **207** stream rows. **192** have installed paths and are written into the reconstructed installation.

Across all 207 known streams, the manifest accounts for:

- 3,971,713 bytes of compressed stream data;
- 12,856,600 bytes of uncompressed stream data.

These totals describe the streams in the current manifest; the installer executable also contains wrapper/metadata bytes outside those compressed ranges.

## Decompression algorithm

`tools/scripts/wise_extract.py` uses zlib with a negative window size (`-15`) to request raw-DEFLATE decoding with no zlib/gzip wrapper.

For each manifest row:

```text
raw = installer[compressed_offset : compressed_offset + compressed_bytes]
data = raw_deflate_decompress(raw)
assert len(data) == uncompressed_bytes
assert crc32(data) == expected_crc32
if installed_path is not empty:
    write data to output / installed_path
```

A file is never written until its stream has passed the recorded length and CRC checks.

## Reproducibility command

The preferred route is the complete bootstrap:

```bash
python3 scripts/bootstrap_reference.py .reference/input .reference/work
```

Direct extraction of the known installer is also supported:

```bash
python3 scripts/wise_extract.py \
  .reference/work/windows-installer/drone_sw.exe \
  .reference/work/windows
```

## Evidence boundary

The stream manifest contains offsets, sizes, CRCs, and installed filenames/paths. It does **not** contain the proprietary payload bytes. This allows the extraction method to be published while requiring the user to supply the original installer.

## Remaining questions

The project has not yet attempted to reconstruct the complete Wise installer metadata model, UI script, or installation logic because those are not required to recover the game payload. If later analysis needs installer-specific version/configuration behavior, that work should receive its own findings and confidence statements rather than being inferred from the current stream table.
