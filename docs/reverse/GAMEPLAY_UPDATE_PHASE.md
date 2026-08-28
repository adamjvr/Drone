# Win32 Gameplay Substep Phase and Frame-Order Landmarks

**Status:** four-phase state-2 substep protocol established; complete whole-frame subsystem ordering remains in progress.

The large Win32 active-gameplay body begins at `0x0040BAB9`. A shared 32-bit value at `0x0053C4D8` is read throughout that body and passed into several recovered subsystem updaters. Its update site at `0x0040C001..0x0040C01D` establishes its basic contract exactly.

## Four-phase protocol

Near the start of every state-2 update the original performs the equivalent of:

```text
old = gameplay_substep_phase
next = old + 1
if old >= 3:
    next = 0
gameplay_substep_phase = next
```

On its canonical runtime domain this is:

```text
0 -> 1 -> 2 -> 3 -> 0 -> ...
```

This is a **four-phase** protocol. Earlier notes that described `0x0053C4D8` generically as an “animation phase” or could be read as implying three phases are superseded by this direct update-site proof.

The clean helper `advance_win32_gameplay_substep_phase()` reproduces the signed comparison and 32-bit increment behavior without tying the portable simulation to the original global-variable architecture.

## Established phase-2 consumers

Phase value `2` is repeatedly used as a slower-work gate. Established examples include:

- normal trajectory FLY AUX animation in `update_trajectory_groups` (`0x00415FA0`);
- eligibility for live trajectory-group conversion to breakaway mode 10;
- scenery `world_scroll_row` decrement/wrap during active gameplay;
- shield-loop sound triggering;
- several boss/destruction counters and animation/effect updates already documented in their subsystem notes;
- portions of results/progression and effect timing inside the state-2 orchestrator.

This does **not** mean every gameplay subsystem runs only on phase 2. Position/path progression can occur every state-2 update while selected animation, random transition, sound, or destruction work is phase-gated.

## Partitioned ordinary frame order

The ordinary state-2 body is now partitioned far enough to establish an explicit scheduler contract. Conditional death/modal/results branches can bypass portions of it, but the normal path has these stable landmarks:

1. state-2 entry / one-shot transition handling begins at `0x0040BAB9`;
2. the four-phase substep advances at `0x0040C001`;
3. formation/object creation and source selection occur before trajectory work;
4. `update_trajectory_groups` is called at `0x0040DDB5`;
5. `update_drone_detonation_effect` is called at `0x0040DEE4`;
6. boss dispatch/active updates occupy `0x0040E817..0x0040E8D8`;
7. the following `0x0040E8E0..0x004100D7` region is simulation-side collision/destruction/effect work, including `entity_hitbox_overlaps_sprite_rect` (`0x00402FC0`), `update_debris_particle_groups` (`0x00402B40`, call `0x0040F06A`) and `update_junk_wheel_debris_sprites` (`0x004032C0`, call `0x0040FEC9`);
8. **`0x004100D8` is the sharp update→presentation boundary**: it calls `compose_scrolling_world_viewport` (`0x004033D0`) to seed the software framebuffer from the 320×600 scenery stack;
9. ordinary transparent sprite rendering (`0x00401660`) follows;
10. `render_debris_particle_groups` (`0x00402CA0`, call `0x004108BD`) writes particle visual codes directly to the framebuffer;
11. Drone destruction presentation uses `render_drone_detonation_radial_noise` (`0x0041EBE0`, calls `0x004109AA/0x004109C9`) while the detonation is active;
12. selected entities use scaled transparent rendering through `0x00403460 -> 0x00413940`;
13. HUD/shield rendering is late (`render_shield_meter` at `0x004111AC`, player shield effect at `0x004111DE`);
14. the optional 15,000-QPC-count guard executes at `0x0041144D..0x00411496`;
15. the software framebuffer is presented at `0x004115A5` on the ordinary path.

This is now encoded as `canonical_win32_gameplay_stage_order()` in clean code. The contract separates **Simulation**, **FidelityPresentation**, and **Host** domains so later platform/remaster code can consume the recovered ordering without importing the original monolithic control-flow shape.

## Why this matters for the clean engine

The portable simulation should not translate `0x0040BAB9..0x00411D86` wholesale. The evidence supports a scheduler/orchestrator whose stable contract is reconstructed incrementally:

- advance canonical logical time / substep phase;
- run creation and entity-update systems in evidence-backed order;
- run collisions/effects;
- render the indexed frame/HUD;
- present through the platform host.

The DOS fidelity scheduler remains a separate contract at approximately 70.0863 logical updates per second. The Win32 four-phase value describes substep selection **inside gameplay updates**; it does not establish a Win32 wall-clock frame rate, and the QPC limiter still cannot be converted to Hz from the executable alone because no counter frequency is queried.

## Clean implementation

- `include/drone/gameplay/gameplay_phase.hpp`
- `src/gameplay/gameplay_phase.cpp`
- `include/drone/gameplay/world_scroll.hpp`
- `src/gameplay/world_scroll.cpp`
- `tests/test_gameplay.cpp`

The collision/effect-to-render boundary and its major debris/scenery/scaled-render helpers are now classified and represented by an explicit portable stage-order contract. The scenery-row producer is also closed: gameplay advances it only on phase 2, while the Ordering Information modal has an independent three-step reuse. The former `0x00446084` “world coordinate” is now correctly classified as the canonical Drone objective entity's Y field. Remaining Phase-2 sequencing work should concentrate on branch-aware encounter/result orchestration and unresolved input/entity semantics rather than re-opening the scrolling contract.
