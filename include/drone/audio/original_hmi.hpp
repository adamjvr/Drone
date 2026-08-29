#pragma once

#include <cstdint>

namespace drone::audio {

// Middleware-level contract recovered from the public Human Machine Interfaces
// S.O.S. 4.x SDK headers.
inline constexpr std::uint32_t original_hmi_default_mixer_channels = 32;
inline constexpr std::uint32_t original_hmi_max_voice_capability = 32;
inline constexpr std::uint32_t original_hmi_sample_flag_active = 0x8000;
inline constexpr std::uint32_t original_hmi_sample_flag_processed = 0x4000;
inline constexpr std::uint32_t original_hmi_sample_flag_done = 0x2000;
inline constexpr std::uint32_t original_hmi_sample_flag_loop = 0x1000;
inline constexpr std::uint32_t original_hmi_pan_left = 0x0000;
inline constexpr std::uint32_t original_hmi_pan_center = 0x8000;
inline constexpr std::uint32_t original_hmi_pan_right = 0xFFFF;
inline constexpr std::uint32_t original_hmi_volume_max = 0x7FFF7FFF;

// Canonical Drone DOS executable facts recovered from the linked S.O.S. code.
inline constexpr std::uint32_t original_drone_dos_hmi_voice_count = 32;
inline constexpr std::uint32_t original_drone_dos_hmi_voice_record_size = 0xF0;
inline constexpr std::uint32_t original_drone_dos_hmi_voice_storage_bytes = 0x1E00;
inline constexpr std::uint32_t original_drone_dos_hmi_one_shot_loop_count = 0;
inline constexpr std::uint32_t original_drone_dos_hmi_infinite_loop_count = 0xFFFFFFFFu;

inline constexpr std::uint32_t original_drone_dos_hmi_start_sample_va = 0x0008AC82u;
inline constexpr std::uint32_t original_drone_dos_hmi_stop_sample_va = 0x0008AE02u;
inline constexpr std::uint32_t original_drone_dos_hmi_stop_all_samples_va = 0x0008AE74u;
inline constexpr std::uint32_t original_drone_dos_hmi_set_sample_volume_va = 0x0008AFC1u;
inline constexpr std::uint32_t original_drone_dos_hmi_set_sample_rate_va = 0x0008B2A7u;
inline constexpr std::uint32_t original_drone_dos_hmi_sample_done_va = 0x0008B549u;
inline constexpr std::uint32_t original_drone_dos_hmi_init_driver_va = 0x0008D4AFu;
inline constexpr std::uint32_t original_drone_dos_hmi_voice_count_write_va = 0x0008D78Au;

[[nodiscard]] constexpr std::uint32_t original_hmi_pack_equal_channel_volume(
    std::uint16_t level) noexcept {
    return static_cast<std::uint32_t>(level) |
           (static_cast<std::uint32_t>(level) << 16u);
}

struct OriginalHmiSosApiContract {
    std::uint32_t default_mixer_channels = 0;
    std::uint32_t max_voice_capability = 0;

    bool descriptor_backed_start = false;
    bool explicit_stop_by_handle = false;
    bool explicit_stop_all_samples = false;
    bool sample_done_query = false;

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
    // not an HMI S.O.S. API prerequisite.
    bool api_requires_preduplicated_sample_pool = false;
};

enum class OriginalDroneDosVoiceSaturationPolicy : std::uint8_t {
    ReturnFailure,
};

struct OriginalDroneDosHmiRuntimeContract {
    std::uint32_t configured_voice_count = 0;
    std::uint32_t voice_record_size = 0;
    std::uint32_t voice_storage_bytes = 0;

    // sosDIGIStartSample scans voice index 0 upward and selects the first
    // record without _SACTIVE. Saturation returns -1; there is no fallback
    // voice steal and the descriptor priority does not participate in this
    // allocation scan.
    bool first_inactive_voice_wins = false;
    OriginalDroneDosVoiceSaturationPolicy saturation_policy{};
    bool priority_used_for_voice_selection = false;
    bool start_copies_full_sample_descriptor = false;
    bool start_returns_voice_index = false;

    // Drone writes HMI-native packed left/right 16-bit levels directly. It
    // does not route sample volume through a universal 0..100 conversion like
    // the Win32 DirectSound helper. Mono paths can use the low word only.
    bool packed_left_right_16_volume = false;
    bool universal_normalized_volume_mapping = false;

    // Ordinary descriptors are zero-loop one-shots; sustained Drone samples
    // set wLoopCount to UINT32_MAX before start for indefinite repetition.
    std::uint32_t one_shot_loop_count = 0;
    std::uint32_t infinite_loop_count = 0;
    bool infinite_loop_uses_loop_count_field = false;

    // Controlled sounds retain the voice index returned by StartSample and
    // later use SampleDone / StopSample / SetSampleVolume / SetSampleRate.
    bool retained_voice_handles_for_runtime_control = false;
};

[[nodiscard]] const OriginalHmiSosApiContract& original_hmi_sos_api_contract() noexcept;
[[nodiscard]] const OriginalDroneDosHmiRuntimeContract& original_drone_dos_hmi_runtime_contract() noexcept;

} // namespace drone::audio
