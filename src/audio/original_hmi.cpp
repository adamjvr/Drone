#include <drone/audio/original_hmi.hpp>

namespace drone::audio {

std::optional<std::size_t> select_original_drone_dos_hmi_voice(
    const std::array<std::uint32_t, original_drone_dos_hmi_voice_count>& voice_flags) noexcept {
    for (std::size_t index = 0; index < voice_flags.size(); ++index) {
        if ((voice_flags[index] & original_hmi_sample_flag_active) == 0) return index;
    }
    return std::nullopt;
}

const OriginalHmiSosApiContract& original_hmi_sos_api_contract() noexcept {
    static constexpr OriginalHmiSosApiContract contract{
        .default_mixer_channels = original_hmi_default_mixer_channels,
        .max_voice_capability = original_hmi_max_voice_capability,
        .descriptor_backed_start = true,
        .explicit_stop_by_handle = true,
        .explicit_stop_all_samples = true,
        .sample_done_query = true,
        .descriptor_has_volume = true,
        .descriptor_has_loop_count = true,
        .descriptor_has_sample_rate = true,
        .descriptor_has_pan = true,
        .descriptor_has_priority = true,
        .descriptor_has_completion_callbacks = true,
        .runtime_set_volume = true,
        .runtime_set_sample_rate = true,
        .runtime_set_pan = true,
        .api_requires_preduplicated_sample_pool = false,
    };
    return contract;
}

const OriginalDroneDosHmiRuntimeContract& original_drone_dos_hmi_runtime_contract() noexcept {
    static constexpr OriginalDroneDosHmiRuntimeContract contract{
        .configured_voice_count = original_drone_dos_hmi_voice_count,
        .voice_record_size = original_drone_dos_hmi_voice_record_size,
        .voice_storage_bytes = original_drone_dos_hmi_voice_storage_bytes,
        .first_inactive_voice_wins = true,
        .saturation_policy = OriginalDroneDosVoiceSaturationPolicy::ReturnFailure,
        .priority_used_for_voice_selection = false,
        .start_copies_full_sample_descriptor = true,
        .start_returns_voice_index = true,
        .packed_left_right_16_volume = true,
        .universal_normalized_volume_mapping = false,
        .one_shot_loop_count = original_drone_dos_hmi_one_shot_loop_count,
        .infinite_loop_count = original_drone_dos_hmi_infinite_loop_count,
        .infinite_loop_uses_loop_count_field = true,
        .retained_voice_handles_for_runtime_control = true,
        .master_digital_volume_control_va = original_drone_dos_hmi_master_digital_volume_va,
        .master_volume_full = original_drone_dos_hmi_master_volume_full,
        .master_volume_mask = original_drone_dos_hmi_master_volume_mask,
        .master_control_is_distinct_from_sample_volume = true,
    };
    return contract;
}


const OriginalDroneDosPresentationAudioContract&
original_drone_dos_presentation_audio_contract() noexcept {
    static constexpr OriginalDroneDosMenuAudioTransition menu_transitions[]{
        {OriginalDroneDosMenuSelection::PlayGame, true, 2, true, true, true, false, 0, false},
        {OriginalDroneDosMenuSelection::Instructions, true, 3, false, false, false, false,
         0x00085A04u, true},
        {OriginalDroneDosMenuSelection::OrderingInformation, true, 7, true, true, true, true,
         0x00085D10u, true},
        {OriginalDroneDosMenuSelection::HighScores, true, 8, false, false, false, false,
         0x00086E04u, true},
        {OriginalDroneDosMenuSelection::ConfigureJoystick, false, 0, false, false, false, false,
         0, false},
        {OriginalDroneDosMenuSelection::PlayDemo, true, 13, true, true, true, false, 0, false},
        {OriginalDroneDosMenuSelection::Quit, true, 0, true, true, true, false, 0, false},
    };

    static constexpr OriginalDroneDosPresentationAudioContract contract{
        .lowbees_descriptor_va = 0x004CBE0u,
        .lowbees_voice_handle_va = 0x004CBF0u,
        .lowbees_restart_byte_va = 0x0083881u,
        .lowbees_start_call_va = 0x00081709u,
        .lowbees_stop_call_va = 0x00082C73u,
        .ordering_lowbees_stop_call_va = 0x00082E2Fu,
        .lowbees_initial_level = 0,
        .lowbees_fade_step = 0x007Du,
        .lowbees_fade_threshold = 0x7000u,
        .lowbees_terminal_written_level = 0x704Eu,
        .lowbees_loop_count = original_drone_dos_hmi_infinite_loop_count,
        .air_descriptor_va = 0x004CBBCu,
        .air_voice_handle_va = 0x004CCC8u,
        .air_start_call_va = 0x00077737u,
        .master_digital_volume_control_va = original_drone_dos_hmi_master_digital_volume_va,
        .master_full_level = original_drone_dos_hmi_master_volume_full,
        .active_master_fade_step = 0x0096u,
        .overlay_master_fade_step = 0x015Eu,
        .overlay_full_start_terminal_remainder = 0x00D9u,
        .pause_raw_state = 5,
        .quit_confirmation_raw_state = 6,
        .nine_lives_raw_state = 99,
        .overlay_directly_controls_air_sample = false,
        .air_voice_continues_through_overlay = true,
        .resume_state_2_restarts_air = false,
        .resume_state_2_restores_master_full = true,
        .teardown_air_sample_done_call_va = 0x0007DF24u,
        .teardown_air_stop_call_va = 0x0007DF38u,
        .teardown_checks_sample_done_before_stop = true,
        .air_survives_gameplay_to_menu_transition = false,
        .menu_transitions = menu_transitions,
        .menu_transition_count = 7,
    };
    return contract;
}

} // namespace drone::audio
