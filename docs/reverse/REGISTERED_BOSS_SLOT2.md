# Registered Boss Slot 2 — Identity Unresolved

Boss dispatch slot 2 is a useful example of the project evidence rule: **known behavior does not imply a known asset identity**.

The canonical shareware PE retains a complete-looking combat/update routine for the encounter, but its dedicated resource-loading path is absent or stripped and the normal shareware campaign terminates before this slot is reached. The project therefore assigns a stable descriptive identity (`RegisteredSlot2Unknown`) without guessing a proper boss name from unrelated artwork.

## Known functions

| address | research name | status |
|---:|---|---|
| `0x00406CC0` | `update_registered_boss_slot2` | established combat ownership; visual identity unknown |
| `0x00407980` | `initialize_registered_boss_slot2` | selected by dispatch slot 2 |
| unknown/absent | dedicated resource loader | not present/identified in canonical shareware PE |
| `0x00407AB0` | `release_registered_boss_slot2_assets` | guarded release path survives |

Resource guard: `0x004D95C0`.

## Surviving object family

Controller/root common entity:

- root `0x0047EA80`;
- standard activity byte `0x0047EBC2` (`+0x142`);
- state-2 progression counter `0x0047EAB4` (`+0x34`).

Three associated sprite/component roots are released by `0x00407AB0`:

- `0x00441618`;
- `0x00480040`;
- `0x0043FFC8`.

The update positions these components relative to the controller, spawns enemy bombs, processes rapid-missile and special-weapon interactions, and performs composite destruction effects.

## Destructible-pixel behavior

One particularly strong surviving behavior is direct modification of the indexed sprite pixels owned by the `0x00441618` component. A missile impact computes an affected region and clears palette indices in the `0x20..0x2B` range to zero. This is genuine destructible sprite-pixel damage rather than merely swapping to a pre-damaged frame.

The clean remaster should preserve this behavior semantically once the missing source art is available; remaster rendering may represent it differently only outside fidelity mode.

## Destruction milestone

A special collision can place the controller into state 2 and reset `0x0047EAB4`. State 2 increments the counter. At exactly 45, the update routine:

- adds 100 to total score;
- adds 100 to extra-life progress;
- clears the three component activity states;
- resets controller motion/progression fields;
- starts explosion/debris effects.

Thus the encounter's core combat/destruction semantics survive even though the resource identity does not.

## Why the name remains unknown

The canonical shareware executable contains no observed write that raises `0x004D95C0` to indicate successful resource loading, and no JBA loader call has been tied to these component roots. `0x00407AB0` still contains the guarded cleanup and three audio-release operations, indicating that a fuller build likely supplied the missing loader/resources.

The normal shareware campaign exits after the second Drone result, before boss slot 2 is normally reached. Generic enemy sheets such as Hydra, Sloop, Bat, Saddle, Frisbee, Blade, Flake and Skate are not evidence for this boss and must not be used to invent an identity.

A lawful registered/full-game build is the preferred next evidence source for resolving this family.
