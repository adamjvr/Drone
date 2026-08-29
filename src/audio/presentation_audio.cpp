#include <drone/audio/presentation_audio.hpp>

namespace drone::audio {
namespace {

[[nodiscard]] constexpr bool lowbees_cleanup_state(const std::int32_t raw) noexcept {
    return raw == 0 || raw == 2 || raw == 7 || raw == 13 || raw == -1;
}

} // namespace

AudioEventQueue begin_original_main_menu_audio(MainMenuAudioRuntimeState& state) noexcept {
    AudioEventQueue events{};
    if (!state.lowbees_restart_armed) {
        return events;
    }

    state.lowbees_restart_armed = false;
    state.lowbees_owned = true;
    state.lowbees_volume_0_to_100 = original_main_menu_lowbees_start_volume;
    (void)events.push({AudioCue::MainMenuLowBees, AudioAction::SetVolume,
                       original_main_menu_lowbees_start_volume});
    (void)events.push({AudioCue::MainMenuLowBees, AudioAction::Play});
    return events;
}

AudioEventQueue tick_original_main_menu_audio(MainMenuAudioRuntimeState& state) noexcept {
    AudioEventQueue events{};
    if (!state.lowbees_owned ||
        state.lowbees_volume_0_to_100 >= original_main_menu_lowbees_volume_cap) {
        return events;
    }

    ++state.lowbees_volume_0_to_100;
    (void)events.push({AudioCue::MainMenuLowBees, AudioAction::SetVolume,
                       state.lowbees_volume_0_to_100});
    return events;
}

AudioEventQueue leave_original_main_menu_audio(
    MainMenuAudioRuntimeState& state, const std::int32_t raw_game_state) noexcept {
    AudioEventQueue events{};
    if (!state.lowbees_owned || !lowbees_cleanup_state(raw_game_state)) {
        return events;
    }

    (void)events.push({AudioCue::MainMenuLowBees, AudioAction::StopAndRewind});
    state.lowbees_owned = false;
    state.lowbees_restart_armed = true;
    state.lowbees_volume_0_to_100 = original_main_menu_lowbees_start_volume;
    return events;
}

AudioEventQueue original_main_menu_air_restart(
    OriginalAudioRuntimeState& audio, const std::int32_t raw_game_state) noexcept {
    AudioEventQueue events{};
    if (raw_game_state == 0) {
        return events;
    }

    (void)events.push({AudioCue::AirLoop, AudioAction::Play});
    audio.air_loop_volume_0_to_100 = original_air_loop_restart_volume;
    (void)events.push({AudioCue::AirLoop, AudioAction::SetVolume,
                       original_air_loop_restart_volume});
    (void)events.push({AudioCue::AirLoop, AudioAction::SetFrequency,
                       static_cast<std::int32_t>(original_air_loop_menu_frequency_hz)});
    return events;
}

AudioEventQueue tick_original_gameplay_overlay_audio(OriginalAudioRuntimeState& audio) noexcept {
    AudioEventQueue events{};
    if (audio.air_loop_volume_0_to_100 > 0) {
        --audio.air_loop_volume_0_to_100;
        (void)events.push({AudioCue::AirLoop, AudioAction::SetVolume,
                           audio.air_loop_volume_0_to_100});
        return events;
    }

    (void)events.push({AudioCue::AirLoop, AudioAction::StopAndRewind});
    return events;
}

AudioEventQueue resume_original_gameplay_overlay_audio(OriginalAudioRuntimeState& audio) noexcept {
    AudioEventQueue events{};
    (void)events.push({AudioCue::AirLoop, AudioAction::Play});
    audio.air_loop_volume_0_to_100 = original_air_loop_loaded_volume;
    (void)events.push({AudioCue::AirLoop, AudioAction::SetVolume,
                       original_air_loop_loaded_volume});
    return events;
}

} // namespace drone::audio
