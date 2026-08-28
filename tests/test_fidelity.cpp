#include <drone/fidelity/palette_effects.hpp>
#include <drone/fidelity/presentation_order.hpp>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

struct SequenceRandom {
    std::vector<std::int32_t> values;
    std::size_t cursor{};

    std::int32_t next() {
        assert(cursor < values.size());
        return values[cursor++];
    }
};

} // namespace

int main() {
    using namespace drone::fidelity;

    {
        WorkingPalette palette{};
        GameplayPaletteEffectState state{};
        SequenceRandom rng;
        // 3 flash phases; 7 red seeds; 21*(toggle,period); 7*(toggle,period).
        for (int i = 0; i < 66; ++i) rng.values.push_back(i);
        initialize_gameplay_palette_effect_bands(
            palette, state, [&rng] { return rng.next(); });
        assert(rng.cursor == 66);

        assert(palette[110].r == 1 && palette[110].g == 1 && palette[110].b == 1);
        assert(state.flash_phase[0] == 0);
        assert(state.flash_phase[1] == 1);
        assert(state.flash_phase[2] == 2);

        // Random red seeds begin at raw value 3, except for the original fixed
        // overrides at palette entries 100..102.
        assert(palette[96].r == 3 && palette[96].g == 24 && palette[96].b == 24);
        assert(palette[99].r == 6);
        assert(palette[100].r == 93);
        assert(palette[101].r == 243);
        assert(palette[102].r == 162);
        assert(state.red_step[0] == 4);
        assert(state.red_step[1] == -4);
        assert(state.red_step[6] == 4);

        assert(palette[128].r == 182 && palette[128].g == 182 && palette[128].b == 57);
        assert(state.yellow[0].toggle == false); // raw 10 % 2
        assert(state.yellow[0].timer == 0);
        assert(state.yellow[0].period == 173); // raw 11 % 240 + 162
        assert(palette[103].r == 162 && palette[103].g == 214 && palette[103].b == 97);
        assert(state.green[0].toggle == false); // raw 52 % 2
        assert(state.green[0].period == 253);   // raw 53 % 240 + 200
    }

    {
        WorkingPalette palette{};
        GameplayPaletteEffectState state{};

        // Flash points: endpoints whiten at phases 0 and 32; center only at 32.
        state.flash_phase = {64, 31, 64};

        palette[96] = {254, 24, 24};
        state.red_step[0] = 4;
        palette[97] = {7, 24, 24};
        state.red_step[1] = -4;

        state.yellow[0] = {.toggle = true, .timer = 1, .period = 2};
        state.yellow[1] = {.toggle = false, .timer = 1, .period = 2};
        state.green[0] = {.toggle = true, .timer = 4, .period = 5};
        state.green[1] = {.toggle = false, .timer = 4, .period = 5};

        advance_gameplay_palette_effect_bands(palette, state);

        assert(state.flash_phase[0] == 0 && palette[110].r == 255);
        assert(state.flash_phase[1] == 32 && palette[111].r == 255);
        assert(state.flash_phase[2] == 0 && palette[112].r == 255);

        assert(palette[96].r == 255 && state.red_step[0] == -4);
        assert(palette[97].r == 6 && state.red_step[1] == 4);

        assert(!state.yellow[0].toggle && palette[128].r == 12 && palette[128].b == 12);
        assert(state.yellow[1].toggle && palette[129].r == 182 && palette[129].b == 57);
        assert(!state.green[0].toggle && palette[103].r == 12 && palette[103].g == 12);
        assert(state.green[1].toggle && palette[104].r == 40 && palette[104].g == 215);
    }

    {
        WorkingPalette palette{};
        GenericPaletteAnimationControls controls{};

        palette[64] = {10, 20, 30};
        controls[64] = {
            .active = true,
            .stop_blue = 29,
            .red_step = -20,
            .green_step = 1,
            .blue_step = -1,
            .upper_blue = 40,
            .lower_blue = 10,
        };

        palette[65] = {10, 10, 10};
        controls[65] = {
            .active = true,
            .stop_blue = 11,
            .red_step = 1,
            .green_step = 1,
            .blue_step = 1,
            .upper_blue = 11,
            .lower_blue = 2,
        };

        palette[66] = {10, 10, 10};
        controls[66] = {
            .active = true,
            .stop_blue = 9,
            .red_step = -1,
            .green_step = -1,
            .blue_step = -1,
            .upper_blue = 40,
            .lower_blue = 9,
        };

        palette[67] = {10, 10, 20};
        controls[67] = {
            .active = true,
            .stop_blue = 21,
            .red_step = 1,
            .green_step = 1,
            .blue_step = 1,
            .upper_blue = 40,
            .lower_blue = 2,
        };

        // All other eligible records are inactive. Only the first inactive
        // one gets a 1% activation roll; all later rolls are non-triggering.
        int inactive_roll = 0;
        auto random = [&inactive_roll]() -> std::int32_t {
            return inactive_roll++ == 0 ? 1 : 99;
        };
        advance_generic_gameplay_palette_animation(palette, controls, random);

        // Negative channels clamp at zero. B=29 matches stop value only after
        // upper/lower checks, so this record becomes inactive.
        assert(palette[64].r == 0 && palette[64].g == 21 && palette[64].b == 29);
        assert(!controls[64].active);

        // Upper/lower bounds take priority over stop-value equality.
        assert(controls[65].red_step == -1 && controls[65].blue_step == -1);
        assert(controls[66].red_step == 1 && controls[66].blue_step == 1);
        assert(controls[67].active == false); // blue reaches stop 21

        // Index 68 is the first inactive eligible slot and activates on roll 1.
        assert(controls[68].active);
        // The skipped holes and terminal 234 are not visited by 0x00403490.
        assert(!controls[171].active && !controls[214].active && !controls[234].active);
        assert(generic_palette_animation_index(170));
        assert(!generic_palette_animation_index(171));
        assert(generic_palette_animation_index(192));
        assert(!generic_palette_animation_index(214));
        assert(generic_palette_animation_index(233));
        assert(!generic_palette_animation_index(234));
    }

    {
        assert((gameplay_palette_upload_ranges(0, true) ==
                std::vector<PaletteUploadRange>{{32, 42}, {64, 110}}));
        assert((gameplay_palette_upload_ranges(1, true) ==
                std::vector<PaletteUploadRange>{{111, 156}}));
        assert((gameplay_palette_upload_ranges(2, true) ==
                std::vector<PaletteUploadRange>{{157, 170}, {192, 213}, {224, 234}}));
        assert((gameplay_palette_upload_ranges(3, true) ==
                std::vector<PaletteUploadRange>{{157, 170}, {192, 213}, {224, 234}}));
        assert((gameplay_palette_upload_ranges(0, false) ==
                std::vector<PaletteUploadRange>{{0, 255}}));

        bool rejected = false;
        try {
            (void)gameplay_palette_upload_ranges(4, true);
        } catch (const std::out_of_range&) {
            rejected = true;
        }
        assert(rejected);
    }

    {
        const auto& order = canonical_win32_gameplay_presentation_order();
        assert(order.size() == canonical_win32_presentation_pass_count);
        assert(order.front().pass == GameplayPresentationPass::ComposeWorldViewport);
        assert(order.front().evidence_start == 0x004100D8u);
        assert(order.back().pass == GameplayPresentationPass::PresentFramebuffer);
        assert(order.back().evidence_start == 0x004115A5u);

        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::TransparentSpriteBatchBeforeDebris,
            GameplayPresentationPass::DebrisParticlePixels));
        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::DebrisParticlePixels,
            GameplayPresentationPass::DroneDetonationRadialNoise));
        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::DroneDetonationRadialNoise,
            GameplayPresentationPass::ScaledTransparentOverlays));
        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::ScaledTransparentOverlays,
            GameplayPresentationPass::HudScoreAndLivesText));
        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::ShieldMeter,
            GameplayPresentationPass::PlayerShieldOverlay));
        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::SpecialWeaponStatusText,
            GameplayPresentationPass::PaletteAnimation));
        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::HostPacing,
            GameplayPresentationPass::PaletteUpload));
        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::PaletteUpload,
            GameplayPresentationPass::PresentFramebuffer));

        assert(order[0].domain == GameplayPresentationDomain::IndexedFramebuffer);
        assert(order[14].domain == GameplayPresentationDomain::WorkingPalette);
        assert(order[15].domain == GameplayPresentationDomain::Host);
        assert(!order[0].conditional);
        assert(order[1].conditional);
        assert(!order[11].conditional); // shield meter is always invoked
        assert(!order[16].conditional); // some upload range is always emitted
        assert(!order[17].conditional);
    }

    std::cout << "fidelity tests passed\n";
    return 0;
}
