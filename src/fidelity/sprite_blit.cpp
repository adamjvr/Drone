#include <drone/fidelity/sprite_blit.hpp>

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace drone::fidelity {

void blit_transparent_original(
    IndexedFramebuffer& framebuffer,
    const IndexedSpriteFrame& frame,
    std::int32_t x,
    std::int32_t y) {

    if (frame.width == 0 || frame.height == 0 || frame.pixels.size() != frame.width * frame.height) {
        throw std::runtime_error("sprite frame has inconsistent dimensions");
    }

    const auto width = static_cast<std::int32_t>(frame.width);
    const auto height = static_cast<std::int32_t>(frame.height);
    constexpr std::int32_t max_x = 319; // literal used by Win32 0x00401660
    constexpr std::int32_t screen_height = 200;

    if (x > max_x || y >= screen_height || x + width <= 0 || y + height <= 0) return;

    std::int32_t source_skip_x = 0;
    std::int32_t copy_width = width;
    if (x < 0) {
        source_skip_x = -x;
        x = 0;
        copy_width = width - source_skip_x;
    } else if (x + width >= max_x) {
        // This is deliberately 319-x rather than 320-x because that is the
        // original Win32 branch. The clean compatibility implementation keeps
        // the quirk visible and tested rather than silently "fixing" it.
        copy_width = max_x - x;
    }

    std::int32_t source_skip_y = 0;
    std::int32_t copy_height = height;
    if (y < 0) {
        source_skip_y = -y;
        y = 0;
        copy_height = height - source_skip_y;
    } else if (y + height >= screen_height) {
        copy_height = screen_height - y;
    }

    if (copy_width <= 0 || copy_height <= 0) return;

    auto& dst = framebuffer.pixels();
    for (std::int32_t row = 0; row < copy_height; ++row) {
        const auto src_row = static_cast<std::size_t>(source_skip_y + row) * frame.width;
        const auto dst_row = static_cast<std::size_t>(y + row) * IndexedFramebuffer::width;
        for (std::int32_t col = 0; col < copy_width; ++col) {
            const auto pixel = frame.pixels[src_row + static_cast<std::size_t>(source_skip_x + col)];
            if (pixel != 0) {
                dst[dst_row + static_cast<std::size_t>(x + col)] = pixel;
            }
        }
    }
}

} // namespace drone::fidelity
