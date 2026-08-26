// Research pseudocode only. This is not linked into the clean implementation.
// Derived from Win32 addresses around 0x40B336, 0x40BAC8 and 0x411456.

struct QpcLowState {
    uint32_t previous_low;
    uint32_t current_low;
    uint32_t delta_low;
};

void observed_frame_timing(QpcLowState& s, bool limiter_enabled) {
    // Original allocates 8 bytes for LARGE_INTEGER but subsequently consumes the low dword.
    do {
        s.current_low = QueryPerformanceCounter_low32();
        s.delta_low = s.current_low - s.previous_low;
    } while (s.current_low == s.previous_low); // early gameplay path observes a changed count

    // ...simulation/render work...

    if (limiter_enabled) {
        do {
            s.current_low = QueryPerformanceCounter_low32();
        } while (uint32_t(s.current_low - s.previous_low) < 15000u);
    }
}
