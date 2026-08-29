#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace drone::audio {

// Win32 0x00420020 operates on exactly twenty reusable DirectSound buffers.
inline constexpr std::size_t original_sfx_voice_pool_capacity = 20;
inline constexpr std::uint32_t directsound_status_playing = 1;

// The original pool helper restarts the first voice whose raw GetStatus value
// is not exactly DSBSTATUS_PLAYING (1). If every entry is exactly 1, voice 0 is
// stolen/restarted. No round-robin cursor is maintained.
[[nodiscard]] std::size_t select_original_sfx_voice(
    const std::array<std::uint32_t, original_sfx_voice_pool_capacity>& raw_status) noexcept;

// Win32 0x00406780 converts the game's 0..100 volume scale to DirectSound's
// hundredths-of-a-decibel attenuation with 30 * (volume - 100).
[[nodiscard]] constexpr std::int32_t original_directsound_attenuation(
    const std::int32_t volume_0_to_100) noexcept {
    return 30 * (volume_0_to_100 - 100);
}

inline constexpr std::uint32_t original_missile_frequency_hz = 22050;
inline constexpr std::uint32_t original_shield_frequency_hz = 11025;
inline constexpr std::uint32_t original_bigexp3_frequency_hz = 15000;

} // namespace drone::audio
