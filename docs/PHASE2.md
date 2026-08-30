# Phase 2 — Gameplay Reconstruction

**Status:** complete.  
**Scope rule:** Phase 2 is one engineering phase. The workstreams below are not numbered subphases and should not become separate roadmap phases unless the project scope materially changes.

Phase 2 turns the Phase 1 archaeology/tooling foundation into an executable model of Drone's gameplay architecture. The immediate objective is not to write a speculative remake. It is to recover enough original structure, timing, trajectory, rendering, and state behavior that clean code can prove each interpretation.

## Completion gate

Phase 2 closed after the common Win32 `0x154` / DOS `0x14F` entity correspondence was promoted from a partial candidate to an established cross-build lifecycle contract. At closure:

- all machine-tracked `critical` architecture questions are resolved;
- state, timing, semantic input, trajectory, collision/projectile, world-scroll, mission-progression, post-game, and entity-layout contracts exist in clean code or durable evidence;
- several meaningful gameplay subsystems execute in `drone_core` with synthetic regression coverage;
- the native indexed-framebuffer host builds on the current Linux validation environment;
- canonical DOS/Windows evidence identities and the existing gameplay probe remain stable.

`python3 scripts/check_phase2_exit.py` makes this boundary durable. It deliberately does **not** require Phase-3 rendering completion, Phase-4 full simulation, Phase-6 trace parity, or Phase-8 retail evidence.


## Confirmed during the current Phase 2 checkpoint

### DOS vertical-retrace primitive

Canonical DOS linear address `0x0006940C` is a dedicated VGA retrace wait. It polls input-status port `0x3DA` until bit `0x08` is set:

```text
push edx
mov  edx, 0x3DA
xor  eax, eax
wait:
    in   al, dx
    test al, 0x08
    je   wait
pop  edx
ret
```

This is direct binary evidence for the README's “vertical retrace sync” terminology. Subsequent cross-build placement work established that ordinary sync-enabled DOS gameplay performs one such retrace wait at the shared logical replay/update tail while running BIOS mode 13h. The clean DOS fidelity scheduler therefore uses the standard mode-13h cadence of approximately **70.0863 Hz**. The Win32 15,000-QPC-count limiter remains a separate historical-port timing question because the executable does not query QPC frequency.

### Reproducible DOS LE object extraction

`scripts/extract_dos_le.py` now reconstructs the DOS/4GW LE load objects using the executable's LE object table and page map. On the canonical shareware executable it reproduces the independently extracted object-1 byte stream exactly.

Canonical object map:

| object | relocation base | virtual size | pages |
|---:|---:|---:|---:|
| 1 | `0x00010000` | `0x00093158` | 148 |
| 2 | `0x000B0000` | `0x0000040D` | 1 |
| 3 | `0x000C0000` | `0x0000018C` | 1 |
| 4 | `0x000D0000` | `0x00087B60` | 7 |

The initial LE entry point is object 1 + `0x0007BECC`, or linear address `0x0008BECC`.

### Win32 sprite/entity object

Win32 routine `0x00401780` initializes a commonly iterated `0x154`-byte object. Routine `0x00401660` consumes the same layout as a clipped transparent sprite, and `0x00401820` releases its per-frame allocations.

High-confidence fields now include:

- `+0x00`: signed 32-bit X position;
- `+0x04`: signed 32-bit Y position;
- `+0x20`: signed 16-bit sprite width;
- `+0x22`: signed 16-bit sprite height;
- `+0x40..+0xBF`: 32 frame pixel pointers;
- `+0x140`: current frame index;
- `+0x141`: loaded frame count;
- `+0x142`: activity/state byte; values `0`, `1`, and `3` are observed, so this is **not** modeled as a Boolean.

The transparent blitter treats palette index `0` as transparent. Its right-edge clip branch uses the original literal-319 quirk documented in `reverse/FRAMEBUFFER_PIPELINE.md`; the framebuffer-height global is initialized to 200.

DOS `0x00068220` initializes the corresponding `0x14F` entity family; `0x000682D0` releases its frames and `0x00068300` renders it. Collision/destruction consumers establish a field-level correspondence with Win32 `0x154`: the DOS frame table starts at `+0x3E` versus Win32 `+0x40`, later frame/status/tail fields stay two bytes earlier, and Win32 adds three unreferenced tail bytes. Damage/threshold and destruction-burst/score metadata correspond exactly. See `docs/reverse/ENTITY_LAYOUT.md`.


### Sprite-sheet extraction and clean blitter

Win32 `0x00401860` is now established as the sprite-frame extractor used after JBA decoding. It takes a decoded 320×200 sheet, a target `0x154` entity, a destination frame slot, and grid cell X/Y. Cells have a one-pixel gutter:

```text
source_x = cell_x * (sprite_width + 1) + 1
source_y = cell_y * (sprite_height + 1) + 1
```

Canonical `Debris1.jba` (25×18, eight frames) and `Debris2a.jba` (27×17, sixteen frames over two rows) independently validate the call convention. The clean core now reconstructs both this crop operation and `0x00401660`'s palette-index-zero-transparent clipped blit. Metadata-only frame hashes live in `manifests/recovered_sprite_frames.csv`.

### FLY trajectory model corrected

Phase 1's “all FLY files start with a count” claim was too broad and is superseded.

- `CURRENT.FLY` is a special counted form: count + triples.
- Runtime trajectory files are raw ASCII triples with **no count header**.
- The Win32 executable allocates arrays and reads a hard-coded number of records for each known path.
- Runtime consumers copy field 0 and field 1 directly into entity X/Y positions (plus per-entity and group offsets), so these fields are now named `x` and `y` with high confidence.
- Field 2 is `aux` sprite-animation control during normal path following: values `<=1` are signed relative frame deltas; values `>1` select absolute frame `aux-2`, followed by original frame-bank wrapping.

The canonical `Rightdiv.fly` and `Swarm.fly` files are each physically one record shorter than the corresponding Win32 loader loop requests. Their recovered canonical group end indices never reach the missing requested slot: Rightdiv caps at physical index 117 and static Swarm groups cap at 945. Clean tools therefore record the unchecked-loader discrepancy but do **not** manufacture a garbage sample that canonical recovered gameplay cannot consume.

`manifests/fly_trajectories.csv` records physical counts, recovered loader counts, coordinate ranges, hashes, and known mismatches without storing trajectory payload data.

### Trajectory-group structure and update semantics

The Win32 game groups trajectory arrays with inline `0x154`-byte entities. The main updater at `0x00415FA0` now resolves the core group lifecycle in addition to path progression and AUX animation. The canonical pool begins at `0x00495CF0`, contains **17** groups (`0x0042B18C`), and uses a fixed **`0x2148`-byte group stride**. The recovered header has:

- `+0x00`: group mode — `0` inactive, `1` persistent loop, `2` retire-on-path-wrap, `10` breakaway fly-off;
- `+0x01`: signed entity count;
- `+0x02`: active entity count;
- `+0x04`: pointer to three adjacent global slots holding X/Y/AUX array pointers;
- `+0x08`: signed X group offset;
- `+0x0A`: signed Y group offset;
- `+0x0C`: stagger/spawn-delay counter;
- `+0x0E`: stagger/spawn-delay interval;
- `+0x10`: number of fixed inline entities activated so far;
- `+0x14`: first inline sprite/entity object.

Observed spawning code computes:

```text
entity.x = trajectory_x[entity.index] + entity.x_offset + group.x_offset
entity.y = trajectory_y[entity.index] + entity.y_offset + group.y_offset
```

The updater advances trajectory-owned entity `+0x32` by signed 16-bit step `+0x36`, resets it to zero when it exceeds `+0x38`, and then indexes X/Y/AUX with that path index. During animation-phase updates, AUX `<=1` is a signed relative sprite-frame delta and AUX `>1` selects absolute frame `aux-2`, with original frame-bank wrapping.

Non-primary groups stagger fixed-slot activation using `+0x0C/+0x0E`; activation increments both active and activated counts. In mode 2, a path-following entity retires on path wrap and the group becomes inactive when its active count reaches zero. Fully activated live groups can randomly enter mode 10 during update phase 2; members switch to recovered 16.16 fly-off motion, accelerate each axis from `0x8000` by 700 per update up to `0x28000`, target points just outside the screen, and retire beyond X `-59..320` or Y `-59..200`. Demo playback and recording explicitly suppress this random conversion.

### Fixed trajectory templates and Win32 four-phase substep established

The contiguous Win32 startup initializer at `0x00409060..0x00409CD5` now establishes the complete **17-template** fixed formation catalog. The clean catalog records each group's trajectory family, fixed entity count, initial mode/active count, stagger interval, inclusive path end, sprite dimensions, frame count, and the non-default per-slot path-index/formation seeds. The families are Loop, Leftdive, Swarm, Swoop, Newcurly, Frisbee1, Frisbee2, Leftdrop, plus two startup-generated path families identified conservatively as `Generated402` and `Generated422`. The metadata-only table is [`../manifests/trajectory_group_templates.csv`](../manifests/trajectory_group_templates.csv).

One original startup quirk is preserved explicitly rather than normalized away: group 15's runtime descriptor selects `Generated422`, but its inactive-slot initial X/Y sample is read from `Generated402`. Also, `sprite_entity_init` does not initialize common-entity `+0x36`; only the primary Loop template is statically proven to write path step `+1`, so the catalog does not invent a default step for groups 1–16.

The state-2 shared scalar at `0x0053C4D8` is also resolved as a **four-phase gameplay substep counter**. Its update site `0x0040C001..0x0040C01D` implements `0→1→2→3→0`, once near the start of active gameplay. The already-advanced phase is then consumed by later systems; phase value `2` gates selected slower animation/effect/random-transition work including trajectory AUX animation and live breakaway eligibility. This protocol is internal to logical gameplay updates and is independent of the unresolved wall-clock interpretation of the Win32 15,000-QPC-count limiter. See [`reverse/GAMEPLAY_UPDATE_PHASE.md`](reverse/GAMEPLAY_UPDATE_PHASE.md).

This establishes the FLY family as literal screen-space flight paths carrying per-point sprite-animation control, with a separate group lifecycle controlling activation, looping, retirement, path acquisition, and breakaway. See [`reverse/TRAJECTORY_GROUPS.md`](reverse/TRAJECTORY_GROUPS.md).


### Win32 game-state protocol resolved

`game_state_raw` at `0x0042B188` is now mapped for the complete user-facing value set tracked by `Q-STATE-001`. The top-level dispatcher directly handles only `0..5`, while subordinate menu/modal/gameplay paths consume larger sentinels:

| raw | established role |
|---:|---|
| `0` | exit/shutdown transition |
| `1` | main-menu entry after full session reset |
| `2` | active gameplay |
| `3` | instructions modal |
| `4` | main-menu re-entry without the state-1 reset |
| `5` | pause overlay |
| `6` | quit-game confirmation |
| `7` | ordering-information modal |
| `8` | high-score modal |
| `13` | demo-launch sentinel |
| `99` | nine-lives/high-score-disqualification notice |

The main menu independently proves the transitions `START GAME→2`, `INSTRUCTIONS→3`, `ORDERING INFORMATION→7`, `HIGH SCORES→8`, `PLAY DEMO→13`, and `EXIT DRONE→0`; joystick configuration is handled synchronously without a replacement state assignment. Both state-1/state-4 menu-entry paths consume 13 by enabling demo playback, resetting the replay index, and entering state 2. Clean protocol helpers and tests now live in `gameplay/game_state.*`; see [`../reverse/windows/state_machine.md`](../reverse/windows/state_machine.md).

### Gameplay-orchestrator call inventory

The Win32 state-2 region `0x0040BAB9..0x00411D86` is now tracked with a reproducible static direct-call inventory in `reverse/windows/gameplay_call_inventory.csv`. The current canonical binary contains **77 distinct direct-call targets** in that region. The densest static call sites include:

| target | provisional role | call instructions in state-2 region |
|---|---|---:|
| `0x00421ED0` | CRT `rand` | 56 |
| `0x00401660` | clipped transparent sprite blitter | 49 |
| `0x00420020` | 20-voice SFX pool | 25 |
| `0x00401470` | bitmap text renderer | 19 |
| `0x00406730` | DirectSound play slot | 16 |
| `0x004067D0` | DirectSound stop/reset | 12 |

These are **static call-instruction counts, not runtime frequencies**. The result supports treating the giant state-2 body as orchestration across reusable helpers rather than translating it wholesale. `scripts/analyze_direct_calls.py` regenerates the inventory from the canonical PE using `objdump`.

### Collision/effect and update→render partition checkpoint

The late ordinary state-2 path is no longer a single unresolved collision/render blob. `0x00402FC0` is an asymmetric centered-hitbox-vs-full-sprite broad phase; the `0x00472B00` `0x18`-record debris groups and secondary debris bank now have clean deterministic motion/lifetime rules; and three 15-entry common-entity debris pools are identified as `junk1.jba`, `junk2.jba`, and `wheel.jba` with their shared `0x00403330` motion/frame updater. This resolves `Q-FX-001`.

At call site `0x004100D8`, `0x004033D0` establishes a sharp simulation-to-presentation boundary by copying a 320×200 view from the three-screen 320×600 scenery buffer, wrapping at world row 600. The following region performs sprite/effect rendering, including direct particle plots, Drone detonation radial/noise rendering, and the newly classified scaled transparent path `0x00403460 -> 0x00413940`. A clean `gameplay_update_order` contract records these domains without translating the original giant state-2 routine wholesale.

### Native fidelity framebuffer/host

The clean core now owns a platform-independent 320×200 indexed framebuffer with 256-entry RGB palette and an RGBA conversion path. `drone_fidelity_host` presents a decoded original full-screen JBA using native platform APIs:

- Linux: X11;
- Windows: Win32/GDI;
- macOS: Cocoa/CoreGraphics.

The host deliberately has no game simulation yet. Its purpose is to establish a narrow native presentation boundary while preserving the original indexed framebuffer contract. iPadOS remains a later platform host using the same `drone_core` framebuffer/simulation contracts rather than a separate game implementation.

### Shield, Tab sync and special terminal-state checkpoint

The Space-key shield is now a tested clean gameplay slice. `0x0047FCB0` is a 32-bit energy accumulator whose high 16 bits are displayed shield units; reset is `75<<16`, per-update recharge is `+0x514` while the high word is below 75, and active drain is `0xBB80`. Protection flag `0x0046198C` gates both rendering and collision protection. The clean module deliberately preserves the original fractional overshoot quirk caused by the high-word-only recharge test. See `docs/reverse/SHIELD.md`.

The same pass proves the Win32 Tab option: `VK_TAB` negates `0x0042B1B4` between `+1/-1`, UI feedback says `VERTICAL RETRACE` / `SYNC ON/OFF`, and `+1` enables the 15,000-QPC-count wait. This resolves option ownership without inventing a fixed FPS.

Probe/Stinger states 4 and 10 are also promoted: state 4 is a visible `hole.jba` interaction state; state 10 is a one-update impact-consumed terminal state reset to inactive by the common dispatcher.

## Mothership checkpoint

The registered-game Mothership sequence is now present as executable-backed archaeology despite the shareware evidence boundary. Win32 `0x00413290` loads the `hull*`, `panel*`, `damage*`, `hub`, `motor`, and `hole` asset cluster after the six-Drone transition. The state-4 Stinger path reaches core target `0x00472598`; its `+0x142` activity byte at `0x004726DA` enters state 2 for the destruction sequence. Counter `0x004725CA` reaches an initialized threshold of 15, awards +500 to score/life-progress, plays `thunder2.wav`, and sets `mothership_destroyed=1`, which is persisted in high-score records. See `docs/reverse/MOTHERSHIP.md`.

## FONT2 0x14-byte structure resolution

The former critical `Q-ENTITY-001` is resolved as a Phase 1 classification artifact rather than simulation debt. Win32 `0x00401470` builds 64 × `0x14` FONT2 glyph descriptors at `0x00466C90`; DOS `0x000809B0` independently builds the same descriptor/cache at data offset `0x6F80`. Both use 16×4 cells, 7×5 masks, `character-0x20` indexing, and a pointer at `+0x10` to a `width*height+1` allocation. Clean `fidelity/font2.*` code models the safe semantic layout and extraction contract. See `docs/reverse/BITMAP_FONT.md`.

## Common entity-layout checkpoint

`Q-ENTITY-002` is resolved for simulation architecture. Win32 `0x154` and DOS `0x14F` common sprite/entities now have an established field-level lifecycle correspondence across constructor, transparent blitter, frame release, collision, destruction and score handling. The common combat bytes are `+0x30` accumulated damage and `+0x31` destruction threshold; Win32 common destructibles use `+0x14F` destruction-burst count and signed `+0x150` score value, corresponding to DOS `+0x14D/+0x14E`. Both records preserve an unreferenced 128-byte block between their 32-frame pointer table and current-frame byte.

The clean engine intentionally does not copy either packed structure. Contextual offsets such as trajectory breakaway accumulators, boss destruction counter `+0x34`, and family flag Win32 `+0x14E` / DOS `+0x14C` stay in subsystem-specific semantic state. The 17 fixed trajectory templates now also carry their exact recovered damage threshold, destruction-burst count and score value. See `docs/reverse/ENTITY_LAYOUT.md`.

## Important unresolved work inside Phase 2

- preserve the established ~70.0863 Hz DOS fidelity scheduler while treating the Win32 QPC wall-clock cadence as a separate nonblocking historical-port question;
- continue partitioning the state-2 Win32 gameplay orchestrator into named subsystems and whole-frame sequencing;
- keep family-specific overlays of the now-established Win32 `0x154` / DOS `0x14F` entity correspondence inside their owning gameplay subsystems rather than introducing a raw packed core ABI;
- finish dynamic special-family trajectory substitutions/producers and their whole-frame integration beyond the now-resolved 17-template startup catalog, group lifecycle, normal AUX/path cycle, and harmless `Rightdiv.fly` / `Swarm.fly` short-loader-slot quirks;
- integrate the now-resolved semantic `GameplayInputFrame` boundary into increasingly complete whole-frame simulation; physical DOS/DirectInput details remain host archaeology rather than core ABI;
- continue outward from the reconstructed player/projectile/lifecycle slices into enemy, encounter-transition, registered-level, and results behavior; the shareware scenery scroll cadence itself is now established;
- use the now-decoded demo channels as deterministic checkpoints while continuing whole-frame simulation reconstruction.


## Boss-family and world-scenery checkpoint

The first pre-Drone boss family is now coherent enough to separate from the registered Mothership. Win32 `0x00417350` loads `lid.jba` (nine 36×40 frames), `top.jba` (one 68×56 frame), `retro1.wav`, and `level1.wav`; `0x00417450` releases the same resources. `0x00417220` initializes the encounter and `0x00416700` performs active gameplay updates including movement, bomb spawning, rapid-missile/special-weapon collision, and destruction effects. When the lid common entity enters activity state 2, its `+0x34` counter reaches 25 and the game awards exactly +100 score/progress before moving the top/root entity into destruction state 2. This independently matches the README's documented **Destroy Boss = 100** rule.

The boss initializer is selected by a six-entry table indexed by the current processed-Drone count when the canonical `drone.jba` objective entity reaches Y=`-200` (`0x00446084`, common entity `+0x04`). The full dispatch is now established as Lid/Top, Gemini, registered-slot-2 unknown, Spidey, Lid/Top reused, and Bomber for processed Drone counts 0..5. The canonical shareware campaign normally reaches only Lid/Top and Gemini before its explicit two-level stop.

The world/scenery path is now also concrete. `0x004D9598` owns three contiguous 64,000-byte indexed screens. Normal gameplay initializes scroll row `0x004D9590` to 599 and decrements/wraps it only on shared gameplay phase 2; the state-7 Ordering Information modal is the only non-gameplay reuse and has an independent three-step cadence plus a 145-row background compositor. This scroll is independent of the Drone objective Y used for encounter progression. Normal session initialization loads river top/mid/bottom; post-Drone progress branches load desert, isle, house, night, or river families. The branch after processed outcome 2 is a dedicated shareware stop: it loads `deserbot.jba`, forces lives to zero, and returns toward post-game flow instead of loading a third scenery family. This directly matches the supplied README's two-level shareware limit while leaving registered scenery strings/code compiled into the executable. See `docs/reverse/LID_TOP_BOSS.md` and `docs/reverse/WORLD_SCENERY.md`.

## Validation gates for Phase 2

A finding is not promoted because it “looks right.” Phase 2 uses:

1. static binary access-pattern evidence;
2. DOS↔Windows cross-build comparison where available;
3. exact corpus structure/hashes;
4. clean-code tests;
5. runtime or framebuffer/state comparison when the fidelity host gains simulation.

The phase remains open until a meaningful gameplay subsystem is running in clean code with reference validation. This checkpoint is intentionally repo-ready even though the phase itself is not complete.

## Player reconstruction checkpoint

The Win32 player is now identified as the common `0x154` entity rooted at `0x00466B18`. `ship.jba` supplies 15 independently reproducible 22×22 guttered frames. Executable input proves exact X/Y bounds and a previously undocumented `A`/`Z` vertical movement path in addition to Left/Right. The 15-frame banking/recenter behavior and recovered directional ordering now have a narrow clean implementation in `drone_core`; see `docs/reverse/PLAYER.md`.

## Rapid-fire missile checkpoint

The normal Ctrl-fire weapon is now mapped end-to-end at the core-lifecycle level. Eight 1×9 entities at `0x0042F200` share three frames from `missile.jba`; `0x0042B198` is capacity 8, `0x00440274` is the active count, and `0x004406F4` is an update-count cooldown saturating at 8. Spawn is player+(11,-3), update is Y-=3, animation wraps three frames, +0x143 is marked at y<0, and the later cleanup threshold is y<-7. The SFX pool at `0x00446028` loads `missile.wav`. Clean code and regression tests are in `gameplay/rapid_missile.*`; see `docs/reverse/RAPID_MISSILES.md`.

`drone_gameplay_probe` now exercises the reconstructed chain against user-supplied reference assets and produces a deterministic PPM snapshot without placing proprietary pixels in the repository.


## Probe/Stinger checkpoint

The special projectile at `0x0045A148` is a common 3×8 entity whose frame 0 is the blue Probe and frame 1 is the red Stinger. Clean code now reproduces the established load/cycle/launch/homing/Probe-attachment behavior. State 4 is now classified as the visible `hole.jba` interaction state, while state 10 is a one-update impact-consumed terminal state reset to inactive by the common dispatcher. The downstream state-4 collision object at `0x00472598` is now established as the Mothership core/target entity; its complete composite encounter behavior remains partial. See `docs/reverse/SPECIAL_WEAPONS.md`.

## Demo replay checkpoint

The canonical 14-channel replay format is now semantically mapped. `0x00440594` is demo playback mode, `0x0047EBD4` is the replay frame index, and `0x00466B00` is the recording-enabled gate used by live channel writers and the terminal save path. Channels 1–6 are selected controls; 7–9 are trajectory-group script data; 10–12 recreate `bomb.jba` projectile spawns; 13–14 restore Drone X/Y.

The four demo files shared by the DOS and Windows evidence sets are byte-for-byte identical, making them cross-build behavioral fixtures. Clean typed decoding and a replay-to-gameplay adapter now live in `formats/demo.*` and `gameplay/demo_replay.*`; metadata-only hashes/statistics are in `manifests/demo_replays.csv`. See `docs/reverse/DEMO_REPLAY.md`.

`bomb.jba` is also now reproducibly extracted as three 1×9 frames, raising the metadata-only recovered sprite-frame set to 49.

## Enemy bomb gameplay checkpoint

The `0x004651A0` pool is now reconstructed beyond its demo checkpoint. Canonical capacity is ten; live spawn assigns `rand()%3` to common field `+0x10`, the active update advances Y by two and uses that field as horizontal steering magnitude, the three-frame animation wraps on the shared animation tick, and logical visibility (`X 0..319`, `Y 0..190`) is distinct from final retirement (`Y > 198`). Demo playback intentionally forces the steering field to zero after restoring X/Y, so replay bombs fall vertically. Clean spawn/update/lifetime code and synthetic tests are in `gameplay/enemy_bomb.*`; see `docs/reverse/ENEMY_BOMBS.md`.


## Player lifecycle and game-over checkpoint

`0x0042B1AC` is now established as the player-life counter: normal initialization writes three, the hidden L-key path writes nine, the HUD reads it, and state-2 uses it to gate post-game processing. Crucially, the player-destruction effect does not consume a life. State-2 waits for a later settlement gate, then decrements lives, resets shield/frame/position, and either respawns at `(147,175)` or invokes `0x0041E420`.

`0x0041E420` is a dedicated `gameover.jba` slide: the 117x20 banner starts at X=325/Y=90 and uses recovered 16.16-style horizontal position with initial velocity 270000 and decrement 2500. It presents 108 vertical-blank-paced iterations and lands at X=100. The fixed-point transition now exists as tested clean code; presentation remains outside gameplay core. See `docs/reverse/PLAYER_LIFECYCLE.md`.

The same lifecycle pass resolves `0x00438C14` as the shared enemy-bomb spawn gate. It saturates at 5, live bomb spawning requires 5 and resets it to 0, session init starts it at -450, and player destruction pushes it to -540. Respawn requires it to rise above -356, so the original reuses bomb suppression as a post-death quiet-period gate. Clean tests preserve the 185-update transition from -540 to first eligible value -355 without asserting a wall-clock duration.


## Objective-to-objective mission progression checkpoint

The objective transition is now connected across state-2 and `0x0041D690`: normal disarm commits outcome 1 at Drone Y=201, moves the objective to Y=202, resets its settlement tick at Y=230, and invokes the transition after Y>230 when the phase-2-only settlement scalar reaches 60. The transition builds a single composite mission interstitial: a transparent 280x37 goodN/badN banner is placed at (17,27) over mission1..5/miss6yes/miss6no, the saved 160x100 surveillance image is copied to (14,81), dynamic encounter statistics are drawn at their recovered coordinates, and confirmation is locked for 58 presentations before input is accepted. It then performs an encounter-only `initialize_gameplay_session(0)` reset that preserves campaign score/lives/outcomes.

The compiled six-way transition table now establishes the canonical shareware stop after processed objective 2 and the dormant registered/endgame condition: after objective 6, **zero detonations** reload river scenery and stage the Mothership; any detonation forces results. Clean `mission_progression` code and tests encode these contracts without proprietary art/audio. Retail reachability still requires lawful full-game evidence. See `docs/reverse/DRONE_OUTCOMES.md`, `WORLD_SCENERY.md`, and `MOTHERSHIP.md`.

## Six-Drone outcome/results checkpoint

The mission outcome ledger is now semantically recovered: byte array `0x00472590` stores per-Drone values `0=unresolved`, `1=disarmed`, `2=detonated`, while `0x00433B54` advances through the six objectives. The post-game path counts disarmed/detonated entries, uses the disarmed count to select `disarm0.jba`..`disarm6.jba`, and selects `suspense.wav`, `choral.wav`, `moon.wav`, or `hiphop.wav` with a recovered branch order. The surrounding `0x004115BE` control flow is also partitioned: lives<=0 entry, six numeric statistics, 58-present confirmation lock, optional Ordering Information modal, exact high-score insertion call mode, slot-0 no-save quirk, state1/state4 return behavior, and perfect-run credits are modeled by clean `post_game` code/tests. See `docs/reverse/DRONE_OUTCOMES.md` and `docs/reverse/POST_GAME_FLOW.md`.


## Scoring and high-score checkpoint

Phase 2 now has an evidence-backed scoring model rather than only the README table. `0x004D95F4` is total score and `0x004D95F8` is a separate rolling extra-life progress bucket. Normal awards and ordinary penalties apply the same signed delta to both. State 2 grants one life when progress reaches 500 and subtracts exactly 500, with at most one conversion per update. Drone detonation instead subtracts 1000 score with a floor at zero and clears progress. The HUD floors negative values and preserves the original one-step `score -= 9999` behavior. Clean code and edge-case tests live in `gameplay/scoring.*`; see `docs/reverse/SCORING.md`.

The Win32 high-score path is also structurally recovered. `0x004461D8` is the high-score disqualification flag: the L-key cheat sets lives to 9, sets this flag, enters state 99, and renders the literal nine-lives/disqualification messages. Post-results qualification is skipped for demos or disqualified sessions. The table contains ten entries ordered lowest-to-highest; qualification uses strict `new_score > existing_score`, ties remain behind existing equal entries, and insertion discards the lowest entry while shifting parallel arrays. Post-results calls `run_high_score_table(1,index,disarmed_count)`; index 0 uniquely skips name editing/persistence and leaves `ENTER YOUR NAME`, while indices above zero enter the 25-character editor and save. See `docs/reverse/HIGH_SCORES.md` and `docs/reverse/POST_GAME_FLOW.md`.

The runtime `scores` persistence format has been recovered from both Win32 reader and writer. Each name or integer carries a three-digit padding count and hundreds of random filler bytes between meaningful characters/digits. A bounds-checked clean decoder and deterministic structurally compatible encoder now round-trip the logical ten-entry table; exact original CRT random filler is intentionally not reproduced. See `docs/formats/SCORES.md`.


## Boss-dispatch completion checkpoint

The pre-Drone boss dispatcher is now classified at the family level. Gemini (`0x00405000` update / `0x00405EF0` init / `0x00405FB0` load / `0x00406190` release) is the second normally reachable shareware boss and reconstructs 30 shared 56x41 body frames plus a 43x34 head from resources present in the evidence set. Spidey and Bomber have complete loader/init/update/release ownership plus exact +100 destruction checkpoints, but their registered resources are absent from the supplied shareware install.

Dispatch slot 2 is intentionally left without a proper asset name: its update (`0x00406CC0`), initializer (`0x00407980`) and guarded release (`0x00407AB0`) survive, including direct destructible indexed-sprite pixel mutation and a +100 destruction path, but no dedicated load path/resource identity is established in the canonical shareware PE. This is recorded as an evidence-set limitation rather than filled with a guessed name.

A prior provisional address was corrected during this checkpoint: Bomber active update begins at `0x00403650`, directly called by state 2; `0x00403600` is a preceding helper ending at `0x00403649`. Likewise, Phase-1's generic `audio_shutdown_or_reset` label at `0x00406190` is superseded by the now-proven `release_gemini_boss_assets` ownership. See [`reverse/BOSS_PROGRESSION.md`](reverse/BOSS_PROGRESSION.md) and the family-specific boss documents.

The replay timeline is now cross-build established as part of the active gameplay update boundary. Win32 `0x0047EBD4` and DOS data offset `0x4CE8C` both reset to zero, pre-increment once per gameplay update when recording/playback is active, and use terminal threshold `0x82F` (2095). The physical DAT corpus contains 2101 records, so the clean `DemoReplayTimeline` preserves runtime cutoff separately from file length and deliberately does not claim a fixed simulation Hz.
