# Asset Catalog — Shareware Evidence

## DOS package

Observed package inventory: **187 files / 17,277,984 bytes**.

| class | count | role |
|---|---:|---|
| `.JBA` | 94 | indexed images/screens/sprites/background strips |
| `.CLV` | 56 | raw 22.05 kHz unsigned-8 stereo audio |
| `.FLY` | 12 | trajectory/script records |
| demo `.DAT` | 4 | recorded 14-field frame/state records |
| `.386` | 3 | Human Machine Interfaces sound drivers |
| `.EXE` | 3 | game, installer/setup, DOS4GW ecosystem |
| `.BNK` | 2 | HMI instrument banks |
| misc HMI/audio/config | several | setup/test/runtime support |

## Reconstructed Windows installation

Observed installation inventory: **192 files / 12,354,961 bytes**.

| class | count | role |
|---|---:|---|
| `.JBA` | 106 | images/screens/sprites/background strips |
| `.WAV` | 61 | Windows PCM replacements/additions |
| `.FLY` | 12 | same trajectory/script family |
| demo `.DAT` | 7 | expanded demo set |
| `.EXE` | 1 | native Win32 game |
| misc | 5 | scores/readme/order/icon |

### Windows-only JBA names in the supplied shareware install

`Logo`, `Minidrg`, `Miniprb`, `Miniprg`, `Miniprr`, `Mission3`, `Redprobe`, `River`, `Screen`, `Splash0`, `Square`, `Top_hole`, `Top_tab`, `Wincred`, plus a Windows `Disarm2` variant.

`Logo.jba` (7,494 bytes), `River.jba` (10,732 bytes), and `Screen.jba` (8,923 bytes) are **not** the normal 64,768-byte full-screen JBA form. Milestone 0 found embedded 128×128 8-bit PCX data inside at least this small-JBA family; their surrounding container bytes still need semantic reconstruction.

### DOS-only JBA names

`Coldblod`, `Disarm2s`, `Ordrshar`.

### Demo-set difference

Windows adds `Demoa1.dat`, `Demoa3.dat`, and `Demob2.dat`. The four files shared with DOS remain useful for cross-version behavioral matching.

### Audio-set difference

Windows-only stems observed in the installed shareware set: `Doorclos`, `Dooropen`, `Firebal2`, `Pulse` plus Windows-side `Msidle`/`Test` WAV resources. DOS-only CLV stem: `Top1`.

## Full-game names embedded in the Win32 executable

The native executable contains strings naming assets not present in this shareware installation, including later level/scenery/mission families such as `level3.wav`, `level4.wav`, `night*`, `house*`, `isle*`, `mission4`, `mission5`, `miss6yes`, `miss6no`, `good3`–`good6`, `bad3`–`bad6`, `spidey*`, and credit resources.

These strings prove code/content paths exist in the executable. They do **not** supply the missing retail payload and are not a substitute for obtaining a lawful full-game reference copy later.

## Canonical per-file metadata manifests

The repository now preserves complete metadata-only inventories for the Phase 1 shareware corpus:

- `manifests/dos_shareware_files.csv` — 187 file rows;
- `manifests/windows_shareware_files.csv` — 192 file rows.

Each row contains relative path, byte size, SHA-256, and extension. These manifests make corpus comparisons and future asset-format work reproducible without committing any proprietary file bytes. Local bootstrap generates independent inventory CSVs under `.reference/work/` for comparison.
