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

## Confirmed/established relationships

| subsystem | DOS evidence | Windows evidence | status |
|---|---|---|---|
| full-screen JBA load | `0x00067F90` | `0x004012B0` | high-confidence algorithm correspondence |
| palette update | `0x00067F1C` candidate | `0x004011E0` | medium correspondence; surrounding platform details differ |
| audio asset family | CLV/HMI path near `0x0007ECB4` | WAV loader `0x00406200` + DirectSound | behavioral/data correspondence; not function equivalence |
| FLY trajectories | raw/count parser family; DOS consumer mapping in progress | hard-coded raw trajectory loaders + trajectory groups | X/Y, normal AUX frame control, path index/step/wrap, and short-loader-slot reachability established; DOS consumer mapping/special families still partial |
| C runtime file parsing | Watcom runtime anchors | Microsoft runtime-like anchors | useful analysis anchors, not game-function correspondence |
| frame pacing/presentation timing | VGA vertical-retrace wait `0x0006940C` | QPC limiter in gameplay path | behavioral correspondence candidate; exact simulation cadence unresolved |
| common sprite/entity object | `0x14F` family initialized at `0x00068220` | `0x154` family initialized at `0x00401780` | **established field-level correspondence**; DOS frame/status/tail region is systematically two bytes earlier and Win32 adds three tail bytes; see `ENTITY_LAYOUT.md` |
| legacy high-score persistence | save/load `0x000894FC` / `0x000898C8` plus helpers | save/load `0x0041F120` / `0x004174D0` plus helpers | high-confidence exact physical-format algorithm correspondence: ten records, 30-byte name stride, four numbers, identical random-padding scheme |
| gameplay input actions | game-port normalizers `0x0006BAD0` / `0x0006BB4C` plus state-2 keyboard/replay consumers | DirectInput poll/normalizer `0x00406AC0` / `0x00420090` plus state-2 keyboard/replay consumers | high-confidence semantic correspondence: independent live actions converge on the same six replay controls; physical APIs remain platform-specific |
| FONT2 bitmap text/cache | `0x000809B0` + `0x000694EC`/`0x0006958C`/`0x00083CB0` | `0x00401470` + `0x004015D0`/`0x00401630`/`0x00401570` | exact cross-build UI correspondence: 64×0x14 descriptors, 16×4 gutter grid, 7×5 masks, character-0x20 indexing, width*height+1 allocation and nonzero-mask rendering |

## Audio backend contract boundary

`CORR-AUD-001` remains a **behavioral candidate**, not a one-to-one function mapping. The public Human Machine Interfaces S.O.S. 4.x SDK headers establish the middleware API used by the DOS audio family: caller-owned `_SOS_SAMPLE` descriptors carry volume, loop count, sample rate, pan, priority and callbacks; `sosDIGIStartSample` starts a descriptor and the API exposes explicit stop/status plus volume/rate/pan controls. The library constants expose a default 32-channel mixer and 32-voice capability ceiling.

Those facts do not prove that Drone configured 32 voices, nor do they prove a particular game-level allocation or steal policy. The canonical DOS function map still places `hmi_sample_loader` at `0x0007ECB4`, but the exact start/stop/control callers must be re-read before upgrading the relationship.

The Win32 side is already stronger and importantly different: Drone explicitly preduplicates selected high-overlap sounds into 20-buffer DirectSound pools and implements its own first-raw-status-not-1 / else-slot-0 selector. Because an HMI descriptor start does not require such a pool at the middleware boundary, the future portable audio layer must not treat the Win32 20-voice policy as universal. Shared semantic cue/control events are appropriate; backend voice arbitration remains platform evidence.

The unresolved DOS specifics are tracked as `Q-AUDIO-003` through `Q-AUDIO-007`.

## Procedure for adding a mapping

A new correspondence row should include:

- DOS address/name;
- Windows address/name;
- relationship type;
- confidence;
- short evidence statement;
- status (`candidate`, `established`, `rejected`, `superseded`).

Do not create a mapping just because two routines are similar in size or appear at similar points in a file. Prefer shared constants, data flow, call role, asset interaction, and runtime behavior.

## Mothership asset lifecycle

`CORR-MSHIP-001` is an established cross-build correspondence. The DOS/Watcom pair `0x0006C964` / `0x0006CDB0` and Win32 pair `0x00413290` / `0x00413870` load and release the same coherent Mothership object family: `hull0..3`, `panel0..3`, `damage0/1`, `hub`, `motor`, and `hole`.

DOS xrefs are recovered from LE internal fixup records rather than inferred from raw immediates. See `reverse/dos/mothership_asset_xrefs.csv` and `scripts/find_dos_le_internal_xrefs.py`. This establishes the asset-lifecycle correspondence; the DOS gameplay/destruction state machine is the next cross-build target.
