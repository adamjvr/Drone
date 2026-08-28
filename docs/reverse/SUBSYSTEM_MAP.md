# Evolving Engine Subsystem Map

This map separates known original-executable behavior from the desired clean architecture. Provisional names are evidence-backed research labels, not claimed original source identifiers.

## Win32 outer loop — high confidence

```text
0x00404E30  winmain-like
   |
   +--> 0x00404B60  DirectX/platform init
   |       +--> DirectDraw / 320x200x8 / primary surface / palette
   |       +--> 0x004068E0 DirectInput creation
   |       +--> 0x00406650 DirectSound init
   |
   +--> message pump
   +--> lock/acquire surface
   +--> 0x0040BA50 game_dispatch_update
   +--> unlock/present
   +--> shutdown
```

## State/gameplay orchestration — protocol confirmed, gameplay partition partial

`0x0040BA50` directly dispatches raw values 0..5. The surrounding menu/modal/gameplay paths now establish the user-facing protocol for `0..8`, `13`, and `99`: exit, menu reset-entry, gameplay, instructions, menu re-entry, pause, quit confirmation, ordering information, high scores, demo launch, and the nine-lives notice. State 2 remains the large active-gameplay region beginning near `0x0040BAB9`; call-site inventory shows that region is primarily an orchestrator over many smaller routines rather than one indivisible algorithm. See [`../../reverse/windows/state_machine.md`](../../reverse/windows/state_machine.md).

The Microsoft-runtime PRNG anchors are now established at:

- `0x00421EC0` — seed setter (`srand`-like);
- `0x00421ED0` — `rand` implementation returning 15 bits.

These calls are common throughout gameplay and provide an important deterministic-validation dependency once seed ownership is mapped.

## Timing — canonical DOS fidelity cadence confirmed

- DOS `0x0006940C`: confirmed VGA vertical-retrace wait via port `0x3DA`, bit `0x08`.
- DOS startup selects BIOS mode 13h; the ordinary gameplay replay/update tail performs one retrace wait when sync is enabled.
- Clean DOS fidelity cadence: standard mode-13h **~70.0863 Hz**.
- Windows `0x00411456..0x00411494`: separate QPC low-32 busy-wait to 15,000 counts when guard enabled; effective historical wall-clock cadence remains nonblocking and frequency-dependent.

See [`../TIMING.md`](../TIMING.md).

## Rendering — partial

Confirmed:

- logical display is 320×200×8 indexed;
- full-screen JBA assets decode into that logical shape;
- `0x00401660` is a clipped transparent sprite blitter;
- source index `0` is transparent;
- X position, Y position, width, height, current frame, and frame-table fields of the common Win32 object are known;
- clean `IndexedFramebuffer` and native `drone_fidelity_host` now preserve this indexed contract.

Newly established presentation/UI details:

- `0x004D9594` is a guarded software framebuffer; the allocation is 204 rows and the logical 200-row framebuffer begins after two guard rows;
- `0x004D9584` / `0x004DA780` are the locked DirectDraw surface pointer/pitch;
- `0x00406B80` copies exactly the logical 320×200 frame to the pitched surface;
- `0x00401470` renders bitmap text from `font2.jba` and is used by menus/status overlays; its formerly unknown 0x14-byte records are now established as a 64-entry FONT2 glyph cache with an exact DOS counterpart at `0x000809B0`.

See [`FRAMEBUFFER_PIPELINE.md`](FRAMEBUFFER_PIPELINE.md).

Still open:

- all blitter variants;
- frame-image load/packing routines;
- HUD composition;
- scrolling background/scenery composition;
- dynamic palette effects and state-specific palette upload scheduling are established; remaining Phase-3 work is small-JBA completion plus HUD/effect/world layering.

## Sprite/entity object layer — partial

Win32 `0x00401780` initializes a common `0x154`-byte object and `0x00401820` releases its frame allocations. The same object participates in trajectory groups and many gameplay loops.

DOS has the corresponding `0x14F`-byte family initialized at `0x00068220`. Init/free/blit/collision/destruction evidence establishes a field-level correspondence, including the two-byte pre-frame-table layout shift and tail combat metadata. See `ENTITY_LAYOUT.md`.

See [`STRUCTURE_RECOVERY.md`](STRUCTURE_RECOVERY.md) and [`COLLISION.md`](COLLISION.md).

## Trajectory/flight-path layer — core lifecycle confirmed

Runtime FLY assets are literal raw X/Y/AUX trajectories, not generic count-prefixed scripts. X/Y positions, path index/step/wrap, and normal AUX sprite-frame control are established. The canonical Win32 trajectory pool contains 17 fixed `0x2148`-byte group records; all 17 startup templates are cataloged, and modes 0/1/2/10, active/activated counts, stagger activation, activity-3 path acquisition, mode-2 wrap retirement, and randomized mode-10 16.16 breakaway are recovered and clean-tested. Dynamic special-family substitutions/producers remain under classification.

Known Win32 path-array triplets include Loop, Leftdrop, Ritedrop, Frisbee1/2, Leftdive, Rightdiv, Swarm, Swoop, and Newcurly. Loader counts and canonical-file quirks are recorded in `manifests/fly_trajectories.csv`; the fixed startup group catalog is complete, while dynamic special-family substitutions/producers remain partial. See [`TRAJECTORY_GROUPS.md`](TRAJECTORY_GROUPS.md).

## Input / player — canonical action boundary confirmed

- DirectInput creation: `0x004068E0`.
- `0x00406AC0` = `poll_directinput_joystick_state`; it polls a `0x50`-byte joystick state buffer.
- `0x00420090` = `normalize_joystick_gameplay_actions`; axis dead zone is ±360 and four configured button indices produce rapid/load/launch/shield action bytes.
- Win32 gameplay ORs keyboard and normalized joystick actions when live, but demo playback replaces exactly channels 1–6 (Left/Right, launch, load/cycle, shield, rapid).
- Live `A`/`Z` vertical movement remains outside replay substitution; DOS has the same architectural split between six replayed controls and live vertical input.
- DOS `0x0006BAD0`, `0x0006BB4C`, and `0x0006BC10` normalize game-port buttons/horizontal/vertical directions and converge on the same six replay action meanings.
- Player entity root `0x00466B18`, exact playfield clamps and 15-frame banking behavior are established and implemented in the clean core.
- `GameplayInputFrame` is the portable clean semantic boundary; it is not claimed to be an original packed structure.
- Win32 Tab writer is established: `VK_TAB` negates `0x0042B1B4` +1/-1 and its UI reports `VERTICAL RETRACE` / `SYNC ON/OFF`; the DOS fidelity cadence is now established separately at ~70.0863 Hz.

See [`INPUT.md`](INPUT.md) and [`PLAYER.md`](PLAYER.md).

## Audio — partial

- DOS: HMI-era audio and CLV stereo sample data.
- Windows: RIFF/WAV + DirectSound.
- Win WAV loader: `0x00406200`.
- DirectSound init: `0x00406650`.

Gameplay event mapping, priority, channel ownership, and exact platform differences remain open.

## Shield — energy/protection core reconstructed

Space (and demo replay channel 5) drives a 75-unit high-word shield accumulator at `0x0047FCB0`. State 2 recharges `+0x514` before input, drains `0xBB80` while requested, clamps negative energy to zero, and sets protection flag `0x0046198C` only while energy remains. The clean module preserves the original high-word-only recharge/overshoot quirk. `0x0041EB70` renders the unit meter; `0x0041E6D0` is the shield visual effect (role established, exact pixels partial). Enemy-bomb collision proves the active flag prevents player destruction and converts the bomb to an effect path. See [`SHIELD.md`](SHIELD.md).

## Rapid-fire projectiles — core lifecycle reconstructed

The normal Ctrl-fire missile path is now connected from input through asset initialization, allocation, movement, top-bound cleanup, collision and rendering. It uses an eight-entry `0x154` pool rooted at `0x0042F200`, three 1×9 frames from `missile.jba`, and a 20-voice `missile.wav` DirectSound pool. The clean core implements the established lifecycle while target-specific collision consequences continue to be named.

See [`RAPID_MISSILES.md`](RAPID_MISSILES.md).

## Gameplay collision/effects — partial but implemented

Three collision primitives are established from Win32 code, including rectangular hitbox and per-pixel opaque-sprite testing. Clean semantic versions live in `drone_core` with synthetic tests. Explosion/effect helpers also establish velocity fields and several pool/audio relationships. See [`COLLISION.md`](COLLISION.md) and [`GAMEPLAY_EFFECTS.md`](GAMEPLAY_EFFECTS.md).

## Levels/demo — open/partial

The trajectory group lifecycle and hybrid demo channels are substantially mapped. Encounter sequencing is also concrete: a six-way boss initializer dispatch is indexed by processed Drone count when the canonical Drone objective reaches Y=-200, and a separate post-outcome transition chooses a three-screen scenery family or the shareware termination path. The cyclic world-scroll cadence/ownership is now exact; the proper registered-slot-2 asset identity and registered/endgame transition details remain open.

## State-2 call inventory

`reverse/windows/gameplay_call_inventory.csv` records all direct call targets in the canonical Win32 state-2 range (`0x0040BAB9..0x00411D86`). It currently contains 77 distinct targets. The high density of calls to `crt_rand` and the transparent sprite blitter, mixed with audio/input/render helpers and unresolved routines, reinforces that this region is an orchestrator. Counts are static call sites, never runtime profiling.

### Enemy bomb pool

- pool root: `0x004651A0`; capacity `0x0042B1A4 == 10`; active count `0x00446F6C`;
- asset: `bomb.jba`, three shared 1×9 frames;
- live spawn: trajectory-group-derived X/Y plus `rand()%3` horizontal steering magnitude;
- demo spawn: channels 10–12 restore X/Y and explicitly force steering magnitude to zero;
- update: optional 3-frame animation tick, horizontal steering, `Y += 2`;
- visibility flag bounds `X 0..319`, `Y 0..190`; later retirement `Y > 198`;
- collision: Probe/Stinger and player paths located; terminal consequences remain partial.

See [`ENEMY_BOMBS.md`](ENEMY_BOMBS.md).


## Boss/encounter dispatch — partial, strong

At the Drone approach boundary `drone_target_entity.position_y (0x00446084) == -200`, state 2 dispatches one of six boss initializers using `drone_outcome_processed_count` (`0x00433B54`). The established family order is Lid/Top, Gemini, registered-slot-2 unknown, Spidey, Lid/Top reused, Bomber. Lifecycle addresses, resource-presence boundaries and destruction checkpoints are maintained in [`BOSS_PROGRESSION.md`](BOSS_PROGRESSION.md) and the family-specific documents. Only slots 0/1 are normally reached by the canonical shareware campaign.

This boss family is distinct from the registered Mothership subsystem at `0x00413290`/`0x00413870`.

## Scenery/world buffer — partial, strong

`0x004D9598` points to three contiguous 320×200 indexed screens. River and desert scenery are present in the shareware corpus; isle/house/night strings and transition branches survive in the executable even though those JBA files are absent from the installed shareware payload. A dedicated branch after the second processed Drone objective terminates the shareware run, matching the documented two-level limit.

See [`WORLD_SCENERY.md`](WORLD_SCENERY.md).


## Post-game / results boundary

The state-2 lives gate enters inline post-game processing at `0x004115BE` when `player_lives <= 0`. That region is now partitioned through result reduction/statistics, a 58-present confirmation lock, optional state-7 Ordering Information, high-score qualification/insertion, and perfect-completion credits. Ordinary no-score return is state 1; ordinary qualifying insertion returns state 4; six-disarm Mothership completion runs credits and returns state 1. See [`POST_GAME_FLOW.md`](POST_GAME_FLOW.md).


### Dynamic palette effects

The Win32 presentation path now has established palette ownership: `0x004011E0` uploads inclusive DirectDraw ranges, `0x0041EFE0` initializes the four purpose-built gameplay bands, `0x0041EE90` advances them, and `0x00403490` handles the separate generic late-game animation bands. Settled state 2 distributes palette uploads by the four-phase scheduler; unstable paths upload all 256 entries. See [`PALETTE_EFFECTS.md`](PALETTE_EFFECTS.md).
