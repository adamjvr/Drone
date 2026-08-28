#pragma once

#include <drone/fidelity/indexed_framebuffer.hpp>
#include <drone/fidelity/sprite_sheet.hpp>

#include <cstdint>

namespace drone::fidelity {

// Semantic reconstruction of Win32 0x00401660.
// Palette index 0 is transparent. The clipping contract intentionally mirrors
// the original, including its horizontal right-edge comparison against 319.
void blit_transparent_original(
    IndexedFramebuffer& framebuffer,
    const IndexedSpriteFrame& frame,
    std::int32_t x,
    std::int32_t y);

} // namespace drone::fidelity
