# Original Behavior Specification — Shareware Documentation Baseline

## Evidence and scope

This document paraphrases behavioral statements from the **supplied DOS DRONE Shareware v1.01 README** and turns them into black-box validation requirements. Documentation claims are valuable evidence, but executable behavior remains authoritative when edge cases differ.

The supplied shareware documentation states that the shareware build contains **two levels** and the registered game contains **seven**.

This file deliberately separates documented rules from details that still require binary/runtime confirmation.

## Game objective

Drone is a top-view vertically scrolling shooter. The player's mission is not simply to destroy every target: six alien nuclear missiles (“Drones”) must be disarmed with blue probes while enemy escorts and bosses are fought with conventional rapid-fire missiles and red stingers.

The registered-game description says successfully disarming all six Drones leads to an alien mothership encounter. That later content is outside the supplied shareware payload and is not yet a parity target.

## Player movement and primary fire

Documented keyboard behavior:

| action | default key |
|---|---|
| move left | Left Arrow |
| move right | Right Arrow |
| rapid fire | either Ctrl key |
| toggle/load probe vs stinger | Down Arrow |
| launch selected probe/stinger | Up Arrow |
| shield | Space |
| pause | P |
| resume | R |
| end current game | Esc |

Player movement is described as left/right only, with the player ship positioned near the bottom of the vertically scrolling playfield.

## Special weapons and targeting

### Red stinger

Documented behavior:

- stingers home toward enemy ships;
- the targeting system attempts to lock onto the nearest/most threatening enemy when stingers are selected;
- their explosion is larger/more powerful than normal rapid fire and can affect multiple enemies.

Exact target-selection ranking, homing acceleration/turning, blast radius, damage, and collision order remain executable-recovery tasks.

### Blue probe

Documented behavior:

- probes are used to disarm Drones;
- when probes are selected, targeting attempts to lock onto the Drone even before the Drone is visible;
- a launched probe homes toward the Drone;
- after attachment, decoding/disarming takes time and depends on selected skill level;
- enemy ships can shoot an attached probe off, requiring another probe;
- the targeting/communication system handles only one active probe-or-stinger interaction at a time, so an attached probe prevents simultaneous stinger use.

Exact attachment conditions, decode/disarm timers, difficulty scaling, probe vulnerability, and special-weapon state machine remain to be recovered.

## Drone interaction

The documentation explicitly warns that the player's objective is to **disarm**, not destroy/detonate, the Drones. Firing conventional weapons carelessly as a Drone approaches can detonate it.

A warning beep is documented when a Drone is approaching.

## HUD

Documented HUD behaviors:

- six Drone icons are shown;
- the current Drone is boxed/outlined;
- blue icons indicate disarmed Drones;
- red icons indicate detonated Drones;
- probe status is shown in the lower-left HUD area;
- score appears at lower right beneath remaining lives.

Exact pixel coordinates, text/font rendering, icon-state timing, and intermediate probe status values remain renderer/state reconstruction targets.

## Scoring

| event | score delta |
|---|---:|
| destroy small enemy | +1 |
| destroy medium enemy | +25 |
| destroy boss | +100 |
| attach probe | +10 |
| disarm Drone | +500 |
| small enemy escapes | -1 |
| medium enemy escapes | -25 |
| Drone detonates | -1000 |

The documentation says the player receives an extra life for every **500 points earned**. Binary recovery must establish edge semantics such as negative-score interaction, multiple-threshold crossings, score floor/overflow, and whether “earned” means current score or a separate accumulated-positive-points counter.

## Boss ordering

Boss ships are documented as appearing immediately before Drones. Exact boss triggers, health/damage model, movement, and relationship to level sequencing remain open.

## Difficulty/skill

The documentation states that probe decoding/disarming duration depends on selected skill level. It does not provide the numerical scaling in the behavior section. Difficulty selection/state and every gameplay quantity affected by it must be recovered from the executable.

## Joystick behavior

The DOS documentation describes configurable standard joystick support. Configuration asks the user to assign/calibrate at least:

- rapid fire;
- probe/stinger load-toggle;
- probe/stinger launch;
- shield;
- joystick center/calibration.

It also describes top-hat use as useful for simultaneous firing plus special-weapon control on hardware where ordinary buttons cannot be registered concurrently.

The clean engine should normalize all physical input devices to canonical actions rather than reproduce old game-port hardware limitations, unless an optional historical-input mode is ever desired.

## Cheats/debug-like controls documented to players

### `L` — nine lives

The README states that `L` grants nine lives and disqualifies the run from the high-score list. This gives us at least two testable behaviors:

1. lives becomes/grants the documented nine-life cheat state;
2. a high-score eligibility flag changes.

The exact interpretation of “grants nine lives” (set-to-nine versus add-nine at all states) should be confirmed in code/runtime before implementation.

### `Tab` — vertical retrace sync toggle

The README states that `Tab` toggles “vertical retrace sync”; turning it off makes the game run much faster and is presented as a novelty/cheat-like behavior.

This is highly relevant to timing reconstruction. The Win32 executable has a mechanically confirmed pacing guard at `0x0042B1B4`, but **the current project has not yet proven that this global is the value toggled by Tab**. Mapping that key path is a Phase 2 timing/input objective.

## Performance-era behavior

The DOS README notes materially different performance under pure DOS versus running in a Windows DOS shell on period hardware, including possible slow graphics or audio hiccups. This is historical context, not a target requirement for the remaster. The compatibility target is intended game behavior, not reproducing host DMA starvation.

## Black-box test IDs to create as subsystems are recovered

| ID | behavior target | current status |
|---|---|---|
| `BHV-CTRL-001` | left/right movement only | documented; executable mapping pending |
| `BHV-WPN-001` | Ctrl/Fire launches normal rapid-fire weapon | documented; executable mapping pending |
| `BHV-WPN-002` | Down toggles special type; Up launches | documented; executable mapping pending |
| `BHV-PROBE-001` | probe targets Drone and can attach/disarm | documented; exact state machine pending |
| `BHV-STING-001` | stinger targets hostile and homes | documented; exact targeting/homing pending |
| `BHV-TARGET-001` | only one probe/stinger targeting communication active | documented; implementation pending |
| `BHV-HUD-001` | six Drone indicators with current/disarmed/detonated states | documented; implementation pending |
| `BHV-SCORE-001` | scoring table deltas | documented; executable edge cases pending |
| `BHV-LIFE-001` | extra life every 500 points earned | documented; threshold semantics pending |
| `BHV-CHEAT-001` | L grants nine lives and invalidates high score | documented; exact assignment pending |
| `BHV-TIME-001` | Tab toggles retrace/pacing and off runs faster | documented; Win32/DOS variable mapping pending |

As each rule is recovered, this table should point to trace/tests rather than remaining a documentation-only checklist.
