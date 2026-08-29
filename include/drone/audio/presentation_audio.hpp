#pragma once

#include <drone/audio/audio_event.hpp>

#include <cstdint>

namespace drone::audio {

// Clean host-side state for the original main-menu ambience owner. This is
// intentionally separate from GameSession: the Win32 owner is run_main_menu,
// and its lifetime spans synchronous menu modals/re-entry paths.
struct MainMenuAudioRuntimeState {
    // Win32 byte 0x00459F8C. Process/bootstrap code arms this to 1; entering
    // the menu consumes it when lowbees.wav is loaded/started. Cleanup on the
    // established exit states arms it again for the next menu entry.
    bool lowbees_restart_armed = true;
    bool lowbees_owned = false;
    std::int32_t lowbees_volume_0_to_100 = original_main_menu_lowbees_start_volume;
};

// Win32 run_main_menu entry, 0x00418ADB..0x00418B1C. When the restart byte is
// armed the original loads lowbees.wav, SetVolume(0), then Play(..., flags=1).
[[nodiscard]] AudioEventQueue begin_original_main_menu_audio(
    MainMenuAudioRuntimeState& state) noexcept;

// Win32 0x004190F9..0x00419110: one unit of menu-volume fade-in per menu loop,
// capped at 80. No event is emitted once the cap has been reached.
[[nodiscard]] AudioEventQueue tick_original_main_menu_audio(
    MainMenuAudioRuntimeState& state) noexcept;

// Win32 0x00419DA6..0x00419DE3. The original tears lowbees down when the raw
// state is 0, 2, 7, 13 or -1; instructions/high-scores/re-entry do not take
// this cleanup branch. A StopAndRewind event is emitted and ownership ends.
// The subsequent DirectSound release is represented by the clean owner flag,
// rather than making platform resource destruction part of gameplay events.
[[nodiscard]] AudioEventQueue leave_original_main_menu_audio(
    MainMenuAudioRuntimeState& state, std::int32_t raw_game_state) noexcept;

// Win32 0x0041A3D6..0x0041A41C. After run_main_menu returns, every non-zero
// raw state restarts air.wav with this exact ordered sequence:
// Play(loop) -> scalar=0 -> SetVolume(0) -> SetFrequency(11025).
[[nodiscard]] AudioEventQueue original_main_menu_air_restart(
    OriginalAudioRuntimeState& audio, std::int32_t raw_game_state) noexcept;

// Win32 states 5/6/99 share 0x0040C638..0x0040C665. Each overlay iteration
// decrements the air scalar by one and applies it while positive; at zero the
// original stops/rewinds the air slot instead.
[[nodiscard]] AudioEventQueue tick_original_gameplay_overlay_audio(
    OriginalAudioRuntimeState& audio) noexcept;

// When an overlay resumes to active state 2, 0x0040C82C..0x0040C913 restarts
// the air loop and restores scalar/volume 50. This helper is deliberately host
// side; GameSession does not own pause/confirmation presentation loops.
[[nodiscard]] AudioEventQueue resume_original_gameplay_overlay_audio(
    OriginalAudioRuntimeState& audio) noexcept;

// Host-owned credits fade state. Win32 run_completion_credits keeps this
// scalar local to the synchronous presentation routine; it is deliberately
// not part of GameSession or OriginalAudioRuntimeState.
struct CompletionCreditsAudioRuntimeState {
    std::int32_t volume_0_to_100 = original_completion_credits_fade_start_volume;
    bool fade_active = false;
    bool fade_complete = false;
};

struct CompletionCreditsAudioStepResult {
    AudioEventQueue audio_events{};
    bool fade_completed = false;
};

// Win32 0x00404ACC begins only after the scrolling credits have reached their
// visual terminal condition (the final tracked Y <= -40). No SetVolume is
// performed here: the original merely enters the fade loop with scalar 100.
void begin_original_completion_credits_fade(
    CompletionCreditsAudioRuntimeState& state) noexcept;

// Win32 0x00404ACC..0x00404B13: one synchronous fade/presentation iteration.
// The scalar is decremented before SetVolume, yielding exactly 99..0 across
// 100 calls. When zero is emitted, fade_completed becomes true; the host then
// completes the modal, whose existing GameSession boundary emits the original
// StopAndRewind before stack-local DirectSound release.
[[nodiscard]] CompletionCreditsAudioStepResult tick_original_completion_credits_fade(
    CompletionCreditsAudioRuntimeState& state) noexcept;

} // namespace drone::audio
