# World Scenery and Mission Progression

## Scope

This document records the recovered Win32 scenery-buffer and post-Drone progression behavior. It deliberately distinguishes **physical scenery mechanics** from guessed “level numbers.” The executable contains registered-game scenery strings and branches that are not all reachable through normal shareware campaign play.

## Three-screen vertical scenery buffer

Global pointer `0x004D9598` owns a contiguous **192,000-byte** indexed world/scenery buffer:

```text
3 * (320 * 200) = 192000 bytes
```

A recurring loader pattern decodes a normal 320×200 JBA into the temporary decoded image buffer and copies exactly 64,000 bytes into one of:

```text
world + 0x00000   top screen
world + 0x0FA00   middle screen
world + 0x1F400   bottom screen
```

This is concrete evidence that scrolling scenery is assembled from three stacked logical 320×200 indexed images rather than one giant proprietary background format.

The session initializer `0x00417F50` uses this exact layout for the initial **river** family:

```text
rivertop.jba
rivermid.jba
riverbot.jba
```

## Runtime viewport compositor — `0x004033D0`

The late state-2 call at `0x004100D8` consumes scroll-row scalar `0x004D9590` and copies the visible 320×200 viewport from the 320×600 scenery buffer into software framebuffer `0x004D9594`.

For canonical start rows `0..400`, all 64,000 bytes are contiguous. For starts `401..599`, the routine copies the remainder through world row 599, then fills the rest of the framebuffer from world row 0. In semantic form:

```text
viewport[y] = world[(scroll_row + y) mod 600]
for y = 0..199
```

This closes the formerly open **physical scrolling-copy** question. The producer side is now recovered too: normal gameplay initializes `0x004D9590` to **599** and decrements it exactly once when the shared four-phase gameplay substep equals `2`; decrementing `0` wraps to `599`. No other active-gameplay writer exists in the canonical PE.

The same global is temporarily reused by the state-7 Ordering Information modal, which resets it to `0` and drives it from a separate local `0 -> 1 -> 2 -> 0` counter. That modal does **not** use the gameplay four-phase scheduler. Its dedicated compositor `0x00403560` copies only framebuffer rows **35..179** (145 rows), leaving the top/bottom UI bands intact.

Clean implementations: `gameplay/world_scroll.*` and `fidelity/world_viewport.*`, with cadence, wrap and modal-band regression tests.

## Scenery families named by the executable

The canonical Win32 binary contains coherent top/mid/bottom string families:

| family | top | middle | bottom | present in supplied Windows shareware install? |
|---|---|---|---|---|
| river | `rivertop.jba` | `rivermid.jba` | `riverbot.jba` | yes |
| desert | `desertop.jba` | `desermid.jba` | `deserbot.jba` | yes |
| isle | `isletop.jba` | `islemid.jba` | `islebot.jba` | no |
| house | `housetop.jba` | `housemid.jba` | `housebot.jba` | no |
| night | `nighttop.jba` | `nightmid.jba` | `nightbot.jba` | no |

The missing files are executable namespace evidence for registered content; their strings do **not** authorize us to synthesize the missing artwork or claim it is present in the shareware package.

Metadata/hashes for present files and explicit absence for referenced registered files are recorded in `manifests/world_scenery_roles.csv`.

## Progress-index transition switch

The now-established `run_mission_outcome_transition` routine at `0x0041D690` presents the objective interstitial and later switches on:

```text
(drone_outcome_processed_count - 1)
```

through jump table `0x0041E400`.

The six target branches are:

| processed outcomes | branch | established scenery/result behavior |
|---:|---|---|
| 1 | `0x0041DC0A` | load desert top/mid/bottom |
| 2 | `0x0041DD55` | **shareware termination gate**; load `deserbot.jba`, force lives to 0 / return toward post-game flow |
| 3 | `0x0041DD86` | load isle top/mid/bottom |
| 4 | `0x0041DED1` | load house top/mid/bottom; also load Lid/Top boss resources |
| 5 | `0x0041E014` | load night top/mid/bottom |
| 6 | `0x0041E157` | if cumulative detonations are zero: reload river stack and stage Mothership endgame; otherwise load `nightbot.jba`, force lives to zero, and return toward results |

This is one of the clearest executable manifestations of the shareware boundary. The supplied README says the shareware version contains **two levels**. The branch selected after the second processed Drone objective does not load a third full scenery family; it forces the run toward termination instead. Registered-only branches for later scenery families remain compiled into the executable even though their referenced JBA files are absent from the supplied Windows shareware installation.

This is a stronger statement than merely noticing missing files: control flow itself contains a dedicated second-objective shareware stop.

The formerly ambiguous branch-6 relationship is now executable-backed. After the common encounter-only reset, processed count `6` plus **zero detonated outcomes** releases the Bomber family, calls `load_mothership_assets` (`0x00413290`), restores the Drone to Y=`-850`, initializes the Mothership composite/core state, and stages the endgame. Any nonzero detonated count instead forces the run toward results. Thus the latent compiled contract is exactly **all six Drones disarmed -> Mothership**. This establishes intent in the shareware PE but does not substitute for later byte/runtime comparison against a lawful retail executable.

## Initial and demo scenery

### Normal session

`0x00417F50(1)` initializes the game with the river three-screen stack and performs a full campaign reset, including the six-Drone outcome ledger, processed count, score/life state and encounter machinery. `0x0041D690` later calls `0x00417F50(0)` after an objective transition; the zero form jumps over campaign-wide reset work and performs only the shared per-encounter reset, preserving mission-wide outcomes/score/lives.

### Attract/demo setup

Attract/demo setup around `0x0041A244` loads the **desert** top/mid/bottom stack and later calls the Lid/Top boss resource loader. This is another reason campaign reachability and resource reachability must be tracked separately: demos can stage assets outside the ordinary new-game progression context.

## Drone approach / boss selection boundary

The main active-gameplay boss switch is **not** keyed from the scenery scroll row. `0x00446080` is independently established as the canonical `drone.jba` common-entity root, so `0x00446084` is simply that entity's common `+0x04` **position Y** field. Session initialization writes the Drone position `(155,-850)`, and demo channels 13/14 record/replay the same entity X/Y pair.

When the Drone objective's Y coordinate reaches exactly **`-200`**, state 2 indexes a six-entry boss-initializer table using the current processed-Drone count **before the next Drone outcome is committed**. This is therefore a **Drone approach boundary**, not a second world-scroll coordinate.

| current processed count | boss initializer |
|---:|---|
| 0 | `0x00417220` — Lid/Top boss family |
| 1 | `0x00405EF0` — unresolved boss family |
| 2 | `0x00407980` — unresolved boss family |
| 3 | `0x00415AC0` — unresolved boss family |
| 4 | `0x00417220` — Lid/Top boss family reused |
| 5 | `0x00404350` — unresolved boss family |

This aligns structurally with the README's statement that boss ships appear immediately before Drones. It also gives us a finite queue for boss-family archaeology instead of searching the entire state-2 region blindly.

See [`LID_TOP_BOSS.md`](LID_TOP_BOSS.md).

## Architectural consequence for the clean engine

Fidelity reconstruction should model scenery as indexed logical screens/bands and progression as data/state, not bake these particular filenames into a renderer.

A clean conceptual split is:

```text
Mission progression
       |
       +--> chooses boss/encounter family
       |
       +--> commits Drone outcome
       |
       +--> chooses next scenery stack / shareware termination

Scenery stack
       |
       +--> 320x200 top
       +--> 320x200 middle
       +--> 320x200 bottom
       |
       +--> scrolling/presentation logic
```

The fidelity layer may initially import original JBA scenery. A later remaster renderer can replace visual assets without changing progression semantics.

## Established vs open

### Established

- `0x004D9598` points to a contiguous three-screen 320×200 indexed scenery buffer;
- river and desert three-file families are present in the shareware corpus;
- isle/house/night families are named by the executable but absent from the installed shareware corpus;
- initial normal session loads river top/mid/bottom;
- demo setup can load desert top/mid/bottom;
- post-outcome switch is indexed by `drone_outcome_processed_count - 1`;
- outcome 1 selects desert;
- outcome 2 contains a dedicated shareware termination path consistent with the documented two-level limit;
- registered/dormant branches for outcomes 3–5 select isle/house/night scenery families;
- branch 6 reloads river and enters the Mothership path only when the cumulative detonated count is zero; any detonation forces the terminal/results path;
- the transition calls `initialize_gameplay_session(0)`, an encounter-only reset that preserves mission-wide outcomes/score/lives;
- active gameplay initializes the scenery row to 599 and decrements/wraps it only on gameplay phase 2;
- Ordering Information is the only non-gameplay owner of that row and uses its own three-step local cadence plus a 145-row modal compositor;
- `0x00446084` is the Drone objective common entity `position_y`, and the boss switch is indexed by processed Drone count when that Y reaches `-200`.

### Open

- exact original “level” numbering/names for every scenery family;
- full registered reachability and transition behavior once a lawful retail reference is obtained;

The scrolling cadence/ownership, Drone-approach coordinate split, branch-6 Mothership condition, and campaign-vs-encounter reset split are no longer open. `Q-LEVEL-001` is now limited to exact retail reachability/historical level naming once lawful full-game evidence is available; registered boss slot-2 asset identity remains under `Q-BOSS-002`.
