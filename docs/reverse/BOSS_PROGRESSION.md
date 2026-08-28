# Boss Progression and Dispatch

This document records the normal pre-Drone boss dispatcher in the canonical Win32 shareware executable. Names are evidence-backed research labels derived from asset/resource ownership; they are not claimed original source identifiers.

## Dispatch contract

When the Drone objective reaches its Y=-200 approach boundary, state 2 indexes a six-entry jump table at `0x00411E24` using `drone_outcome_processed_count` (`0x00433B54`). The thunks lead to the following initializers:

| processed Drone outcomes | thunk | initializer | family | canonical shareware reachability |
|---:|---:|---:|---|---|
| 0 | `0x0040E836` | `0x00417220` | Lid/Top | reachable |
| 1 | `0x0040E844` | `0x00405EF0` | Gemini | reachable |
| 2 | `0x0040E828` | `0x00407980` | registered slot 2 — exact identity unknown | not normally reachable |
| 3 | `0x0040E82F` | `0x00415AC0` | Spidey | registered progression |
| 4 | `0x0040E836` | `0x00417220` | Lid/Top reused | registered progression |
| 5 | `0x0040E83D` | `0x00404350` | Bomber | registered progression |

The clean `gameplay/boss_progression` module preserves this dispatch contract without embedding original executable addresses into simulation code.

## Shareware boundary

The dispatch table contains registered-game encounters, but the canonical shareware campaign does not normally progress into slot 2. The world-transition code explicitly terminates the run after the second processed Drone outcome by forcing lives to zero instead of continuing to the third scenery/encounter family. Therefore:

- slots 0 and 1 are normal shareware campaign content;
- slots 2..5 are useful executable archaeology but are not shareware parity requirements unless separately reachable through demo/debug paths;
- resource strings/code existing in the PE does not prove the corresponding registered assets are present in the shareware install.

See [`WORLD_SCENERY.md`](WORLD_SCENERY.md).

## Established family lifecycle map

| family | update | initialize | load resources | release resources | destruction checkpoint |
|---|---:|---:|---:|---:|---|
| Lid/Top | `0x00416700` | `0x00417220` | `0x00417350` | `0x00417450` | lid state 2 reaches 25 -> +100/top state 2; top then retires after 30 phase-2 ticks |
| Gemini | `0x00405000` | `0x00405EF0` | `0x00405FB0` | `0x00406190` | two independent body-side +100 destruction entries; each body retires after 20 phase-2 ticks |
| Registered slot 2 unknown | `0x00406CC0` | `0x00407980` | **not present/identified in canonical shareware PE** | `0x00407AB0` | state 2 counter reaches 45; +100 |
| Spidey | `0x00414D80` | `0x00415AC0` | `0x00415C80` | `0x00415F40` | state 2 counter reaches 45; +100 |
| Bomber | `0x00403650` | `0x00404350` | `0x004044B0` | `0x00404690` | state 2 counter reaches 60; +100 |

The Gemini total encounter score must not be simplified to a single generic +100 because the executable contains independent +100 threshold-crossing branches for its two major bodies. Phase 4 preserves those awards and 20-phase-2-tick body retirements independently; exact hit/damage-threshold production remains documented in [`GEMINI_BOSS.md`](GEMINI_BOSS.md).

## Resource-presence rule

`manifests/boss_resource_roles.csv` records known resource names and whether each is present in the supplied Windows shareware corpus. This manifest deliberately distinguishes:

1. resource names proved by executable literals/loaders;
2. assets actually present and hashable in the canonical shareware install;
3. dormant registered code whose dedicated loader/resources are not recoverable from the current evidence set.

This distinction prevents later work from quietly treating absent registered content as if it had been supplied.
