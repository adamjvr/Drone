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

} // namespace drone::audio
