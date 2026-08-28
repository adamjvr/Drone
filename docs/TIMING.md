# Timing Research

Timing remains a high-risk reconstruction area. Phase 2 has now established the original display-synchronization primitives, the Win32 user-facing Tab toggle, the canonical DOS fidelity cadence, and the Win32 four-phase gameplay substep. The project still does not assign an unsupported fixed Win32 wall-clock simulation frequency.

## Canonical DOS display mode and nominal sync cadence

The DOS executable now proves its display mode directly. Startup at `0x0006E005` loads `EAX = 0x13` and calls `0x00067E50`. That helper clears `AH`, copies `AL` into a real-mode interrupt register frame, selects interrupt `0x10`, and invokes the DOS-extender interrupt bridge. This is BIOS `INT 10h, AH=00h, AL=13h`: standard VGA/MCGA **320x200x256 mode 13h**.

Standard VGA mode 13h has a nominal refresh of approximately **70.086 Hz** (about **14.268 ms per refresh**). That hardware fact can now be combined with executable evidence rather than inferred from resolution alone: the active DOS gameplay loop increments the shared replay/update index once, performs gameplay, and near its presentation tail conditionally calls the VGA retrace wait when sync mode is `+1`.

This gives the project a defensible **canonical DOS sync cadence reference: mode-13h vertical refresh (~70.086 Hz)**. It is the appropriate historical pacing target for DOS-fidelity validation. Two caveats remain:

1. `wait_vga_vertical_retrace()` waits until the retrace bit is set; it does not explicitly wait for clear-then-set, so pathological re-entry during the same retrace interval is an original implementation quirk.
2. The later Win32 port does not normalize its 15,000-count QPC threshold with `QueryPerformanceFrequency`, so its host-dependent pacing must not redefine the canonical DOS cadence.

The clean engine may therefore target the DOS mode-13h cadence for fidelity scheduling while still preserving legacy-platform timing differences as separate compatibility evidence.

## Confirmed DOS vertical-retrace primitive

Canonical DOS linear address `0x0006940C` performs a direct VGA vertical-retrace wait:

```asm
push edx
mov  edx, 03DAh
xor  eax, eax
wait_retrace:
    in   al, dx
    test al, 08h
    je   wait_retrace
pop  edx
ret
```

The original therefore polls VGA input-status port `0x3DA` until bit `0x08` is set. It has numerous render/game call sites.

The supplied README independently documents **Tab = vertical retrace sync** and says disabling it makes the game run much faster. Phase 2 now also recovers the corresponding DOS implementation: the key-state byte at offset `0x0F` (IBM PC set-1 Tab scan code) negates a 32-bit mode scalar at DOS data offset `0xD0`, switching it between `+1` and `-1`. The same path renders the object-4 strings `VERTICAL RETRACE`, `SYNC OFF`, and `SYNC ON`. Initialization writes `+1`.

At DOS gameplay/presentation address `0x0007DC89`, the mode scalar is compared to `+1`; only that value calls `0x0006940C`. This is the first proven Tab-gated gameplay retrace call and closes the former question of whether the DOS option actually owns the hardware retrace wait.

## Win32 QPC pacing mechanics

The canonical Windows PE import table exposes `QueryPerformanceCounter` but **not** `QueryPerformanceFrequency`; this has been re-verified directly from the executable import directory.

Established locations:

- QPC buffer allocation/initialization: near `0x0040B336`;
- previous low-32-bit counter: `0x004677E8`;
- current/delta bookkeeping: `0x0045A140` / `0x0048003C`;
- user-facing sync mode: `0x0042B1B4`;
- end-of-state-2 pacing loop: `0x0041144D..0x00411496`;
- threshold: `0x00003A98` = **15,000 counter counts**.

At gameplay start the code first obtains a fresh QPC sample and waits until its low 32 bits differ from the prior value, then updates the stored counter/delta. At the end of the active gameplay path it conditionally enforces the 15,000-count minimum:

```cpp
if (vertical_retrace_sync_mode == +1) {
    do {
        QueryPerformanceCounter(&counter);
    } while (uint32_t(counter.low - previous_counter_low) < 15000u);
}
```

The use of only low 32-bit arithmetic is part of the historical implementation and should not be silently rewritten into a claimed source-level design.

## Win32 Tab toggle is now proven

The state-2 input path at `0x0040C39C` tests virtual key **`0x09` (`VK_TAB`)**. When pressed it executes:

```asm
mov ecx, [0x0042B1B4]
neg ecx
mov [0x0042B1B4], ecx
```

Initialization at `0x00418051` writes **`+1`**, and subsequent Tab presses therefore toggle the value:

```text
+1 <-> -1
```

The same feedback path renders the literal strings:

```text
VERTICAL RETRACE
SYNC ON
SYNC OFF
```

through the recovered bitmap-text renderer. This directly resolves the former question of whether `0x0042B1B4` belongs to the documented Tab option: **it does**.

The `+1` value is the one that enables the 15,000-QPC-count wait. The original user-facing terminology is therefore retained in research names as `vertical_retrace_sync_mode`, even though this specific Win32 implementation enforces the option through a QPC busy wait.

## Separate Win32 DirectDraw vertical-blank primitive

The Windows build additionally has an explicit DirectDraw path:

```text
0x004018F0  wait_vertical_blank_thunk
       -> 0x004061E0
       -> IDirectDraw::WaitForVerticalBlank(flags=4, event=null)
```

The wrapper calls DirectDraw vtable slot `0x58`. The active state-2 region contains 11 static call instructions to the thunk, and multiple presentation paths call it immediately before the recovered software-framebuffer-to-surface copy.

This is the closest direct structural counterpart to DOS `0x0006940C`: both explicitly wait for display retrace/blanking. The QPC loop and DirectDraw wait therefore remain **separate recovered mechanisms**, even though the original Windows UI labels the QPC-controlled option “VERTICAL RETRACE.”

Do not collapse them into one imagined source routine.

## DOS Tab toggle and gameplay gate

Canonical DOS evidence now gives the same user-facing protocol as Win32:

```text
initialization: sync_mode = +1
Tab pressed:    sync_mode = -sync_mode
UI:             VERTICAL RETRACE / SYNC ON|OFF
frame path:     if (sync_mode == +1) wait_vga_vertical_retrace()
```

The recovered DOS sites are:

- initialization writer: `0x00080BD6` (`+1`);
- Tab key-state test / sign-flip: `0x00078338..0x0007834C`;
- sync feedback strings: object-4 offsets `0x5298`, `0x52AC`, `0x52B8`;
- proven gated gameplay/presentation wait: `0x0007DC89..0x0007DC97`;
- hardware wait primitive: `0x0006940C`.

The DOS low-address key-state table is independently corroborated in the same gameplay body by set-1 scan-code locations used for Ctrl (`0x1D`) and Space (`0x39`). This makes the `0x0F` Tab identification high confidence.

## What is now proven

- DOS has a VGA `0x3DA` retrace wait.
- DOS Tab negates its retrace-sync scalar between `+1/-1`, defaulting to `+1`.
- At least one canonical DOS gameplay/presentation frame path calls the VGA retrace wait iff that scalar is `+1`.
- Win32 has a DirectDraw `WaitForVerticalBlank` wrapper.
- Win32 also has a 15,000-QPC-count busy-wait pacing guard.
- Win32 `VK_TAB` directly toggles that guard between `+1/-1`.
- The Win32 UI names the option `VERTICAL RETRACE` and reports `SYNC ON/OFF`.
- Default Win32 value is `+1` / sync-on.

## Shared gameplay-update / replay boundary

Phase 2 now identifies a cross-build logical update clock independent of wall-clock units.

Win32 state 2 resets `demo_frame_index` (`0x0047EBD4`) to zero during session initialization. Near the top of each active gameplay invocation, after initial bookkeeping and before replay consumers, it executes:

```cpp
if (demo_recording_enabled == 1 || demo_playback_mode == 1)
    ++demo_frame_index;
```

The same invocation later compares the index with `0x82F` (2095). The QPC sync guard sits near the **tail** of this same state-2 invocation, after the bulk of gameplay work and before final presentation copying. Thus one replay-index increment corresponds to one logical active-gameplay update in Win32.

DOS independently implements the same protocol inside its active gameplay body:

- recording flag: data offset `0x4CE84`;
- playback flag: data offset `0x4CE88`;
- replay index: data offset `0x4CE8C`;
- gated increment: `0x00077C34..0x00077C46`;
- terminal comparison: `0x000782E0`;
- Tab-controlled retrace wait near the presentation tail: `0x0007DC89..0x0007DC97`.

DOS also resets the replay index to zero before use. Both builds therefore share **zero reset → one pre-increment per active gameplay update → terminal test at 2095**.

This closes an important architectural question: replay progression is an engine-update counter, not a separate timer. It still does **not** provide an Hz value by itself.

The canonical DAT files contain 2,101 physical records, so runtime cutoff and file length are intentionally kept separate. The clean core now models the executable timeline explicitly in `DemoReplayTimeline`.

## Canonical DOS fidelity cadence

The earlier prohibition against declaring a fixed DOS cadence is now superseded by stronger evidence. The combination is specific enough to define the clean fidelity scheduler:

1. DOS startup explicitly selects BIOS mode `0x13`;
2. the replay index advances exactly once per active logical gameplay update;
3. the ordinary sync-enabled DOS gameplay tail performs one VGA vertical-retrace wait per such update;
4. standard VGA mode-13h timing is 25.175 MHz / (800 clocks × 449 scanlines), nominally **~70.0863 Hz**.

`drone::gameplay::canonical_dos_fidelity_tick_hz` and `canonical_dos_fidelity_tick_seconds` encode this historical DOS reference. At that cadence the executable replay terminal index of 2095 corresponds to roughly **29.89 seconds** of canonical DOS gameplay time. This is a fidelity-derived duration, not a claim that the 2,101 physical DAT rows are themselves timestamped.

Two cautions still stand:

- the Win32 `15,000` QPC-count guard must **not** be converted to milliseconds or FPS because the executable never calls `QueryPerformanceFrequency`;
- exceptional presentation/state-2 paths contain additional explicit retrace/vblank waits, so they must be modeled as exceptional blocking behavior rather than silently redefining the ordinary logical tick.

## Win32 four-phase gameplay substep

A separate timing-like mechanism inside active Win32 state 2 is now directly established. Global `0x0053C4D8` is advanced at `0x0040C001..0x0040C01D` using the equivalent of:

```text
old = gameplay_substep_phase
next = old + 1
if old >= 3:
    next = 0
gameplay_substep_phase = next
```

On the canonical gameplay domain the exact cycle is therefore **`0 → 1 → 2 → 3 → 0`**. The increment/reset occurs near the beginning of each state-2 logical update, so later consumers see the already-advanced value. Phase `2` is an established slower-work gate for normal trajectory AUX animation, live trajectory breakaway eligibility, shield-loop sound cadence, and other selected animation/effect/destruction work. Position/path work can still run on every logical update.

This is **not** a wall-clock frame-rate proof. It is an internal four-way substep selector inside gameplay. The storage is also reused after active gameplay in later menu/post-game code, so the semantic name applies specifically to its state-2 lifetime. See `docs/reverse/GAMEPLAY_UPDATE_PHASE.md` for the recovered ordering landmarks and clean helper.

## Remaining timing work

The foundational DOS scheduler and Win32 four-phase substep questions are resolved. Remaining timing archaeology is narrower:

- classify exceptional state-2 retrace/vblank sites and their exact conditions;
- partition the remaining branch-specific Win32 state-2 update/collision/effect/render ordering and compare equivalent DOS/Win32 subsystem cadence where useful;
- use shared demo recordings as deterministic cross-build checkpoints as the clean simulation expands;
- preserve the Win32 port's machine-dependent QPC behavior as historical evidence without using it as the portable core's canonical scheduler.
