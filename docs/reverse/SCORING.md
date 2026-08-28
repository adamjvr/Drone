# Scoring and Extra-Life Reconstruction

## Scope

This document records the established Win32 scoring behavior recovered during Phase 2. It distinguishes the player's displayed/total score from the separate rolling progress bucket used to grant extra lives. The clean implementation lives in `include/drone/gameplay/scoring.hpp` and `src/gameplay/scoring.cpp`.

## Canonical globals

| address | established role | representation |
|---|---|---|
| `0x004D95F4` | total score | signed 32-bit scalar |
| `0x004D95F8` | extra-life progress | signed 32-bit scalar |
| `0x0042B1AC` | player lives | signed 32-bit scalar |

The total score is also consumed by the post-results high-score qualifier, independently corroborating its identity.

## Normal score mutation

Normal score-producing gameplay paths add the same signed delta to both score variables:

```text
total_score         += delta
extra_life_progress += delta
```

This applies to positive awards and ordinary escape penalties. Consequently, `extra_life_progress` is **not** a monotonic count of all points ever earned. A normal negative score event can undo progress toward the next life.

Known documented events include small/medium/boss kills, probe attachment, Drone disarm, and escape penalties. Event-specific producer recovery continues, but the shared accumulator behavior is established.

## Extra-life conversion

The state-2 gameplay path tests the rolling accumulator against **500 points**. When the threshold is reached it grants exactly one life and subtracts exactly 500:

```text
if extra_life_progress >= 500:
    player_lives += 1
    extra_life_progress -= 500
```

The original performs at most one conversion per state-2 update. It is therefore intentionally different from a `while (progress >= 500)` normalization.

Examples:

| before update | result |
|---|---|
| `progress=505` | one life, remainder `5` |
| `progress=1000` | one life, remainder `500`; another life can be granted on a later update |

The clean `consume_one_extra_life_threshold()` helper preserves this one-threshold-per-update behavior.

## Drone detonation penalty

Drone detonation uses a dedicated path rather than the generic signed-delta helper:

```text
if total_score < 1000:
    total_score = 0
else:
    total_score -= 1000

extra_life_progress = 0
```

This matches the documented `-1000` Drone penalty while revealing an additional rule: a detonation wipes all current progress toward the next extra life.

## HUD normalization quirk

The HUD/update path normalizes both accumulators before display:

```text
if total_score < 0:
    total_score = 0

if extra_life_progress < 0:
    extra_life_progress = 0

if total_score >= 9999:
    total_score -= 9999
```

The final operation is intentionally **not** a clamp and not a conventional modulo. One HUD pass subtracts one 9999 block only.

Examples:

```text
10000 -> 1
19998 -> 9999 -> 0 across two HUD passes
```

This behavior is preserved in fidelity code because replacing it with `min()`, `% 9999`, or a wider modern score display would alter original simulation state.

## Clean implementation boundary

`drone::gameplay::ScoreState` contains only semantic state and deterministic operations. It does not know about:

- HUD pixels or fonts;
- high-score persistence;
- DirectDraw;
- sound effects;
- platform APIs.

High-score qualification/insertion is documented separately in [`HIGH_SCORES.md`](HIGH_SCORES.md). The legacy disk file is documented in [`../formats/SCORES.md`](../formats/SCORES.md).

## Validation

`tests/test_gameplay.cpp` currently covers:

- positive and negative generic deltas;
- 500-point conversion with preserved remainder;
- one-threshold-per-update behavior;
- Drone detonation score floor and progress reset;
- negative normalization;
- one-step 9999 subtraction.

## Remaining work

- Map all score-producing object families to their exact event values and addresses.
- Correlate the DOS implementation and determine whether all arithmetic quirks are shared.
- High-score persistence semantics are now complete: the additional fields are Mothership destroyed and Percentage hit. Remaining work is presentation-only: determine which original UI modes expose those persisted statistics.
