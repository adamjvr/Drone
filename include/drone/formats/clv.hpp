#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace drone::formats {

struct ClvAudio {
    static constexpr std::uint32_t sample_rate = 22050;
    std::vector<std::uint8_t> interleaved_stereo_u8;

    [[nodiscard]] std::size_t frames() const noexcept {
        return interleaved_stereo_u8.size() / 2;
    }
};

ClvAudio load_clv(const std::filesystem::path& path);
std::vector<std::uint8_t> downmix_clv_to_mono(const ClvAudio& audio);
void write_mono_u8_wav(const std::vector<std::uint8_t>& samples,
                       std::uint32_t sample_rate,
                       const std::filesystem::path& path);

} // namespace drone::formats
