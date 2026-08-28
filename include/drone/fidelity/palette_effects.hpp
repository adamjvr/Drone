#pragma once

#include <drone/formats/jba.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace drone::fidelity {

using WorkingPalette = std::array<formats::Rgb8, 256>;
using RandomIntSource = std::function<std::int32_t()>;

// Inclusive palette-index interval consumed by the original DirectDraw
// SetEntries wrapper at Win32 0x004011E0.
struct PaletteUploadRange {
    std::uint16_t first{};
    std::uint16_t last{};

    friend constexpr bool operator==(const PaletteUploadRange&, const PaletteUploadRange&) = default;
};

// State owned by the four purpose-built gameplay palette bands initialized by
// Win32 0x0041EFE0 and advanced by 0x0041EE90. The clean representation is
// semantic and intentionally does not mirror the original 0x44-byte records.
struct TimedPaletteBandEntry {
    bool toggle{};
    std::int32_t timer{};
    std::int32_t period{};
};

struct GameplayPaletteEffectState {
    std::array<std::int32_t, 3> flash_phase{};       // palette 110..112
    std::array<std::int32_t, 7> red_step{};          // palette 96..102
    std::array<TimedPaletteBandEntry, 21> yellow{};  // palette 128..148
    std::array<TimedPaletteBandEntry, 7> green{};    // palette 103..109
};

// Initializes only the four recovered dynamic bands, leaving all unrelated
// palette entries untouched. random_int must reproduce the original nonnegative
// CRT rand()-style source if bit-exact initialization is required.
void initialize_gameplay_palette_effect_bands(
    WorkingPalette& palette,
    GameplayPaletteEffectState& state,
    const RandomIntSource& random_int);

void advance_gameplay_palette_effect_bands(
    WorkingPalette& palette,
    GameplayPaletteEffectState& state) noexcept;

// Generic late-game palette animator recovered at Win32 0x00403490. Current
// colors are kept separately from control metadata instead of reproducing the
// historical packed record layout.
struct GenericPaletteAnimationControl {
    bool active{};
    std::int32_t stop_blue{};
    std::int32_t red_step{};
    std::int32_t green_step{};
    std::int32_t blue_step{};
    std::int32_t upper_blue{};
    std::int32_t lower_blue{};
};

using GenericPaletteAnimationControls = std::array<GenericPaletteAnimationControl, 256>;

// Advances exactly the palette indices visited by 0x00403490:
// 64..170, 192..213, 224..233. Inactive records consume one random value and
// become active when rand()%100 < 2.
void advance_generic_gameplay_palette_animation(
    WorkingPalette& palette,
    GenericPaletteAnimationControls& controls,
    const RandomIntSource& random_int);

[[nodiscard]] constexpr bool generic_palette_animation_index(std::size_t index) noexcept {
    return (index >= 64 && index <= 170) ||
           (index >= 192 && index <= 213) ||
           (index >= 224 && index <= 233);
}

// Once the original state-2 palette path is fully settled it distributes
// DirectDraw palette uploads across the four gameplay phases. During fades,
// destruction, first-objective setup, or other unsettled conditions the caller
// requests the original full-range fallback instead.
[[nodiscard]] std::vector<PaletteUploadRange> gameplay_palette_upload_ranges(
    std::uint8_t gameplay_phase,
    bool settled_for_sliced_uploads);

} // namespace drone::fidelity
