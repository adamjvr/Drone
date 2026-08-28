#pragma once

#include <drone/formats/jba.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace drone::fidelity {

// Compatibility framebuffer contract recovered from the original renderer:
// 320x200, one 8-bit palette index per pixel, 256 RGB entries.
class IndexedFramebuffer {
public:
    static constexpr std::size_t width = 320;
    static constexpr std::size_t height = 200;
    static constexpr std::size_t pixel_count = width * height;

    IndexedFramebuffer();

    void load(const formats::JbaImage& image);

    std::array<formats::Rgb8, 256>& palette() noexcept { return palette_; }
    const std::array<formats::Rgb8, 256>& palette() const noexcept { return palette_; }
    std::vector<std::uint8_t>& pixels() noexcept { return pixels_; }
    const std::vector<std::uint8_t>& pixels() const noexcept { return pixels_; }

    // RGBA byte order, primarily for native presentation backends and tests.
    std::vector<std::uint8_t> rgba8() const;

private:
    std::array<formats::Rgb8, 256> palette_{};
    std::vector<std::uint8_t> pixels_;
};

} // namespace drone::fidelity
