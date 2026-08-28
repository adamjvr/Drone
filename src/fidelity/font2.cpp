#include <drone/fidelity/font2.hpp>

#include <stdexcept>

namespace drone::fidelity {

std::optional<std::size_t> font2_glyph_index(std::uint8_t character) noexcept {
    if (character < font2_first_character) {
        return std::nullopt;
    }
    const auto index = static_cast<std::size_t>(character - font2_first_character);
    if (index >= font2_glyph_count) {
        return std::nullopt;
    }
    return index;
}

Font2GlyphLayout font2_glyph_layout(std::size_t index) {
    if (index >= font2_glyph_count) {
        throw std::out_of_range("FONT2 glyph index lies outside original 64-entry table");
    }

    Font2GlyphLayout layout;
    layout.index = index;
    layout.cell_x = index % font2_columns;
    layout.cell_y = index / font2_columns;
    layout.source_x = 1 + layout.cell_x * font2_cell_width;
    layout.source_y = 1 + layout.cell_y * font2_cell_height;
    return layout;
}

IndexedSpriteFrame extract_font2_glyph(
    const formats::JbaImage& sheet,
    std::size_t index) {

    const auto layout = font2_glyph_layout(index);
    return extract_guttered_jba_frame(
        sheet,
        layout.width,
        layout.height,
        layout.cell_x,
        layout.cell_y);
}

} // namespace drone::fidelity
