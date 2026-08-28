#include <drone/fidelity/indexed_framebuffer.hpp>

#include <stdexcept>

namespace drone::fidelity {

IndexedFramebuffer::IndexedFramebuffer() : pixels_(pixel_count, 0) {}

void IndexedFramebuffer::load(const formats::JbaImage& image) {
    if (image.pixels.size() != pixel_count) {
        throw std::runtime_error("cannot load JBA with non-320x200 pixel buffer into fidelity framebuffer");
    }
    palette_ = image.palette;
    pixels_ = image.pixels;
}

std::vector<std::uint8_t> IndexedFramebuffer::rgba8() const {
    if (pixels_.size() != pixel_count) {
        throw std::runtime_error("fidelity framebuffer pixel buffer has wrong size");
    }
    std::vector<std::uint8_t> out(pixel_count * 4);
    for (std::size_t i = 0; i < pixel_count; ++i) {
        const auto& c = palette_[pixels_[i]];
        out[i * 4 + 0] = c.r;
        out[i * 4 + 1] = c.g;
        out[i * 4 + 2] = c.b;
        out[i * 4 + 3] = 255;
    }
    return out;
}

} // namespace drone::fidelity
