#include <drone/fidelity/framebuffer_snapshot.hpp>
#include <drone/fidelity/host_capture.hpp>
#include <drone/fidelity/hud_presentation.hpp>
#include <drone/fidelity/palette_effects.hpp>
#include <drone/fidelity/presentation_order.hpp>
#include <drone/fidelity/scaled_overlay_presentation.hpp>
#include <drone/fidelity/world_presentation_subpasses.hpp>

#include <cassert>
#include <cstdint>
#include <filesystem>
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
        WorkingPalette base{};
        WorkingPalette working{};
        base[0] = {255, 128, 10};
        base[255] = {7, 6, 5};

        assert(gameplay_palette_fade_subtract(0) == 255);
        assert(gameplay_palette_fade_subtract(1) == 250);
        assert(gameplay_palette_fade_subtract(60) == 3);
        assert(gameplay_palette_fade_subtract(61) == 0);

        assert(apply_gameplay_palette_fade_from_base(base, working, 0));
        assert(working[0].r == 0 && working[0].g == 0 && working[0].b == 0);
        assert(apply_gameplay_palette_fade_from_base(base, working, 60));
        assert(working[0].r == 252 && working[0].g == 125 && working[0].b == 7);
        assert(working[255].r == 4 && working[255].g == 3 && working[255].b == 2);
        const auto before0 = working[0];
        const auto before255 = working[255];
        assert(!apply_gameplay_palette_fade_from_base(base, working, 61));
        assert(working[0].r == before0.r && working[0].g == before0.g && working[0].b == before0.b);
        assert(working[255].r == before255.r && working[255].g == before255.g && working[255].b == before255.b);

        std::int32_t counter = 60;
        advance_gameplay_palette_fade_counter(counter, 1);
        assert(counter == 60);
        advance_gameplay_palette_fade_counter(counter, 2);
        assert(counter == 61);
        advance_gameplay_palette_fade_counter(counter, 2);
        assert(counter == 62);
        advance_gameplay_palette_fade_counter(counter, 2);
        assert(counter == 62);
    }

    {
        assert(score_text_placement(0).x == 309);
        assert(score_text_placement(9).x == 309);
        assert(score_text_placement(10).x == 301);
        assert(score_text_placement(99).x == 301);
        assert(score_text_placement(100).x == 293);
        assert(score_text_placement(999).x == 293);
        assert(score_text_placement(1000).x == 285);
        assert(score_text_placement(9998).x == 285);
        assert(score_text_placement(42).y == 190);
        assert(score_text_placement(42).palette_index == 28);
        assert(lives_text_placement().x == 309);
        assert(lives_text_placement().y == 180);

        const auto markers = plan_drone_outcome_markers({0, 1, 2, 3, 0, 1});
        assert(!markers[0].visible && markers[0].x == 3 && markers[0].y == 160);
        assert(markers[1].visible && markers[1].frame_index == 0 && markers[1].y == 141);
        assert(markers[2].visible && markers[2].frame_index == 1 && markers[2].y == 122);
        assert(markers[3].visible && markers[3].frame_index == 2 && markers[3].y == 103);
        assert(!markers[4].visible && markers[4].y == 84);
        assert(markers[5].visible && markers[5].frame_index == 0 && markers[5].y == 65);

        assert(drone_outcome_cursor_target_y(0) == 159);
        assert(drone_outcome_cursor_target_y(1) == 140);
        assert(drone_outcome_cursor_target_y(5) == 64);
        assert(drone_outcome_cursor_target_y(6) == 45);
        assert(drone_outcome_cursor_target_y(9) == 45);
        auto cursor = plan_drone_outcome_cursor(2, 121);
        assert(cursor.visible && cursor.x == 2 && cursor.y == 121 && cursor.target_y == 121);
        cursor = plan_drone_outcome_cursor(6, 64);
        assert(!cursor.visible && cursor.target_y == 45);
        std::int32_t cursor_y = 159;
        advance_drone_outcome_cursor_y(cursor_y, 140, 1);
        assert(cursor_y == 159);
        advance_drone_outcome_cursor_y(cursor_y, 140, 2);
        assert(cursor_y == 158);
        cursor_y = 140;
        advance_drone_outcome_cursor_y(cursor_y, 140, 2);
        assert(cursor_y == 140);

        SpecialWeaponHudTimers timers{};
        auto miss = plan_special_weapon_status({.activity_state = 0}, timers);
        assert(miss.visible && miss.status == SpecialWeaponHudStatus::Miss && miss.text == "MISS");
        assert(miss.next_timers.miss_hold == 1);
        timers.miss_hold = 110;
        assert(!plan_special_weapon_status({.activity_state = 0}, timers).visible);

        auto ready = plan_special_weapon_status({.activity_state = 1}, {});
        assert(ready.visible && ready.text == "READY" && ready.placement.x == 5 && ready.placement.y == 190);
        auto seeking = plan_special_weapon_status({.activity_state = 3}, {});
        assert(seeking.visible && seeking.text == "SEEKING");

        SpecialWeaponHudInputs decode{
            .activity_state = 2,
            .decode_phase1_elapsed = 2,
            .decode_phase1_threshold = 10,
            .decode_phase2_elapsed = 0,
            .decode_phase2_threshold = 10,
        };
        auto decoding = plan_special_weapon_status(decode, {});
        assert(decoding.visible && decoding.text == "DECODING");
        decode.decode_phase1_elapsed = 10;
        decode.decode_phase2_elapsed = 2;
        auto disarming = plan_special_weapon_status(decode, {.miss_hold = 0, .disarmed_hold = 77});
        assert(disarming.visible && disarming.text == "DISARMING");
        assert(disarming.next_timers.disarmed_hold == 1);
        decode.decode_phase2_elapsed = 10;
        auto disarmed = plan_special_weapon_status(decode, {.miss_hold = 0, .disarmed_hold = 1});
        assert(disarmed.visible && disarmed.text == "DISARMED!");
        assert(disarmed.placement.palette_index == 57);
        assert(disarmed.next_timers.disarmed_hold == 2);
        assert(!plan_special_weapon_status(decode, {.miss_hold = 0, .disarmed_hold = 200}).visible);

        assert((special_target_reticle_placement({100, 80, 20, 10}) == ReticlePlacement{102, 77}));
        assert((special_target_reticle_placement({-50, -50, 10, 10}) == ReticlePlacement{0, 0}));
        assert((special_target_reticle_placement({315, 195, 20, 20}) == ReticlePlacement{301, 185}));

        const auto shield = plan_shield_meter_rows(75);
        assert(shield[0].x == 313 && shield[0].y == 138 && shield[0].width == 4 && shield[0].palette_index == 27);
        assert(shield[10].y == 128 && shield[10].palette_index == 27);
        assert(shield[11].y == 127 && shield[11].palette_index == 57);
        assert(shield[25].palette_index == 57);
        assert(shield[26].palette_index == 28);
        assert(shield[74].y == 64 && shield[74].palette_index == 28);
        const auto empty_shield = plan_shield_meter_rows(-5);
        assert(empty_shield[0].width == 0);
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
            GameplayPresentationPass::GameplayPaletteFadeIn));
        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::GameplayPaletteFadeIn,
            GameplayPresentationPass::HudScoreAndLivesText));
        assert(canonical_win32_gameplay_presentation_precedes(
            GameplayPresentationPass::DroneOutcomeStrip,
            GameplayPresentationPass::DroneOutcomeCursor));
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
        assert(order[7].domain == GameplayPresentationDomain::WorkingPalette);
        assert(order[15].domain == GameplayPresentationDomain::WorkingPalette);
        assert(order[16].domain == GameplayPresentationDomain::Host);
        assert(!order[0].conditional);
        assert(order[1].conditional);
        assert(order[7].conditional); // startup fade only while counter <= 60
        assert(!order[12].conditional); // shield meter is always invoked
        assert(!order[17].conditional); // some upload range is always emitted
        assert(!order[18].conditional);
    }


    {
        const auto& scaled = canonical_win32_scaled_overlay_subpasses();
        assert(scaled.size() == canonical_win32_scaled_overlay_subpass_count);
        assert(scaled[0].subpass == ScaledOverlaySubpass::MiniExplosionScaledPool);
        assert(scaled[0].entity_root == 0x00480318u && scaled[0].fixed_element_count == 110);
        assert(scaled[1].entity_root == 0x00446FC8u && scaled[1].fixed_element_count == 165);
        assert(scaled[0].requires_active_state && scaled[0].requires_scaled_family_flag);
        assert(scaled[2].requires_objective_debris_flag);
        assert(effect_entity_uses_scaled_render_route(1, 1));
        assert(!effect_entity_uses_scaled_render_route(0, 1));
        assert(!effect_entity_uses_scaled_render_route(1, 0));

        const auto& debris = objective_scaled_debris_descriptors();
        assert(debris.size() == 3);
        assert(debris[0].entity_root == 0x00441928u && debris[0].source_width == 25 && debris[0].source_height == 18);
        assert(debris[0].frame_count == 8 && debris[0].initial_velocity_x == -3 && debris[0].initial_velocity_y == 4);
        assert(debris[1].entity_root == 0x004417D0u && debris[1].frame_count == 16);
        assert(debris[1].initial_velocity_x == -5 && debris[1].initial_velocity_y == -1);
        assert(debris[2].entity_root == 0x00441AC8u && debris[2].source_width == 26 && debris[2].source_height == 20);
        assert(debris[2].initial_velocity_x == 3 && debris[2].initial_velocity_y == 1);

        ScaledOverlayGeometry geometry{100, 80, 25, 18};
        advance_objective_scaled_debris_growth(geometry, 1);
        assert((geometry == ScaledOverlayGeometry{100, 80, 25, 18}));
        advance_objective_scaled_debris_growth(geometry, 2);
        assert((geometry == ScaledOverlayGeometry{99, 79, 27, 20}));
        assert((objective_scaled_debris_destination(geometry) ==
                ScaledOverlayDestination{99, 79, 126, 99}));
        assert(objective_scaled_debris_visible(geometry));
        assert(!objective_scaled_debris_visible({-27, 0, 27, 20}));
        assert(objective_scaled_debris_visible({-26, 0, 27, 20}));
        assert(!objective_scaled_debris_visible({0, -20, 27, 20}));
        assert(objective_scaled_debris_visible({0, -19, 27, 20}));
        assert(!objective_scaled_debris_visible({319, 0, 27, 20}));
        assert(!objective_scaled_debris_visible({0, 199, 27, 20}));
    }

    {
        const auto& subpasses = canonical_win32_world_presentation_subpasses();
        assert(subpasses.size() == canonical_win32_world_presentation_subpass_count);
        assert(subpasses.front().subpass == WorldPresentationSubpass::MothershipComposite);
        assert(subpasses.back().subpass == WorldPresentationSubpass::SecondaryImpactSpritePool);

        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::MothershipComposite,
            WorldPresentationSubpass::PointParticleBank));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::SpecialProjectile,
            WorldPresentationSubpass::GeminiProceduralBeam));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::GeminiProceduralBeam,
            WorldPresentationSubpass::GeminiBodyHeadA));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::MiniExplosionUnscaled,
            WorldPresentationSubpass::DebrisParticlePixels));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::DebrisParticlePixels,
            WorldPresentationSubpass::SpriteDebrisTriplet));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::SpriteDebrisTriplet,
            WorldPresentationSubpass::DroneDetonationRadialNoise));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::RetroSpriteA,
            WorldPresentationSubpass::TrajectoryGroups));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::TrajectoryGroups,
            WorldPresentationSubpass::RapidMissilePool));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::RapidMissilePool,
            WorldPresentationSubpass::EnemyBombPool));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::EnemyBombPool,
            WorldPresentationSubpass::Player));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::Player,
            WorldPresentationSubpass::PlayerDestructionExplosion));
        assert(canonical_win32_world_presentation_precedes(
            WorldPresentationSubpass::PlayerDestructionExplosion,
            WorldPresentationSubpass::SecondaryImpactSpritePool));

        const auto* particles = canonical_win32_world_presentation_descriptor(
            WorldPresentationSubpass::PointParticleBank);
        assert(particles != nullptr);
        assert(particles->primitive == WorldPresentationPrimitive::FixedPointPixelParticles);
        assert(particles->primary_root == 0x00434D80u);
        assert(particles->fixed_element_count == 650);

        const auto* debris = canonical_win32_world_presentation_descriptor(
            WorldPresentationSubpass::SpriteDebrisTriplet);
        assert(debris != nullptr);
        assert(debris->primary_root == 0x0042FCA0u);
        assert(debris->fixed_element_count == 15);

        const auto* flare = canonical_win32_world_presentation_descriptor(
            WorldPresentationSubpass::Flare);
        const auto* chute = canonical_win32_world_presentation_descriptor(
            WorldPresentationSubpass::Chute);
        const auto* stinger = canonical_win32_world_presentation_descriptor(
            WorldPresentationSubpass::StingerEntity);
        assert(flare && flare->primary_root == 0x00440E00u);
        assert(chute && chute->primary_root == 0x0045BDA8u);
        assert(stinger && stinger->primary_root == 0x00434C10u);

        const auto* retro_a = canonical_win32_world_presentation_descriptor(
            WorldPresentationSubpass::RetroSpriteA);
        const auto* retro_b = canonical_win32_world_presentation_descriptor(
            WorldPresentationSubpass::RetroSpriteB);
        assert(retro_a && retro_a->primary_root == 0x004673E0u);
        assert(retro_b && retro_b->primary_root == 0x00438C80u);

        const auto* destruction = canonical_win32_world_presentation_descriptor(
            WorldPresentationSubpass::PlayerDestructionExplosion);
        assert(destruction && destruction->primary_root == 0x00491CE0u);
        const auto* secondary = canonical_win32_world_presentation_descriptor(
            WorldPresentationSubpass::SecondaryImpactSpritePool);
        assert(secondary && secondary->primary_root == 0x004605A0u);
        assert(secondary->fixed_element_count == 15);
    }


    {
        IndexedFramebuffer framebuffer;
        for (std::size_t i = 0; i < framebuffer.pixels().size(); ++i) {
            framebuffer.pixels()[i] = static_cast<std::uint8_t>(i & 0xffu);
        }
        for (std::size_t i = 0; i < framebuffer.palette().size(); ++i) {
            framebuffer.palette()[i] = {
                static_cast<std::uint8_t>(i),
                static_cast<std::uint8_t>(255u - i),
                static_cast<std::uint8_t>((i * 5u) & 0xffu),
            };
        }
        const auto before = make_framebuffer_snapshot(framebuffer);
        const auto root = std::filesystem::temp_directory_path() / "drone-host-capture-test";
        std::filesystem::remove_all(root);
        const FidelityHostCaptureLandmark landmark{.label = "Boss / Arrival #1", .sequence = 42};
        assert(sanitize_fidelity_capture_label(landmark.label) == "boss-arrival-1");
        const auto expected = root / "00000042-boss-arrival-1.drfb";
        assert(fidelity_capture_landmark_path(root, landmark) == expected);
        const auto written = write_fidelity_host_landmark_capture(framebuffer, root, landmark);
        assert(written == expected);
        assert(std::filesystem::exists(written));
        const auto captured = load_framebuffer_snapshot(written);
        assert(compare_framebuffer_snapshots(before, captured).exact());
        const auto after = make_framebuffer_snapshot(framebuffer);
        assert(compare_framebuffer_snapshots(before, after).exact());
        std::filesystem::remove_all(root);
    }

    {
        FramebufferSnapshot reference;
        reference.pixels.resize(IndexedFramebuffer::pixel_count);
        for (std::size_t y = 0; y < IndexedFramebuffer::height; ++y) {
            for (std::size_t x = 0; x < IndexedFramebuffer::width; ++x) {
                reference.pixels[y * IndexedFramebuffer::width + x] =
                    static_cast<std::uint8_t>((x + y) & 0xffu);
            }
        }
        for (std::size_t i = 0; i < reference.palette.size(); ++i) {
            reference.palette[i] = {
                static_cast<std::uint8_t>(i),
                static_cast<std::uint8_t>((i * 3u) & 0xffu),
                static_cast<std::uint8_t>(255u - i),
            };
        }

        const auto path = std::filesystem::temp_directory_path() / "drone-test-framebuffer.drfb";
        write_framebuffer_snapshot(reference, path);
        assert(std::filesystem::file_size(path) == framebuffer_snapshot_file_size);
        const auto loaded = load_framebuffer_snapshot(path);
        std::filesystem::remove(path);

        auto exact = compare_framebuffer_snapshots(reference, loaded);
        assert(exact.exact());
        assert(exact.pixel_mismatch_count == 0);
        assert(exact.rendered_rgb_mismatch_count == 0);
        assert(exact.palette_entry_mismatch_count == 0);
        assert(!exact.pixel_mismatch_bounds);

        auto candidate = loaded;
        candidate.pixels[10 * IndexedFramebuffer::width + 20] ^= 7;
        candidate.pixels[12 * IndexedFramebuffer::width + 25] ^= 3;
        candidate.palette[7].r ^= 1;
        candidate.palette[7].g ^= 2;

        const auto mismatch = compare_framebuffer_snapshots(reference, candidate);
        assert(!mismatch.exact());
        assert(mismatch.pixel_mismatch_count == 2);
        assert(mismatch.palette_entry_mismatch_count == 1);
        assert(mismatch.palette_channel_mismatch_count == 2);
        assert(mismatch.pixel_mismatch_bounds.has_value());
        assert(mismatch.pixel_mismatch_bounds->x == 20);
        assert(mismatch.pixel_mismatch_bounds->y == 10);
        assert(mismatch.pixel_mismatch_bounds->width == 6);
        assert(mismatch.pixel_mismatch_bounds->height == 3);

        const auto clean_region = compare_framebuffer_snapshot_region(
            reference, candidate, FramebufferRect{100, 100, 4, 4});
        assert(clean_region.pixel_mismatch_count == 0);
        assert(clean_region.rendered_rgb_mismatch_count == 0);
        assert(clean_region.palette_channel_mismatch_count == 0);

        bool rejected = false;
        try {
            (void)compare_framebuffer_snapshot_region(
                reference, candidate, FramebufferRect{319, 199, 2, 1});
        } catch (const std::out_of_range&) {
            rejected = true;
        }
        assert(rejected);
    }

    std::cout << "fidelity tests passed\n";
    return 0;
}
