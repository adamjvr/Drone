#include <drone/audio/original_hmi.hpp>

namespace drone::audio {

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
    };
    return contract;
}

} // namespace drone::audio
