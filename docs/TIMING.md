# Timing Research

Timing is one of the highest-risk reconstruction areas because the original Windows executable uses a counter threshold without querying the counter frequency, while the DOS documentation exposes a user-toggleable “vertical retrace sync” mode that materially changes speed.

## Confirmed Win32 mechanics

The canonical Windows executable imports `QueryPerformanceCounter` but **not** `QueryPerformanceFrequency`.

Observed locations:

- timestamp buffer allocation/initialization: near `0x0040B336`;
- initial low-32-bit counter copied to `0x004677E8`;
- gameplay path obtains a new counter and stores low-32-bit delta at `0x0048003C`;
- pacing guard global: `0x0042B1B4`;
- pacing loop: `0x00411456`–`0x00411494`;
- threshold: `0x00003A98` = **15,000 counter counts**.

Behavioral pseudocode:

```cpp
if (frame_limiter_enabled == 1) {
    do {
        QueryPerformanceCounter(&counter);
    } while (uint32_t(counter.low - previous_counter_low) < 15000u);
}
```

The name `frame_limiter_enabled` describes observed function, not yet the original user-facing option name.

## Documented DOS/shareware control

The supplied v1.01 README says **Tab toggles “vertical retrace sync”** and that disabling it makes the game run much faster.

This is strong black-box evidence that the original game exposes a pacing/synchronization guard to input. It is **not yet proven** that Win32 global `0x0042B1B4` is the exact setting toggled by Tab, although that is an obvious Phase 2 hypothesis to test.

## What is not established

### 15,000 counts are not automatically 15 ms

`QueryPerformanceCounter` frequency is platform-dependent. Since the executable does not call `QueryPerformanceFrequency`, the threshold cannot be converted to seconds from this code path alone.

Historical host implementations may have exposed different effective QPC frequencies. We therefore do not currently assert 60 Hz, 70 Hz, 80 Hz, or any other simulation rate.

### Update/render coupling is not yet established

The outer Win32 loop calls the game dispatcher around DirectDraw surface acquisition/presentation, but we still need to determine:

- whether every render iteration advances simulation exactly once;
- whether some states bypass or alter pacing;
- whether the QPC guard is equivalent to retrace sync, an additional limiter, or a porting approximation;
- whether DOS uses VGA retrace polling, PIT/timer interrupts, elapsed ticks, or a combination;
- whether gameplay constants were tuned to frame-dependent movement.

## Phase 2 plan

1. Map Tab input in Windows and determine whether it writes `0x0042B1B4` or a related setting.
2. Recover the DOS Tab/retrace path.
3. Recover DOS timer/retrace polling functions and identify frame/update boundaries.
4. Measure or emulate representative original behavior over multi-second intervals.
5. Compare player/projectile/scroll rates rather than relying only on display frame counts.
6. Determine whether a fixed simulation tick can be stated with high confidence.
7. Only then encode the canonical scheduler rate in `drone_core`.

## Clean-engine requirement

Even if the original tied simulation speed directly to frame pacing, the remaster should represent the reconstructed intended cadence explicitly so modern high-refresh displays do not speed up gameplay. A fidelity option can reproduce relevant original pacing quirks if they are behaviorally meaningful, but the core should not accidentally depend on host refresh rate.
