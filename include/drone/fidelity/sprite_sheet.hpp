#pragma once

#include <drone/formats/jba.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace drone::fidelity {

struct IndexedSpriteFrame {
    std::size_t width{};
    std::size_t height{};
    std::vector<std::uint8_t> pixels;
};

// Reconstructs Win32 routine 0x00401860's sheet-cropping contract.
// Sprite cells are separated by a one-pixel gutter on a decoded 320x200 JBA
// sheet. The original routine stores the returned allocation into a caller-
// selected frame slot; frame-slot selection is intentionally separated from
// this pure extraction helper.
IndexedSpriteFrame extract_guttered_jba_frame(
    const formats::JbaImage& sheet,
    std::size_t sprite_width,
    std::size_t sprite_height,
    std::size_t cell_x,
    std::size_t cell_y);

void write_sprite_ppm(
    const IndexedSpriteFrame& frame,
    const std::array<formats::Rgb8, 256>& palette,
    const std::filesystem::path& path);

} // namespace drone::fidelity
