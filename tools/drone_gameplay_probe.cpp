#include <drone/fidelity/indexed_framebuffer.hpp>
#include <drone/fidelity/sprite_blit.hpp>
#include <drone/fidelity/sprite_sheet.hpp>
#include <drone/formats/jba.hpp>
#include <drone/gameplay/player.hpp>
#include <drone/gameplay/rapid_missile.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

void write_ppm(const drone::fidelity::IndexedFramebuffer& fb, const fs::path& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("unable to create output: " + path.string());
    out << "P6\n320 200\n255\n";
    for (const auto index : fb.pixels()) {
        const auto& c = fb.palette()[index];
        const char rgb[3]{static_cast<char>(c.r), static_cast<char>(c.g), static_cast<char>(c.b)};
        out.write(rgb, sizeof(rgb));
    }
}

std::array<drone::fidelity::IndexedSpriteFrame, 15> load_ship_frames(
    const drone::formats::JbaImage& sheet) {
    std::array<drone::fidelity::IndexedSpriteFrame, 15> frames;
    for (std::size_t slot = 0; slot < frames.size(); ++slot) {
        frames[slot] = drone::fidelity::extract_guttered_jba_frame(
            sheet, 22, 22, slot % 4, slot / 4);
    }
    return frames;
}

std::array<drone::fidelity::IndexedSpriteFrame, 3> load_missile_frames(
    const drone::formats::JbaImage& sheet) {
    std::array<drone::fidelity::IndexedSpriteFrame, 3> frames;
    for (std::size_t slot = 0; slot < frames.size(); ++slot) {
        frames[slot] = drone::fidelity::extract_guttered_jba_frame(sheet, 1, 9, slot, 0);
    }
    return frames;
}

} // namespace

int main(int argc, char** argv) try {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: drone_gameplay_probe <windows-shareware-root> <output.ppm> [updates]\n";
        return 2;
    }

    const fs::path root = argv[1];
    const fs::path output = argv[2];
    const int updates = argc == 4 ? std::stoi(argv[3]) : 36;
    if (updates < 0 || updates > 10000) throw std::runtime_error("updates must be 0..10000");

    const auto ship_sheet = drone::formats::load_jba_320x200(root / "Sights" / "Ship.jba");
    const auto missile_sheet = drone::formats::load_jba_320x200(root / "Sights" / "Missile.jba");
    const auto ship_frames = load_ship_frames(ship_sheet);
    const auto missile_frames = load_missile_frames(missile_sheet);

    drone::gameplay::PlayerMotionState player;
    drone::gameplay::RapidMissilePool missiles;

    // Run a tiny deterministic slice of reconstructed state-2 ordering:
    // cooldown advance -> fire input -> missile update -> top cleanup.
    // Fire is held continuously; the original cooldown therefore determines
    // when subsequent pool entries are allocated.
    for (int update = 0; update < updates; ++update) {
        drone::gameplay::advance_rapid_missile_cooldown(missiles);
        (void)drone::gameplay::try_fire_rapid_missile(missiles, player, true, true);
        drone::gameplay::step_rapid_missiles(missiles, (update % 4) == 2);
        (void)drone::gameplay::retire_rapid_missiles_above_top(missiles);
    }

    drone::fidelity::IndexedFramebuffer framebuffer;
    framebuffer.palette() = ship_sheet.palette;
    std::fill(framebuffer.pixels().begin(), framebuffer.pixels().end(), std::uint8_t{0});

    const auto player_frame = static_cast<std::size_t>(player.frame) % ship_frames.size();
    drone::fidelity::blit_transparent_original(
        framebuffer, ship_frames[player_frame], player.x, player.y);

    for (const auto& missile : missiles.missiles) {
        if (!missile.active) continue;
        drone::fidelity::blit_transparent_original(
            framebuffer, missile_frames[missile.frame % missile_frames.size()], missile.x, missile.y);
    }

    write_ppm(framebuffer, output);
    std::cout << "updates=" << updates
              << " active_missiles=" << missiles.active_count
              << " cooldown=" << missiles.fire_cooldown
              << " output=" << output << '\n';
    return 0;
} catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
}
