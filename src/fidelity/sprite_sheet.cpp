#include <drone/fidelity/sprite_sheet.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace drone::fidelity {
namespace {
std::size_t checked_mul(std::size_t a, std::size_t b, const char* what) {
    if (a != 0 && b > std::numeric_limits<std::size_t>::max() / a) {
        throw std::runtime_error(what);
    }
    return a * b;
}
}

IndexedSpriteFrame extract_guttered_jba_frame(
    const formats::JbaImage& sheet,
    std::size_t sprite_width,
    std::size_t sprite_height,
    std::size_t cell_x,
    std::size_t cell_y) {

    if (sheet.pixels.size() != formats::JbaImage::pixel_count) {
        throw std::runtime_error("JBA sheet pixel buffer has wrong size");
    }
    if (sprite_width == 0 || sprite_height == 0) {
        throw std::runtime_error("sprite dimensions must be non-zero");
    }

    // Win32 0x00401860:
    //   source_x = cell_x * (sprite_width  + 1) + 1
    //   source_y = cell_y * (sprite_height + 1) + 1
    // followed by sprite_height copies of sprite_width bytes from a 320-byte
    // source stride. The +1 terms are the sheet gutters.
    const auto x_stride = sprite_width + 1;
    const auto y_stride = sprite_height + 1;
    const auto source_x = checked_mul(cell_x, x_stride, "sprite sheet X origin overflow") + 1;
    const auto source_y = checked_mul(cell_y, y_stride, "sprite sheet Y origin overflow") + 1;

    if (source_x > formats::JbaImage::width || sprite_width > formats::JbaImage::width - source_x ||
        source_y > formats::JbaImage::height || sprite_height > formats::JbaImage::height - source_y) {
        throw std::runtime_error("sprite cell lies outside decoded 320x200 JBA sheet");
    }

    IndexedSpriteFrame out;
    out.width = sprite_width;
    out.height = sprite_height;
    out.pixels.resize(checked_mul(sprite_width, sprite_height, "sprite frame size overflow"));

    for (std::size_t y = 0; y < sprite_height; ++y) {
        const auto src = (source_y + y) * formats::JbaImage::width + source_x;
        const auto dst = y * sprite_width;
        std::copy_n(sheet.pixels.begin() + static_cast<std::ptrdiff_t>(src),
                    static_cast<std::ptrdiff_t>(sprite_width),
                    out.pixels.begin() + static_cast<std::ptrdiff_t>(dst));
    }
    return out;
}

void write_sprite_ppm(
    const IndexedSpriteFrame& frame,
    const std::array<formats::Rgb8, 256>& palette,
    const std::filesystem::path& path) {

    if (frame.width == 0 || frame.height == 0 ||
        frame.pixels.size() != frame.width * frame.height) {
        throw std::runtime_error("sprite frame has inconsistent dimensions");
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("could not create sprite PPM: " + path.string());
    out << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
    for (const auto index : frame.pixels) {
        const auto& c = palette[index];
        out.put(static_cast<char>(c.r));
        out.put(static_cast<char>(c.g));
        out.put(static_cast<char>(c.b));
    }
}

} // namespace drone::fidelity
