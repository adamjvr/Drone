#pragma once

#include <array>
#include <cstddef>
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

// Windows shareware also ships three much smaller .JBA files whose payload is
// a self-describing preamble followed by a 128x128 8-bit PCX stream and a raw
// 256xRGB8 palette. This is a distinct physical family from JbaImage.
struct SmallJbaPcxImage {
    static constexpr std::size_t width = 128;
    static constexpr std::size_t height = 128;
    static constexpr std::size_t pixel_count = width * height;
    static constexpr std::size_t pcx_header_bytes = 128;
    static constexpr std::size_t palette_bytes = 256 * 3;

    std::uint8_t preamble_length{};
    std::vector<std::uint8_t> opaque_preamble;
    std::array<Rgb8, 256> palette{};
    std::vector<std::uint8_t> pixels;
};

JbaImage load_jba_320x200(const std::filesystem::path& path);
SmallJbaPcxImage load_small_jba_pcx128(const std::filesystem::path& path);

void write_ppm(const JbaImage& image, const std::filesystem::path& path);
void write_ppm(const SmallJbaPcxImage& image, const std::filesystem::path& path);

} // namespace drone::formats
