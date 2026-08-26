#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace drone::formats {

struct Rgb8 {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
};

struct JbaImage {
    static constexpr std::size_t width = 320;
    static constexpr std::size_t height = 200;
    static constexpr std::size_t pixel_count = width * height;
    static constexpr std::size_t palette_bytes = 256 * 3;
    static constexpr std::size_t file_bytes = palette_bytes + pixel_count;

    std::array<Rgb8, 256> palette{};
    std::vector<std::uint8_t> pixels;
};

JbaImage load_fullscreen_jba(const std::filesystem::path& path);
void write_ppm(const JbaImage& image, const std::filesystem::path& path);

} // namespace drone::formats
