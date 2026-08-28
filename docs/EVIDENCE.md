# Evidence Set and Confidence Rules

The canonical identities and sizes are maintained in [`PROVENANCE.md`](PROVENANCE.md). This file summarizes what the evidence set can and cannot prove.

## Canonical source packages

| package | SHA-256 |
|---|---|
| `drone_sw.zip` | `ceba0a398a6b3260b415227383bae35b7ffac442a723329581bb2007f023d192` |
| `drone_sw(win).zip` | `4f4c0d4c7d0333f1066b1d3f0e4ff622e4df5d911a7b7440eb2ebb9e4407837d` |

## Principal executables

| executable | identity |
|---|---|
| DOS `DRONE_SW.EXE` | 32-bit x86 Watcom C/C++; Linear Executable; DOS/4GW |
| Windows `Drone_sw.exe` | PE32/i386; linker major 5; PE timestamp observed as 1999-03-26; native Win32 GUI |

The Windows executable imports DirectDraw, DirectInput, DirectSound, WinMM, and `QueryPerformanceCounter`. It does **not** import `QueryPerformanceFrequency`.

## Corpus metadata

Known canonical shareware corpora:

- DOS: 187 extracted files / 17,277,984 bytes;
- Windows: 192 reconstructed installed files / 12,354,961 bytes.

Complete path/size/SHA-256 inventories are checked in as metadata:

- `manifests/dos_shareware_files.csv`;
- `manifests/windows_shareware_files.csv`.

## What this evidence can establish

The two shareware builds are enough to reconstruct substantial shared engine behavior, formats, platform contracts, menu/gameplay paths, and the first two levels' available content. Shared behavior independently visible in both binaries is particularly strong evidence.

## What this evidence cannot establish by itself

The shareware packages do not contain the complete registered game payload. Strings naming later-level assets prove code paths/names exist; they do not provide those assets or guarantee every retail build uses identical addresses/behavior.

Full levels 3–7 parity requires a separately identified lawful full-game evidence set.

## Confidence vocabulary

- **high / confirmed** — direct and unambiguous binary behavior, exact reproduction, or strong independent corroboration;
- **medium** — strong structural relationship with some semantic uncertainty;
- **low / hypothesis** — navigation aid or plausible interpretation not yet proven;
- **rejected/superseded** — retained record of a disproven or replaced claim.

The detailed finding lifecycle is defined in `docs/RE_HANDBOOK.md`.

## DOS video-mode evidence

Canonical startup at `0x0006E005` passes `0x13` to `0x00067E50`, which constructs BIOS interrupt `INT 10h` with `AH=0`. The DOS build therefore explicitly selects standard VGA/MCGA mode 13h (320x200x256). This is used in the timing reconstruction together with the independently recovered VGA retrace gate.
