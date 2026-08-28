#pragma once

#include <drone/fidelity/sprite_sheet.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace drone::fidelity {

// Recovered FONT2.JBA bitmap-font geometry shared by the DOS and Win32 builds.
// The original cache stores 64 descriptors at 0x14 bytes each, but the clean
// engine intentionally models only the semantic layout rather than the raw
// pointer-bearing historical record.
inline constexpr std::uint8_t font2_first_character = 0x20;
inline constexpr std::size_t font2_glyph_count = 64;
inline constexpr std::size_t font2_columns = 16;
inline constexpr std::size_t font2_rows = 4;
inline constexpr std::size_t font2_glyph_width = 7;
inline constexpr std::size_t font2_glyph_height = 5;
inline constexpr std::size_t font2_cell_width = 8;
inline constexpr std::size_t font2_cell_height = 6;
inline constexpr std::size_t font2_advance_x = 8;

struct Font2GlyphLayout {
    std::size_t index{};
    std::size_t cell_x{};
    std::size_t cell_y{};
    std::size_t source_x{};
    std::size_t source_y{};
    std::size_t width{font2_glyph_width};
    std::size_t height{font2_glyph_height};
};

// Maps the exact original table domain, ASCII 0x20 (' ') through 0x5F ('_').
// The original routine directly indexed outside this range; the clean API
// rejects unsupported bytes instead of reproducing unsafe memory access.
std::optional<std::size_t> font2_glyph_index(std::uint8_t character) noexcept;

// Returns the recovered source rectangle within decoded FONT2.JBA.
// Throws std::out_of_range for index >= 64.
Font2GlyphLayout font2_glyph_layout(std::size_t index);

// Extracts the 7x5 mask represented by one original FONT2 descriptor. This is
// equivalent to the original 320-byte-stride copy and reuses the established
// one-pixel-gutter JBA sheet contract.
IndexedSpriteFrame extract_font2_glyph(
    const formats::JbaImage& sheet,
    std::size_t index);

} // namespace drone::fidelity
