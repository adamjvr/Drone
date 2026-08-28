# Legacy `scores` File Format

## Status

**Win32 physical encoding: established.**  
**DOS physical-format correspondence: established.**

Both canonical shareware executables create/read a runtime file named `scores` using `rt`/`wt` stdio modes. Despite the mode names, the logical values are hidden among large runs of random printable filler bytes. The DOS Watcom implementation and later Win32 implementation independently reproduce the same constants and physical encoding algorithm, making this one of the project's strongest cross-build format correspondences.

The Win32 reader/writer anchors are:

| address | role |
|---|---|
| `0x004174D0` | load complete high-score file |
| `0x00417570` | decode padded name |
| `0x00417700` | decode padded decimal integer |
| `0x0041F120` | save complete high-score file |
| `0x0041F220` | encode padded name |
| `0x0041F390` | encode padded decimal integer |

The clean parser/compatible writer are implemented in `include/drone/formats/scores.hpp` and `src/formats/scores.cpp`.

### DOS correspondence anchors

| DOS address | role |
|---|---|
| `0x000894FC` | save complete high-score file |
| `0x000895CC` | encode padded name |
| `0x0008974C` | encode padded decimal integer |
| `0x000898C8` | load complete high-score file |
| `0x00089954` | decode padded name |
| `0x00089C60` | decode padded decimal integer |

The DOS save loop writes ten records, advances names by `0x1E` (30) bytes per entry, and serializes four numeric values after each name. Its name writer uses `rand()%10` for the six decimal noise digits, `rand()%400 + 300` for padding, and `rand()%26 + 'A'` for name filler. The numeric writer uses the same envelope and decimal-digit filler. The load side mirrors those rules.

This establishes physical-format compatibility at the algorithm level. Subsequent producer/display triangulation resolved all four numeric meanings in both builds: Drones disarmed, score, Mothership destroyed, and Percentage hit.

## Logical table

The file contains exactly **10 high-score entries**. Each logical entry is:

1. name;
2. Drones-disarmed count;
3. score;
4. Mothership-destroyed flag (stored as a signed 16-bit decimal integer; established producer writes 0/1);
5. Percentage hit (signed 16-bit integer percentage).

See [`../reverse/HIGH_SCORES.md`](../reverse/HIGH_SCORES.md) for table semantics.

## Shared value envelope

Each encoded value begins with:

```text
3 filler bytes
3 ASCII decimal digits = padding length
3 filler bytes
```

The original writer chooses:

```text
padding = rand() % 400 + 300
```

so canonical output uses a per-value filler width of **300..699 bytes**.

The initial three bytes and the second three bytes are noise. Their character class differs by writer path but they do not carry semantic payload.

## Encoded name

After the 9-byte envelope, the name writer emits every meaningful byte as:

```text
padding filler bytes
actual character
```

This repeats through the terminating NUL character.

Conceptually:

```text
[noise3][DDD][noise3]
[filler × D] 'A'
[filler × D] 'C'
[filler × D] 'E'
[filler × D] '\0'
```

where `DDD` is the three-digit padding length.

The original writer uses random uppercase `A..Z` filler for the padded character region.

The decoder skips the filler runs and preserves only the meaningful character following each run until NUL.

## Encoded integer

Numeric values use the same 9-byte envelope. It is followed by one ASCII decimal digit giving the number of decimal characters in the value. Then each decimal character is stored after one full padding run:

```text
[noise3][DDD][noise3][digit_count]
[filler × D] digit0
[filler × D] digit1
...
```

The original numeric writer converts a non-negative value to decimal text, emits `strlen(decimal_text)` as one ASCII digit, and uses random decimal-digit filler in the padded region.

Normal high-score values require only 1..4 decimal digits in the recovered paths.

The decoder reconstructs the integer by decimal accumulation.

## Why this is called obfuscation, not encryption

The physical representation does not provide cryptographic confidentiality or integrity. The meaningful bytes are simply separated by a padding count that is itself stored in clear decimal text near each value. The scheme appears designed to make casual hand-editing of the score file inconvenient.

The project therefore describes it as **padding/obfuscation**, not encryption.

## Clean decoder safety

The original C implementation trusts the file structure aggressively. The clean parser intentionally adds bounds and format checks so malformed or truncated files produce `ScoresFormatError` rather than memory corruption or uncontrolled reads.

That safety hardening does not change the interpretation of valid legacy files.

## Compatible deterministic encoder

`encode_legacy_scores()` writes a structurally compatible file but deliberately does **not** claim byte-for-byte reproduction of the original writer's CRT `rand()` sequence. It uses deterministic filler driven by an explicit seed so tests and tooling are reproducible.

Compatibility invariant:

```text
legacy logical table
    ↓ clean compatible encoder
padded scores bytes
    ↓ clean decoder
same logical table
```

This is appropriate for migration/import/export tooling. If exact original PRNG-byte reproduction later becomes useful for forensic comparison, it should be implemented as a separate reference writer rather than changing the deterministic clean API.

## Remaining correspondence work

The physical algorithm and all four logical field meanings are established in both builds. A future controlled execution test can strengthen this further by writing a non-empty score table in one original build and loading it in the other, but semantic cross-build parity no longer depends on that experiment.
