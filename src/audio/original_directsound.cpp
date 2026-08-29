#include <drone/audio/original_directsound.hpp>

namespace drone::audio {

std::size_t select_original_sfx_voice(
    const std::array<std::uint32_t, original_sfx_voice_pool_capacity>& raw_status) noexcept {
    for (std::size_t index = 0; index < raw_status.size(); ++index) {
        if (raw_status[index] != directsound_status_playing) {
            return index;
        }
    }
    return 0;
}

} // namespace drone::audio
