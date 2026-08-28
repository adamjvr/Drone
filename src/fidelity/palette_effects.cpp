#include <drone/fidelity/palette_effects.hpp>

#include <algorithm>
#include <stdexcept>

namespace drone::fidelity {
namespace {

std::int32_t positive_remainder(const RandomIntSource& random_int, std::int32_t divisor) {
    if (!random_int) {
        throw std::invalid_argument("palette effect initialization requires a random source");
    }
    const std::int32_t value = random_int();
    if (value < 0) {
        throw std::invalid_argument("palette random source must be nonnegative like CRT rand()");
    }
    return value % divisor;
}

std::uint8_t as_palette_byte(std::int32_t value) noexcept {
    return static_cast<std::uint8_t>(value);
}

void set_rgb(WorkingPalette& palette, std::size_t index,
             std::int32_t r, std::int32_t g, std::int32_t b) noexcept {
    palette[index] = {
        as_palette_byte(r),
        as_palette_byte(g),
        as_palette_byte(b),
    };
}

} // namespace

std::int32_t gameplay_palette_fade_subtract(const std::int32_t counter) noexcept {
    if (counter < 0) return 255;
    if (counter > gameplay_palette_fade_last_render_counter) return 0;
    // Win32 0x00410EAA..0x00410EC1 uses x87 and the helper at 0x00421E90,
    // which temporarily sets round-toward-zero before converting to integer.
    const double subtract = 255.0 - static_cast<double>(counter) * 4.19;
    return static_cast<std::int32_t>(subtract);
}

bool apply_gameplay_palette_fade_from_base(
    const WorkingPalette& base_palette,
    WorkingPalette& working_palette,
    const std::int32_t counter) noexcept {
    if (counter > gameplay_palette_fade_last_render_counter) return false;
    const std::int32_t subtract = gameplay_palette_fade_subtract(counter);
    for (std::size_t i = 0; i < working_palette.size(); ++i) {
        working_palette[i] = {
            as_palette_byte(std::max(0, static_cast<std::int32_t>(base_palette[i].r) - subtract)),
            as_palette_byte(std::max(0, static_cast<std::int32_t>(base_palette[i].g) - subtract)),
            as_palette_byte(std::max(0, static_cast<std::int32_t>(base_palette[i].b) - subtract)),
        };
    }
    return true;
}

void advance_gameplay_palette_fade_counter(
    std::int32_t& counter,
    const std::uint8_t gameplay_phase) noexcept {
    if (gameplay_phase == 2 && counter < gameplay_palette_fade_settled_counter) {
        ++counter;
    }
}

void initialize_gameplay_palette_effect_bands(
    WorkingPalette& palette,
    GameplayPaletteEffectState& state,
    const RandomIntSource& random_int) {

    // 0x0041EFE0: sparse white flash trio, palette 110..112.
    for (std::size_t slot = 0; slot < state.flash_phase.size(); ++slot) {
        set_rgb(palette, 110 + slot, 1, 1, 1);
        state.flash_phase[slot] = positive_remainder(random_int, 20);
    }

    // Red breathing band, palette 96..102. The original consumes rand()%255
    // for every entry even though 100..102 are replaced by fixed red values.
    std::int32_t step = 4;
    for (std::size_t slot = 0; slot < state.red_step.size(); ++slot) {
        const std::size_t index = 96 + slot;
        std::int32_t red = positive_remainder(random_int, 255);
        if (index == 100) red = 93;
        if (index == 101) red = 243;
        if (index == 102) red = 162;
        set_rgb(palette, index, red, 24, 24);
        state.red_step[slot] = step;
        step = -step;
    }

    // Slow yellow/olive blink band, palette 128..148.
    for (std::size_t slot = 0; slot < state.yellow.size(); ++slot) {
        set_rgb(palette, 128 + slot, 182, 182, 57);
        auto& entry = state.yellow[slot];
        entry.toggle = positive_remainder(random_int, 2) != 0;
        entry.timer = 0;
        entry.period = positive_remainder(random_int, 240) + 162;
    }

    // Slow green blink band, palette 103..109.
    for (std::size_t slot = 0; slot < state.green.size(); ++slot) {
        set_rgb(palette, 103 + slot, 162, 214, 97);
        auto& entry = state.green[slot];
        entry.toggle = positive_remainder(random_int, 2) != 0;
        entry.timer = 0;
        entry.period = positive_remainder(random_int, 240) + 200;
    }
}

void advance_gameplay_palette_effect_bands(
    WorkingPalette& palette,
    GameplayPaletteEffectState& state) noexcept {

    // 0x0041EE90: palette 110..112. All three are dark (1,1,1) unless
    // their phase hits an established sparse white-flash point.
    for (std::size_t slot = 0; slot < state.flash_phase.size(); ++slot) {
        auto& phase = state.flash_phase[slot];
        ++phase;
        if (phase == 65) phase = 0;
        bool white = phase == 32;
        if ((slot == 0 || slot == 2) && phase == 0) white = true;
        set_rgb(palette, 110 + slot, white ? 255 : 1, white ? 255 : 1, white ? 255 : 1);
    }

    // Palette 96..102: red-channel breathing with exact 6/255 clamps and
    // +/-4 direction reversals.
    for (std::size_t slot = 0; slot < state.red_step.size(); ++slot) {
        const std::size_t index = 96 + slot;
        std::int32_t red = static_cast<std::int32_t>(palette[index].r) + state.red_step[slot];
        if (red > 255) {
            red = 255;
            state.red_step[slot] = -4;
        } else if (red < 6) {
            red = 6;
            state.red_step[slot] = 4;
        }
        palette[index].r = as_palette_byte(red);
    }

    // Palette 128..148: timer-driven yellow/olive <-> dark switches.
    for (std::size_t slot = 0; slot < state.yellow.size(); ++slot) {
        auto& entry = state.yellow[slot];
        ++entry.timer;
        if (entry.timer != entry.period) continue;
        entry.timer = 0;
        if (!entry.toggle) {
            set_rgb(palette, 128 + slot, 182, 182, 57);
            entry.toggle = true;
        } else {
            set_rgb(palette, 128 + slot, 12, 12, 12);
            entry.toggle = false;
        }
    }

    // Palette 103..109: timer-driven green <-> dark switches.
    for (std::size_t slot = 0; slot < state.green.size(); ++slot) {
        auto& entry = state.green[slot];
        ++entry.timer;
        if (entry.timer != entry.period) continue;
        entry.timer = 0;
        if (!entry.toggle) {
            set_rgb(palette, 103 + slot, 40, 215, 97);
            entry.toggle = true;
        } else {
            set_rgb(palette, 103 + slot, 12, 12, 12);
            entry.toggle = false;
        }
    }
}

void advance_generic_gameplay_palette_animation(
    WorkingPalette& palette,
    GenericPaletteAnimationControls& controls,
    const RandomIntSource& random_int) {

    for (std::size_t index = 64; index < 234; ++index) {
        if (index == 171) index = 192;
        if (index == 214) index = 224;

        auto& control = controls[index];
        if (!control.active) {
            if (positive_remainder(random_int, 100) < 2) {
                control.active = true;
            }
            continue;
        }

        std::int32_t red = static_cast<std::int32_t>(palette[index].r) + control.red_step;
        std::int32_t green = static_cast<std::int32_t>(palette[index].g) + control.green_step;
        std::int32_t blue = static_cast<std::int32_t>(palette[index].b) + control.blue_step;

        red = std::max(red, 0);
        green = std::max(green, 0);
        blue = std::max(blue, 0);
        set_rgb(palette, index, red, green, blue);

        if (blue >= control.upper_blue) {
            control.red_step = -1;
            control.green_step = -1;
            control.blue_step = -1;
        } else if (blue <= control.lower_blue) {
            control.red_step = 1;
            control.green_step = 1;
            control.blue_step = 1;
        } else if (blue == control.stop_blue) {
            control.active = false;
        }
    }
}

std::vector<PaletteUploadRange> gameplay_palette_upload_ranges(
    std::uint8_t gameplay_phase,
    bool settled_for_sliced_uploads) {

    if (!settled_for_sliced_uploads) {
        return {{0, 255}};
    }
    if (gameplay_phase > 3) {
        throw std::out_of_range("gameplay palette upload phase must be 0..3");
    }

    switch (gameplay_phase) {
        case 0:
            return {{32, 42}, {64, 110}};
        case 1:
            return {{111, 156}};
        case 2:
        case 3:
            return {{157, 170}, {192, 213}, {224, 234}};
    }
    return {};
}

} // namespace drone::fidelity
