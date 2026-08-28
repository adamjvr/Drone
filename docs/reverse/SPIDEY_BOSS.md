# Spidey Boss

The Spidey family is selected by boss dispatch slot 3. The research name comes directly from its dedicated asset/audio namespace and is reinforced by a matching loader/releaser pair and composite combat state machine.

## Functions

| address | research name |
|---:|---|
| `0x00414D80` | `update_spidey_boss` |
| `0x00415AC0` | `initialize_spidey_boss` |
| `0x00415C80` | `load_spidey_boss_assets` |
| `0x00415F40` | `release_spidey_boss_assets` |

Resource load guard: `0x004D95C4`.

## Resource namespace

The dedicated loader references:

- `spidey1.jba`, `spidey2.jba`, `spidey3.jba`;
- `spidey01.jba`, `spidey02.jba`, `spidey03.jba`;
- `spideydo.jba`;
- `spidey.wav`;
- `level4.wav`.

These registered resource names survive in the shareware PE while the corresponding files are absent from the supplied shareware install. The loader nonetheless reveals frame extraction and entity-sharing behavior that can be reconstructed once a lawful full-game evidence set is available.

## Composite state

The central common entity is rooted at `0x004402D0`, with activity byte `0x00440412` and state-2 progression counter `0x00440304`. Additional component roots include `0x00440440`, `0x00464FF0`, and `0x00495B90` plus the loader's associated subcomponents.

## Destruction milestone

The recovered combat path can set the central entity to state 2 and reset the counter. State-2 updates increment it; when the value reaches exactly 45, the executable:

- adds 100 to total score;
- adds 100 to extra-life progress;
- clears the other component activity states;
- resets central motion/progression fields;
- emits the established explosion/debris sequence.

Again, 45 is an update/progression threshold, not a count of weapon impacts.
