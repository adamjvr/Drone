#pragma once

#include <cstdint>

namespace drone::audio {

// Middleware-level contract recovered from the public Human Machine Interfaces
// S.O.S. 4.x SDK headers. These constants describe library capability, not the
// number of voices or allocation policy selected by Drone's DOS executable.
inline constexpr std::uint32_t original_hmi_default_mixer_channels = 32;
inline constexpr std::uint32_t original_hmi_max_voice_capability = 32;
inline constexpr std::uint32_t original_hmi_sample_flag_active = 0x8000;
inline constexpr std::uint32_t original_hmi_sample_flag_processed = 0x4000;
inline constexpr std::uint32_t original_hmi_sample_flag_done = 0x2000;
inline constexpr std::uint32_t original_hmi_sample_flag_loop = 0x1000;
inline constexpr std::uint32_t original_hmi_pan_left = 0x0000;
inline constexpr std::uint32_t original_hmi_pan_center = 0x8000;
inline constexpr std::uint32_t original_hmi_pan_right = 0xFFFF;

struct OriginalHmiSosApiContract {
    std::uint32_t default_mixer_channels = 0;
    std::uint32_t max_voice_capability = 0;

    // sosDIGIStartSample consumes one caller-owned _SOS_SAMPLE descriptor.
    bool descriptor_backed_start = false;
    bool explicit_stop_by_handle = false;
    bool explicit_stop_all_samples = false;
    bool sample_done_query = false;

    // _SOS_SAMPLE fields and matching SOS API controls.
    bool descriptor_has_volume = false;
    bool descriptor_has_loop_count = false;
    bool descriptor_has_sample_rate = false;
    bool descriptor_has_pan = false;
    bool descriptor_has_priority = false;
    bool descriptor_has_completion_callbacks = false;
    bool runtime_set_volume = false;
    bool runtime_set_sample_rate = false;
    bool runtime_set_pan = false;

    // A twenty-copy reusable pool is a Win32 Drone/DirectSound construction,
    // not an HMI S.O.S. API prerequisite. Drone DOS may still maintain its own
    // game-level slots; that executable-specific policy remains open evidence.
    bool api_requires_preduplicated_sample_pool = false;
};

[[nodiscard]] const OriginalHmiSosApiContract& original_hmi_sos_api_contract() noexcept;

} // namespace drone::audio
