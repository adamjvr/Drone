#include <drone/formats/clv.hpp>
#include <drone/formats/demo.hpp>
#include <drone/formats/fly.hpp>
#include <drone/formats/jba.hpp>
#include <drone/fidelity/font2.hpp>
#include <drone/fidelity/indexed_framebuffer.hpp>
#include <drone/fidelity/sprite_sheet.hpp>
#include <drone/fidelity/sprite_blit.hpp>
#include <drone/gameplay/collision.hpp>
#include <drone/gameplay/demo_replay.hpp>
#include <drone/gameplay/enemy_bomb.hpp>
#include <drone/gameplay/player.hpp>
#include <drone/gameplay/rapid_missile.hpp>
#include <drone/gameplay/special_weapon.hpp>
#include <drone/gameplay/shield.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

int main() {
    const auto base = fs::temp_directory_path() / "drone_format_tests";
    fs::remove_all(base); fs::create_directories(base);

    // Synthetic JBA fixture encoded exactly as the recovered original loader expects.
    {
        std::vector<unsigned char> file;
        file.reserve(drone::formats::JbaImage::file_bytes);
        for (int i = 0; i < 256; ++i) {
            file.push_back(static_cast<unsigned char>(i & 63));
            file.push_back(static_cast<unsigned char>((i + 1) & 63));
            file.push_back(static_cast<unsigned char>((i + 2) & 63));
        }
        std::vector<unsigned char> expected(drone::formats::JbaImage::pixel_count);
        for (std::size_t i = 0; i < expected.size(); ++i) expected[i] = static_cast<unsigned char>((i * 17 + 3) & 255);
        for (std::size_t lane = 0; lane < 10; ++lane)
            for (std::size_t dst = lane; dst < expected.size(); dst += 10) file.push_back(expected[dst]);
        const auto path = base / "fixture.jba";
        std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(file.data()), file.size());
        const auto image = drone::formats::load_jba_320x200(path);
        assert(image.pixels == expected);
        assert(image.palette[1].r == 4 && image.palette[1].g == 8 && image.palette[1].b == 12);
    }

    {
        const auto path = base / "fixture.clv";
        const unsigned char samples[] = {0, 255, 10, 20, 100, 100};
        std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(samples), sizeof(samples));
        const auto clv = drone::formats::load_clv(path);
        assert(clv.frames() == 3);
        const auto mono = drone::formats::downmix_clv_to_mono(clv);
        assert((mono == std::vector<std::uint8_t>{127, 15, 100}));
    }

    {
        const auto path = base / "CURRENT.FLY";
        std::ofstream(path) << "2\n1\n2\n3\n-4\n5\n-6\n";
        const auto fly = drone::formats::load_counted_fly(path);
        assert(fly.size() == 2 && fly[1].x == -4 && fly[1].aux == -6);
    }

    {
        const auto path = base / "trajectory.fly";
        std::ofstream(path) << "-50 -15 40\n-49 -15 -1\n";
        const auto fly = drone::formats::load_raw_fly(path, 2);
        assert(fly.size() == 2 && fly[0].x == -50 && fly[0].y == -15 && fly[1].aux == -1);
        const auto short_read = drone::formats::load_raw_fly(path, 3, false);
        assert(short_read.size() == 2);
        assert(drone::formats::known_fly_asset("RIGHTDIV.FLY")->loader_record_count == 119);
        assert(drone::formats::known_fly_asset("RIGHTDIV.FLY")->physical_record_count == 118);
    }

    {
        // Win32 0x00401860 sprite-sheet extraction: one-pixel gutter around
        // each grid cell, with exact width x height row copies.
        drone::formats::JbaImage sheet;
        sheet.pixels.resize(drone::formats::JbaImage::pixel_count);
        for (std::size_t y = 0; y < drone::formats::JbaImage::height; ++y) {
            for (std::size_t x = 0; x < drone::formats::JbaImage::width; ++x) {
                sheet.pixels[y * drone::formats::JbaImage::width + x] =
                    static_cast<std::uint8_t>((x + 3 * y) & 0xff);
            }
        }
        const auto frame = drone::fidelity::extract_guttered_jba_frame(sheet, 4, 3, 2, 1);
        assert(frame.width == 4 && frame.height == 3 && frame.pixels.size() == 12);
        const std::size_t source_x = 2 * (4 + 1) + 1;
        const std::size_t source_y = 1 * (3 + 1) + 1;
        assert(frame.pixels[0] == sheet.pixels[source_y * 320 + source_x]);
        assert(frame.pixels[3] == sheet.pixels[source_y * 320 + source_x + 3]);
        assert(frame.pixels[4] == sheet.pixels[(source_y + 1) * 320 + source_x]);
        bool rejected = false;
        try {
            (void)drone::fidelity::extract_guttered_jba_frame(sheet, 100, 100, 4, 0);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        assert(rejected);
    }

    {
        // FONT2.JBA is a 16x4 table of 7x5 masks in 8x6 guttered cells.
        // Win32 and DOS both index the 64-entry cache by character - 0x20.
        assert(drone::fidelity::font2_glyph_index(0x20) == 0);
        assert(drone::fidelity::font2_glyph_index(0x2f) == 15);
        assert(drone::fidelity::font2_glyph_index(0x30) == 16);
        assert(drone::fidelity::font2_glyph_index(0x5f) == 63);
        assert(!drone::fidelity::font2_glyph_index(0x1f));
        assert(!drone::fidelity::font2_glyph_index(0x60));

        const auto first = drone::fidelity::font2_glyph_layout(0);
        assert(first.cell_x == 0 && first.cell_y == 0);
        assert(first.source_x == 1 && first.source_y == 1);
        assert(first.width == 7 && first.height == 5);

        const auto last_first_row = drone::fidelity::font2_glyph_layout(15);
        assert(last_first_row.source_x == 121 && last_first_row.source_y == 1);
        const auto first_second_row = drone::fidelity::font2_glyph_layout(16);
        assert(first_second_row.source_x == 1 && first_second_row.source_y == 7);
        const auto last = drone::fidelity::font2_glyph_layout(63);
        assert(last.source_x == 121 && last.source_y == 19);

        bool rejected = false;
        try {
            (void)drone::fidelity::font2_glyph_layout(64);
        } catch (const std::out_of_range&) {
            rejected = true;
        }
        assert(rejected);

        drone::formats::JbaImage sheet;
        sheet.pixels.resize(drone::formats::JbaImage::pixel_count);
        for (std::size_t y = 0; y < drone::formats::JbaImage::height; ++y) {
            for (std::size_t x = 0; x < drone::formats::JbaImage::width; ++x) {
                sheet.pixels[y * drone::formats::JbaImage::width + x] =
                    static_cast<std::uint8_t>((x + 17 * y) & 0xff);
            }
        }
        const auto glyph = drone::fidelity::extract_font2_glyph(sheet, 63);
        assert(glyph.width == 7 && glyph.height == 5 && glyph.pixels.size() == 35);
        assert(glyph.pixels.front() == sheet.pixels[19 * 320 + 121]);
        assert(glyph.pixels.back() == sheet.pixels[23 * 320 + 127]);
    }

    {
        drone::fidelity::IndexedFramebuffer fb;
        std::fill(fb.pixels().begin(), fb.pixels().end(), 5);
        drone::fidelity::IndexedSpriteFrame frame{3, 2, {0, 7, 8, 9, 0, 10}};
        drone::fidelity::blit_transparent_original(fb, frame, 1, 1);
        assert(fb.pixels()[1 * 320 + 1] == 5); // zero stays transparent
        assert(fb.pixels()[1 * 320 + 2] == 7);
        assert(fb.pixels()[1 * 320 + 3] == 8);
        assert(fb.pixels()[2 * 320 + 1] == 9);
        assert(fb.pixels()[2 * 320 + 2] == 5);
        assert(fb.pixels()[2 * 320 + 3] == 10);

        // Left/top clipping advances into the source frame.
        std::fill(fb.pixels().begin(), fb.pixels().end(), 5);
        drone::fidelity::blit_transparent_original(fb, frame, -1, -1);
        assert(fb.pixels()[0] == 5);  // source row 1, col 1 is transparent
        assert(fb.pixels()[1] == 10); // source row 1, col 2

        // Original Win32 routine clips against literal X=319 in a way that
        // leaves the final logical column untouched for this path.
        std::fill(fb.pixels().begin(), fb.pixels().end(), 5);
        drone::fidelity::IndexedSpriteFrame edge{2, 1, {11, 12}};
        drone::fidelity::blit_transparent_original(fb, edge, 318, 10);
        assert(fb.pixels()[10 * 320 + 318] == 11);
        assert(fb.pixels()[10 * 320 + 319] == 5);
    }

    {
        drone::formats::JbaImage image;
        image.pixels.assign(drone::formats::JbaImage::pixel_count, 1);
        image.palette[1] = {10, 20, 30};
        drone::fidelity::IndexedFramebuffer framebuffer;
        framebuffer.load(image);
        const auto rgba = framebuffer.rgba8();
        assert(rgba.size() == drone::fidelity::IndexedFramebuffer::pixel_count * 4);
        assert(rgba[0] == 10 && rgba[1] == 20 && rgba[2] == 30 && rgba[3] == 255);
    }


    {
        using drone::gameplay::CollisionEntityView;
        using drone::gameplay::Point;

        CollisionEntityView entity{
            .x = 10,
            .y = 20,
            .sprite_width = 4,
            .sprite_height = 3,
            .hitbox_width = 3,
            .hitbox_height = 2,
        };

        // Win32 0x00401F60 uses inclusive hitbox boundaries.
        assert(drone::gameplay::point_in_hitbox(Point{10, 20}, entity));
        assert(drone::gameplay::point_in_hitbox(Point{13, 22}, entity));
        assert(!drone::gameplay::point_in_hitbox(Point{14, 22}, entity));
        assert(!drone::gameplay::point_in_hitbox(Point{13, 23}, entity));

        // 4x3 indexed frame; palette index zero is transparent.
        const std::vector<std::uint8_t> frame{
            0, 0, 0, 0,
            0, 7, 0, 0,
            0, 0, 9, 0,
        };
        assert(!drone::gameplay::point_hits_opaque_pixel(Point{10, 20}, entity, frame));
        assert(drone::gameplay::point_hits_opaque_pixel(Point{11, 21}, entity, frame));
        assert(drone::gameplay::point_hits_opaque_pixel(Point{12, 22}, entity, frame));
        assert(!drone::gameplay::point_hits_opaque_pixel(Point{14, 21}, entity, frame));

        assert(drone::gameplay::point_plus_y9_in_hitbox(Point{11, 11}, entity));
        assert(!drone::gameplay::point_plus_y9_in_hitbox(Point{11, 10}, entity));

        const CollisionEntityView centered_hitbox{0, 0, 10, 10, 4, 4};
        const CollisionEntityView touching_sprite{7, 3, 2, 2, 0, 0};
        const CollisionEntityView separated_sprite{8, 3, 2, 2, 0, 0};
        assert(drone::gameplay::entity_hitbox_overlaps_sprite_rect(
            centered_hitbox, touching_sprite));
        assert(!drone::gameplay::entity_hitbox_overlaps_sprite_rect(
            centered_hitbox, separated_sprite));
    }


    {
        using drone::gameplay::PlayerDirectionalInput;
        using drone::gameplay::PlayerMotionState;

        PlayerMotionState player{.x = 147, .y = 175, .horizontal_motion = 0, .frame = 0};
        drone::gameplay::step_player_directional_motion(
            player, PlayerDirectionalInput{.left = true}, true);
        assert(player.x == 145 && player.y == 175);
        assert(player.horizontal_motion == -1);
        assert(player.frame == 1);

        drone::gameplay::step_player_directional_motion(
            player, PlayerDirectionalInput{.right = true}, true);
        assert(player.x == 147 && player.horizontal_motion == 1 && player.frame == 0);

        // The original handles A/Z vertical movement one pixel per update.
        drone::gameplay::step_player_directional_motion(
            player, PlayerDirectionalInput{.up = true}, false);
        assert(player.y == 174);
        drone::gameplay::step_player_directional_motion(
            player, PlayerDirectionalInput{.down = true}, false);
        assert(player.y == 175);

        // Exact recovered playfield clamps.
        player.x = 1; player.y = 119;
        drone::gameplay::step_player_directional_motion(player, {}, false);
        assert(player.x == 2 && player.y == 120);
        player.x = 298; player.y = 176;
        drone::gameplay::step_player_directional_motion(player, {}, false);
        assert(player.x == 297 && player.y == 175);

        // Neutral banking returns to frame zero by the shortest side of the
        // original 15-frame ring.
        player.frame = 3;
        drone::gameplay::step_player_directional_motion(player, {}, true);
        assert(player.frame == 2);
        player.frame = 14;
        drone::gameplay::step_player_directional_motion(player, {}, true);
        assert(player.frame == 0);

        // Both horizontal directions are processed in original order: position
        // cancels, frame cancels, and the later right path leaves motion +1.
        player.x = 100; player.frame = 4;
        drone::gameplay::step_player_directional_motion(
            player, PlayerDirectionalInput{.left = true, .right = true}, true);
        assert(player.x == 100 && player.frame == 4 && player.horizontal_motion == 1);
    }

    {
        using drone::gameplay::PlayerMotionState;
        using drone::gameplay::RapidMissilePool;

        RapidMissilePool pool;
        PlayerMotionState player{.x = 147, .y = 175, .horizontal_motion = 0, .frame = 0};

        // Win32 cooldown 0x004406F4 saturates at eight.
        pool.fire_cooldown = 6;
        drone::gameplay::advance_rapid_missile_cooldown(pool);
        assert(pool.fire_cooldown == 7);
        drone::gameplay::advance_rapid_missile_cooldown(pool);
        drone::gameplay::advance_rapid_missile_cooldown(pool);
        assert(pool.fire_cooldown == 8);

        // Ctrl-fire creates the first inactive 1x9 missile at the recovered
        // player-relative origin and resets the cooldown.
        assert(drone::gameplay::try_fire_rapid_missile(pool, player, true, true));
        assert(pool.active_count == 1 && pool.fire_cooldown == 0);
        assert(pool.missiles[0].active);
        assert(pool.missiles[0].x == 158); // player.x + 11
        assert(pool.missiles[0].y == 172); // player.y - 3
        assert(!pool.missiles[0].passed_top_edge);

        // Cooldown and player-active gates are independent of pool capacity.
        assert(!drone::gameplay::try_fire_rapid_missile(pool, player, true, true));
        pool.fire_cooldown = 8;
        assert(!drone::gameplay::try_fire_rapid_missile(pool, player, true, false));
        assert(pool.active_count == 1);

        // Active missiles move up three pixels on every state-2 update and
        // animate over three frames only on the separate animation tick.
        pool.missiles[0].frame = 2;
        drone::gameplay::step_rapid_missiles(pool, true);
        assert(pool.missiles[0].y == 169 && pool.missiles[0].frame == 0);
        drone::gameplay::step_rapid_missiles(pool, false);
        assert(pool.missiles[0].y == 166 && pool.missiles[0].frame == 0);

        // +0x143 is marked when y first goes negative, but the later cleanup
        // pass does not retire the missile until y < -7.
        pool.missiles[0].y = 1;
        drone::gameplay::step_rapid_missiles(pool, false);
        assert(pool.missiles[0].y == -2 && pool.missiles[0].passed_top_edge);
        assert(drone::gameplay::retire_rapid_missiles_above_top(pool) == 0);
        pool.missiles[0].y = -8;
        assert(drone::gameplay::retire_rapid_missiles_above_top(pool) == 1);
        assert(!pool.missiles[0].active && pool.active_count == 0);

        // Allocation is ascending by slot and reused entries retain their
        // current animation frame because the original spawn path does not
        // reset +0x140.
        pool.fire_cooldown = 8;
        pool.missiles[0].frame = 2;
        pool.missiles[0].active = true; // occupy slot zero without changing count yet
        pool.active_count = 1;
        assert(drone::gameplay::try_fire_rapid_missile(pool, player, true, true));
        assert(pool.missiles[1].active && pool.active_count == 2);
        assert(pool.missiles[1].frame == 0);
        assert(drone::gameplay::deactivate_rapid_missile(pool, 0));
        assert(pool.active_count == 1);
        pool.fire_cooldown = 8;
        assert(drone::gameplay::try_fire_rapid_missile(pool, player, true, true));
        assert(pool.missiles[0].active && pool.missiles[0].frame == 2);
        assert(pool.active_count == 2);

        // Capacity gate matches the recovered static pool size of eight.
        for (auto& missile : pool.missiles) missile.active = true;
        pool.active_count = 8;
        pool.fire_cooldown = 8;
        assert(!drone::gameplay::try_fire_rapid_missile(pool, player, true, true));
    }

    {
        using drone::gameplay::PlayerMotionState;
        using drone::gameplay::SpecialWeaponActivity;
        using drone::gameplay::SpecialWeaponKind;
        using drone::gameplay::SpecialWeaponState;

        PlayerMotionState player{.x = 147, .y = 175, .horizontal_motion = 0, .frame = 0};
        SpecialWeaponState special;

        // Canonical initialization has frame/weapon 0 (blue probe), activity
        // zero and a twelve-update switch threshold.
        assert(special.kind == SpecialWeaponKind::Probe);
        assert(special.activity == SpecialWeaponActivity::Inactive);
        assert(special.switch_threshold == 12);

        // First Down-key action loads the currently selected kind. The input
        // path writes Y=player.y+7, clears +0x143, and resets the switch timer.
        assert(drone::gameplay::load_special_weapon(special, player, true, true));
        assert(special.activity == SpecialWeaponActivity::LoadedTracking);
        assert(special.y == 182 && !special.out_of_bounds);
        assert(special.switch_progress == 0);

        // State 1 advances the recovered +0x36 counter to the +0x3C value 12;
        // only then can another Down-key action toggle Probe <-> Stinger.
        for (int i = 0; i < 11; ++i) {
            drone::gameplay::advance_special_weapon_switch_progress(special);
        }
        assert(special.switch_progress == 11);
        assert(!drone::gameplay::toggle_loaded_special_weapon(special, true, true));
        drone::gameplay::advance_special_weapon_switch_progress(special);
        assert(drone::gameplay::toggle_loaded_special_weapon(special, true, true));
        assert(special.kind == SpecialWeaponKind::Stinger && special.switch_progress == 0);

        // While loaded, each update first re-anchors the projectile to
        // player+(14,7), then applies Y-=2 and one-pixel horizontal homing.
        assert(drone::gameplay::step_special_weapon_homing(special, player, 200));
        assert(special.x == 162); // (147 + 14) + 1 toward target
        assert(special.y == 180); // (175 + 7) - 2

        // Up Arrow launches the state-1 entity as state 3. Subsequent updates
        // retain its current position and continue Y-=2 / X step-by-one.
        assert(drone::gameplay::launch_special_weapon(special, true, true));
        assert(special.activity == SpecialWeaponActivity::LaunchedHoming);
        assert(drone::gameplay::step_special_weapon_homing(special, player, 160));
        assert(special.x == 161 && special.y == 178);

        assert(drone::gameplay::stinger_target_x(100, 15) == 107);
        assert(drone::gameplay::probe_drone_target_x(100) == 104);

        // Only a launched blue probe can enter the recovered attached/decode
        // state 2 via the Drone collision path.
        assert(!drone::gameplay::attach_probe_to_drone(special));
        special.kind = SpecialWeaponKind::Probe;
        assert(drone::gameplay::attach_probe_to_drone(special));
        assert(special.activity == SpecialWeaponActivity::ProbeAttachedDecoding);
        assert(special.motion_x == 0 && special.motion_y == 0);

        // Common state dispatch pins an attached Probe to Drone.x+5.
        assert(drone::gameplay::pin_attached_probe_to_drone(special, 200));
        assert(special.x == 205);

        // State 4 is entered only from a launched projectile after the
        // separately gated hole collision; state 10 is a terminal impact state
        // that the common next-update dispatcher clears to inactive.
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        assert(drone::gameplay::enter_special_weapon_hole_interaction(special));
        assert(special.activity == SpecialWeaponActivity::HoleInteraction);
        assert(!drone::gameplay::enter_special_weapon_hole_interaction(special));
        special.activity = SpecialWeaponActivity::ImpactConsumed;
        assert(drone::gameplay::settle_special_weapon_terminal_state(special));
        assert(special.activity == SpecialWeaponActivity::Inactive);
        assert(!drone::gameplay::settle_special_weapon_terminal_state(special));

        // Out-of-range X terminates the movable special entity and raises the
        // common +0x143 edge flag in the canonical update path.
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.x = 319;
        special.y = 50;
        assert(drone::gameplay::step_special_weapon_homing(special, player, 400));
        assert(special.activity == SpecialWeaponActivity::Inactive);
        assert(special.out_of_bounds);
    }

    {
        const auto path = base / "fixture.dat";
        std::ofstream out(path);
        // Two synthetic records using the recovered channel semantics.
        const int record0[14] = {1, 0, 1, 0, 1, 1, 4, -37, 2, 1, 123, 45, 201, -18};
        const int record1[14] = {0, 1, 0, 1, 0, 0, 99, 0, 99, 0, 0, 0, 205, -17};
        for (const auto value : record0) out << value << '\n';
        for (const auto value : record1) out << value << '\n';
        out.close();

        const auto raw = drone::formats::load_demo_dat(path);
        assert(raw.size() == 2 && raw[1][13] == -17);

        const auto demo = drone::formats::load_demo_frames(path);
        assert(demo.size() == 2);
        assert(demo[0].left && !demo[0].right);
        assert(demo[0].launch_special && demo[0].shield && demo[0].rapid_missile);
        assert(demo[0].has_trajectory_group_event());
        assert(demo[0].trajectory_group_slot == 4);
        assert(demo[0].trajectory_group_x_offset == -37);
        assert(demo[0].has_explicit_trajectory_path_family());
        assert(demo[0].trajectory_path_family == 2);
        assert(demo[0].bomb_spawned && demo[0].bomb_x == 123 && demo[0].bomb_y == 45);
        assert(demo[0].drone_x == 201 && demo[0].drone_y == -18);
        assert(!demo[1].has_trajectory_group_event());
        assert(!demo[1].has_explicit_trajectory_path_family());

        const auto gameplay = drone::gameplay::build_demo_gameplay_frame(demo[0]);
        assert(gameplay.horizontal_input.left && !gameplay.horizontal_input.right);
        // Replay channels do not contain the live A/Z vertical player controls.
        assert(!gameplay.horizontal_input.up && !gameplay.horizontal_input.down);
        assert(gameplay.launch_special && gameplay.shield && gameplay.rapid_missile);
        assert(gameplay.trajectory.spawn && gameplay.trajectory.group_slot == 4);
        assert(gameplay.trajectory.group_x_offset == -37);
        assert(gameplay.trajectory.path_family && *gameplay.trajectory.path_family == 2);
        assert(gameplay.bomb.spawn && gameplay.bomb.x == 123 && gameplay.bomb.y == 45);
        assert(gameplay.drone.x == 201 && gameplay.drone.y == -18);

        // The original fscanf path writes integers through pointers into
        // byte/word arrays; gameplay observes only the low 8/16 bits. Mirror
        // that historical narrowing instead of sanitizing it away.
        auto widened = raw[0];
        widened[6] = 500;      // low byte 0xF4 -> signed -12
        widened[10] = 65535;   // low word 0xFFFF -> signed -1
        const auto narrowed = drone::formats::decode_demo_record(widened);
        assert(narrowed.trajectory_group_slot == -12);
        assert(narrowed.bomb_x == -1);
    }


    {
        using drone::gameplay::EnemyBombPool;
        using drone::gameplay::EnemyBombSteeringContext;

        EnemyBombPool bombs;

        // Live mode chooses rand()%3 upstream and stores that value in +0x10.
        assert(drone::gameplay::spawn_live_enemy_bomb(bombs, 100, 50, 2));
        assert(bombs.active_count == 1);
        assert(bombs.bombs[0].active && bombs.bombs[0].horizontal_step == 2);

        // The active update uses the three-frame ring, steers toward player.x+17,
        // and falls two pixels per update.
        bombs.bombs[0].frame = 2;
        drone::gameplay::step_enemy_bombs(
            bombs, true, EnemyBombSteeringContext{.player_x = 90});
        assert(bombs.bombs[0].frame == 0);
        assert(bombs.bombs[0].x == 102); // target = 107, step +2
        assert(bombs.bombs[0].y == 52);

        // Attached-Probe redirection uses probe.x+1 when the caller supplies
        // the independently recovered gating condition.
        drone::gameplay::step_enemy_bombs(
            bombs, false,
            EnemyBombSteeringContext{
                .player_x = 90,
                .redirect_to_attached_probe = true,
                .attached_probe_x = 80,
            });
        assert(bombs.bombs[0].x == 100); // target = 81, step -2
        assert(bombs.bombs[0].y == 54);

        // Demo playback restores X/Y but explicitly zeroes +0x10, so these
        // bombs fall vertically even though live bombs may steer.
        assert(drone::gameplay::spawn_replay_enemy_bomb(bombs, 200, 189));
        assert(bombs.bombs[1].horizontal_step == 0);
        drone::gameplay::step_enemy_bombs(
            bombs, false, EnemyBombSteeringContext{.player_x = 0});
        assert(bombs.bombs[1].x == 200 && bombs.bombs[1].y == 191);
        assert(bombs.bombs[1].out_of_bounds); // logical visibility bound is 190

        // Cleanup is later and deliberately looser: y==198 survives, y==199 retires.
        bombs.bombs[1].y = 198;
        assert(drone::gameplay::retire_enemy_bombs_below_bottom(bombs) == 0);
        bombs.bombs[1].y = 199;
        assert(drone::gameplay::retire_enemy_bombs_below_bottom(bombs) == 1);
        assert(bombs.active_count == 1);
    }


    {
        // Win32 bomb->player collision consequence: the bomb is consumed in
        // both branches. Shielded impact requests a stationary mini effect;
        // unshielded impact destroys the player and auto-launches a merely
        // loaded Probe/Stinger before the death path.
        drone::gameplay::EnemyBombPool pool;
        assert(drone::gameplay::spawn_live_enemy_bomb(pool, 50, 150, 2));
        const auto absorbed = drone::gameplay::resolve_enemy_bomb_player_impact(
            pool, 0, true, true);
        assert(absorbed.bomb_deactivated && absorbed.shield_absorbed);
        assert(absorbed.spawn_absorption_effect && !absorbed.destroy_player);
        assert(!absorbed.launch_loaded_special && !absorbed.play_player_hit_sfx);
        assert(pool.active_count == 0 && pool.bombs[0].horizontal_step == 0);

        assert(drone::gameplay::spawn_live_enemy_bomb(pool, 80, 140, 1));
        const auto lethal = drone::gameplay::resolve_enemy_bomb_player_impact(
            pool, 0, false, true);
        assert(lethal.bomb_deactivated && !lethal.shield_absorbed);
        assert(lethal.destroy_player && lethal.launch_loaded_special);
        assert(lethal.play_player_hit_sfx && !lethal.spawn_absorption_effect);
        assert(pool.active_count == 0);

        const auto none = drone::gameplay::resolve_enemy_bomb_player_impact(
            pool, 0, false, true);
        assert(!none.bomb_deactivated && !none.destroy_player);
    }


    {
        using drone::gameplay::PlayerShieldState;

        PlayerShieldState shield;
        assert(shield.energy == drone::gameplay::shield_nominal_max_energy);
        assert(drone::gameplay::displayed_shield_units(shield) == 75);
        assert(!shield.active);

        // Exact nominal maximum does not recharge.
        drone::gameplay::regenerate_player_shield(shield);
        assert(shield.energy == drone::gameplay::shield_nominal_max_energy);

        // Fidelity quirk: recharge is guarded by only the high 16 bits, so a
        // 74.x value can cross the nominal maximum without being clamped.
        shield.energy = (74 << 16) | 0xFF00;
        drone::gameplay::regenerate_player_shield(shield);
        assert(shield.energy == drone::gameplay::shield_nominal_max_energy + 1044);
        assert(drone::gameplay::displayed_shield_units(shield) == 75);

        // The state-2 ordering recharges before applying the Space-key drain.
        shield.energy = drone::gameplay::shield_nominal_max_energy;
        auto result = drone::gameplay::step_player_shield(shield, true, true, true);
        assert(shield.energy == drone::gameplay::shield_nominal_max_energy - 0xBB80);
        assert(shield.active && result.active && result.play_sound);

        // Sound is cadence-gated even while protection remains active.
        result = drone::gameplay::step_player_shield(shield, true, true, false);
        assert(result.active && !result.play_sound);

        // A nearly empty shield can regenerate and still be exhausted by the
        // same update's drain. Negative results clamp to exactly zero.
        shield.energy = 1;
        result = drone::gameplay::step_player_shield(shield, true, true, true);
        assert(shield.energy == 0 && !shield.active);
        assert(!result.active && !result.play_sound);

        // Space does not drain while the player entity is inactive, but the
        // ordinary per-update recharge still occurs.
        shield.energy = 10 << 16;
        result = drone::gameplay::step_player_shield(shield, true, false, true);
        assert(shield.energy == (10 << 16) + drone::gameplay::shield_regen_per_update);
        assert(!result.active && !result.play_sound);

        drone::gameplay::reset_player_shield(shield);
        assert(shield.energy == drone::gameplay::shield_nominal_max_energy);
        assert(!shield.active);
    }

    fs::remove_all(base);
    std::cout << "all Drone format tests passed\n";
    return 0;
}
