# DOS ↔ Windows Correspondence

## Purpose

Drone provides an unusually useful comparison: an earlier Watcom/DOS4GW build and a later native Win32 build share game concepts while using very different platform/runtime layers. Correspondence between the two is one of the project's strongest tools for distinguishing true game behavior from decompiler/compiler artifacts.

The machine-readable table is `reverse/correspondence/dos_windows.csv`.

## Correspondence classes

### Exact algorithm correspondence

Used when both binaries independently implement the same recoverable transform. Current strongest example: full-screen JBA loading/deinterleave.

### Behavioral correspondence

Used when different code/platform mechanisms implement the same game contract. Example: original DOS audio loading through HMI versus Windows WAV/DirectSound paths.

### Data-layout correspondence

Used when matching record widths/field accesses establish a shared data format or logical structure.

### Namespace/content correspondence

Shared asset names, demo sets, level names, and load sequences can identify the role of a path even when function bodies differ greatly.

### Platform-specific / no correspondence expected

Examples include DirectDraw surface management, DirectInput object creation, DOS VGA palette I/O, and HMI sound-driver plumbing. These help define host interfaces but should not be forced into one-to-one function mappings.

## Confirmed/established Phase 1 relationships

| subsystem | DOS evidence | Windows evidence | status |
|---|---|---|---|
| full-screen JBA load | `0x00067F90` | `0x004012B0` | high-confidence algorithm correspondence |
| palette update | `0x00067F1C` candidate | `0x004011E0` | medium correspondence; surrounding platform details differ |
| audio asset family | CLV/HMI path near `0x0007ECB4` | WAV loader `0x00406200` + DirectSound | behavioral/data correspondence; not function equivalence |
| FLY records | original consumers/parser family | original consumers/parser family | physical layout established; semantic/function map incomplete |
| C runtime file parsing | Watcom runtime anchors | Microsoft runtime-like anchors | useful analysis anchors, not game-function correspondence |

## Procedure for adding a mapping

A new correspondence row should include:

- DOS address/name;
- Windows address/name;
- relationship type;
- confidence;
- short evidence statement;
- status (`candidate`, `established`, `rejected`, `superseded`).

Do not create a mapping just because two routines are similar in size or appear at similar points in a file. Prefer shared constants, data flow, call role, asset interaction, and runtime behavior.
