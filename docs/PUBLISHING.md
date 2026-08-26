# Publishing and Original-Data Policy

## Repository objective

The public repository should contain enough code, tooling, metadata, and documentation for another engineer to understand and build the reconstructed engine **without bundling the original proprietary game payload**.

## Checked in

- independently written clean implementation code;
- analysis/extraction/validation tools;
- hashes, offsets, file names/sizes, and other non-payload metadata;
- function/global/structure/correspondence maps;
- independently written behavioral and format specifications;
- Ghidra helper/label scripts;
- synthetic tests/fixtures created by this project.

## Not checked in

- original game executables;
- original art/audio/level/demo payloads;
- extracted Wise installer payloads;
- retail/full-game content;
- bulk decompiler/disassembly dumps;
- local Ghidra database/project files.

`.reference/` is ignored by Git. Known reference packages are reconstructed locally and hash verified.

## Public build behavior

The repository must compile and run its synthetic tests without original data. Features that inspect/import original game data should fail with clear instructions when no user-supplied data is present rather than treating proprietary content as an undeclared build dependency.

## Shipping a playable remaster

Source-code reconstruction and original-asset distribution are separate rights questions. Before publishing packaged builds that contain original Pixelsplash payloads, distribution rights must be resolved. Until then, viable technical models include user-supplied original data and/or independently created replacement assets.

See `LICENSE_AND_RIGHTS.md` for the unresolved project source-license decision.
