# Evidence Provenance and Binary Identity

## Purpose

Reverse-engineering addresses and conclusions are meaningful only when tied to an exact binary. This document defines the canonical Phase 1 evidence set and the chain-of-custody rules for future evidence.

The original game material is **not** included in the repository. The hashes below identify user-supplied files that the local bootstrap tooling accepts.

## Canonical source packages

| ID | file | bytes | SHA-256 | role |
|---|---|---:|---|---|
| `PKG-DOS-SW-01` | `drone_sw.zip` | 7,296,421 | `ceba0a398a6b3260b415227383bae35b7ffac442a723329581bb2007f023d192` | DOS shareware distribution |
| `PKG-WIN-SW-01` | `drone_sw(win).zip` | 3,964,087 | `4f4c0d4c7d0333f1066b1d3f0e4ff622e4df5d911a7b7440eb2ebb9e4407837d` | Windows shareware distribution containing Wise installer |

These values are duplicated in `manifests/source_packages.sha256` so scripts can verify them mechanically.

## Principal binaries

| ID | local path after bootstrap | bytes | SHA-256 | identity |
|---|---|---:|---|---|
| `BIN-DOS-SW-01` | `dos/DRONE_SW.EXE` | 744,819 | `e7de54f9cd158289df58a7eee4ecbe6f1b9b04972e7eae88b26bfe32515a0fc0` | 32-bit x86 Watcom C/C++, LE/DOS4GW game executable |
| `BIN-WIN-INSTALL-01` | `windows-installer/drone_sw.exe` | 3,987,560 | `dde5be99d57a80998df8272174998ea46382ef36fb4a0d323c84d878932a9869` | 1999 Wise installer wrapper |
| `BIN-WIN-SW-01` | `windows/Drone_sw.exe` | 188,928 | `4fffc0406157b0def539b619fa53d1bb6c59537a44a6551df3aa3c060eaf3784` | PE32/i386 native Win32 game executable; linker major 5; PE timestamp observed as 1999-03-26 |

Principal executable hashes are stored in `manifests/reference_binaries.sha256`. `scripts/verify_reference.py` currently verifies the two game binaries.

## Package reconstruction

`scripts/bootstrap_reference.py` performs the canonical local reconstruction:

1. verify both source ZIP hashes;
2. extract the DOS ZIP normally;
3. extract the Windows outer ZIP;
4. locate the Wise installer executable;
5. reconstruct its installed files using `scripts/wise_extract.py` and `manifests/windows_shareware_wise_streams.csv`;
6. generate local DOS and Windows inventories.

The known Windows installer manifest contains **207** recovered raw-DEFLATE streams; **192** streams map to installed files. Each stream is checked against its recorded uncompressed length and CRC-32 before it is written.

## Observed corpus size

For the canonical shareware evidence set:

- DOS extracted corpus: **187 files**, 17,277,984 bytes;
- reconstructed Windows installed corpus: **192 files**, 12,354,961 bytes.

These figures describe the known reference packages, not every historical Drone release.

## Address scope

Unless a document says otherwise:

- `0x004.....` Win32 addresses refer to `BIN-WIN-SW-01` loaded at its normal PE image base;
- DOS linear addresses in `reverse/dos/function_map.csv` refer to `BIN-DOS-SW-01` under the current LE analysis mapping.

Do not transplant labels to a different executable solely because it has the same filename.

## Adding future evidence

A future retail/full-game, patch, alternate shareware build, or localized release must receive a new evidence ID. Before analysis, record:

- original filename;
- provenance/source description without embedding sensitive personal information;
- SHA-256;
- byte size;
- executable/container type;
- version strings and timestamps when present;
- relationship to known builds;
- whether it may be redistributed (default: **no** unless clearly established).

Never replace an old hash in place just because a newer/better build appears. The historical evidence record should remain stable.
