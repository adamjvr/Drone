# High-Score Reconstruction

## Scope

This document records the Phase 2 reconstruction of Drone's Win32 high-score table, qualification rules, nine-lives disqualification, insertion behavior, name entry, and persistence boundary.

The clean semantic implementation is in:

- `include/drone/high_score.hpp`
- `include/drone/gameplay/high_scores.hpp`
- `src/gameplay/high_scores.cpp`
- `include/drone/gameplay/post_game.hpp` / `src/gameplay/post_game.cpp` for the post-results orchestration boundary

The on-disk format is specified separately in [`../formats/SCORES.md`](../formats/SCORES.md).

## Session eligibility

Post-results high-score qualification is gated before the score table is scanned.

A session is ineligible when either:

1. demo playback mode (`0x00440594`) is nonzero; or
2. high-score disqualification (`0x004461D8`) is nonzero.

Normal gameplay initialization clears `0x004461D8`.

### Nine-lives cheat

The executable directly proves the README's cheat warning. The L-key path:

1. detects `L` (`VK_L`, `0x4C`);
2. writes `player_lives = 9`;
3. writes `high_score_disqualified = 1`;
4. enters the special message state (`game_state_raw = 99`);
5. renders the literal messages:
   - `<L> KEY GRANTED NINE LIVES!`
   - `DISQUALIFIED FROM HIGH SCORES`

After results, the qualifier refuses to scan the high-score table while this flag is set. The disqualification is therefore executable-established behavior, not merely documentation lore.

## Table representation

The Win32 build stores **10 entries** as parallel arrays rather than one packed C structure.

| address | representation | established meaning |
|---|---|---|
| `0x00434D68` | 10 × signed 16-bit | score |
| `0x00469EB8` | 10 × 30-byte slots | name storage |
| `0x00446F58` | 10 × signed 16-bit | Drones disarmed |
| `0x00440428` | 10 × signed 16-bit | Mothership destroyed (0/1 in established producer) |
| `0x00446DE8` | 10 × signed 16-bit | Percentage hit |

All four persisted numeric fields are now semantically established. `0x00440428` receives the low word of the live Mothership-destroyed flag (`0x00464B90`) during score insertion. `0x00446DE8` receives the integer accuracy percentage computed from alien ships hit and total alien ships; the original results artwork labels the value **Percentage Hit**.

### Ordering

Memory order is **lowest score at index 0, highest score at index 9**.

This is important because the original qualifier scans from low to high and retains the final qualifying index.

## Qualification rule

Qualification uses a strict comparison:

```text
new_score > existing_score
```

For every entry satisfying that predicate, the candidate insertion index is updated. The final match is used.

Consequences:

- a score equal to an existing entry does not displace that equal entry;
- a tie can still qualify below existing equal scores if it is greater than a lower entry;
- a score that is not greater than the lowest entry does not qualify.

The clean `find_high_score_insertion_index()` reproduces this exact ordering and gating behavior.

## Insertion

For insertion at index `i`, the original discards the lowest entry and shifts entries `1..i` one slot toward index 0:

```text
old[1] -> new[0]
old[2] -> new[1]
...
old[i] -> new[i-1]
new entry -> new[i]
```

Entries above `i` remain unchanged. All parallel statistic/name arrays are shifted together.

The clean `insert_high_score()` performs the equivalent semantic table operation.

## Name entry

The interactive name-entry path accepts:

- alphabetic characters;
- digits;
- space;
- backspace editing.

Lowercase letters are normalized to uppercase. Interactive entry is capped at **25 characters** (`0x19`) even though each persistent name slot reserves 30 bytes. The transient stack buffer has room for the terminating NUL.

The clean model exposes the 25-character limit as `high_score_interactive_name_max_chars`; platform/UI-specific text input remains outside `drone_core`.

## High-score UI/function anchor

`0x0041AFB0` is the central high-score display/insertion/name-entry routine. Its two established call modes are now exact:

```text
main-menu display:       run_high_score_table(0, 0, 0)
post-results insertion:  run_high_score_table(1, qualifying_index, disarmed_count)
```

The third argument is consumed by the completed-endgame path rather than being a generic display value. Table mutation, name-input semantics, and the post-results state handoff are established; pixel-level presentation details are not required by the clean simulation contract.

### Slot-zero fidelity quirk

If the qualifying insertion index is exactly 0, the Win32 routine directly writes the candidate into the lowest slot with the literal placeholder name `ENTER YOUR NAME`, skips the interactive name editor, and does **not** call `save_high_scores_file` on that path. Insertions above slot 0 perform the usual shift, interactive name edit, and file save. This original quirk is exposed by the clean post-game planner rather than silently repaired.

## Persistence

The runtime-generated file is named simply `scores`. It stores all ten entries with the logical record:

```text
name
Drones disarmed
score
Mothership destroyed
Percentage hit
```

The physical bytes are deliberately padded/obfuscated and are specified in [`../formats/SCORES.md`](../formats/SCORES.md).

## Validation

`tests/test_gameplay.cpp` covers:

- demo exclusion;
- nine-lives disqualification;
- strict qualification;
- tie placement;
- lowest-to-highest insertion;
- table shifting;
- compatible `scores` encode/decode round-trip;
- exact post-results invocation mode and insertion index;
- slot-zero placeholder/no-save behavior versus interactive persisted insertion.

## Remaining questions

1. Does any original build ever persist a Mothership-destroyed value other than 0 or 1? The established producer writes a boolean-style 0/1 flag, but the disk field is a signed decimal integer.
2. A runtime-created score file should eventually be exchanged between controlled DOS and Win32 executions to supplement the already-established algorithm-level cross-build correspondence.

The former question about the post-results call mode is closed; see [`POST_GAME_FLOW.md`](POST_GAME_FLOW.md).
