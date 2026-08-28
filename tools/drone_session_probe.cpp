#include <drone/gameplay/game_session.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

drone::gameplay::GameplayInputFrame scripted_input(const int update) {
    drone::gameplay::GameplayInputFrame input{};

    // Deterministic portable input choreography. This is a clean validation
    // fixture, not a claim about the original demo.dat recording.
    const int horizontal_phase = (update / 20) % 2;
    input.movement.left = horizontal_phase == 0;
    input.movement.right = horizontal_phase != 0;
    input.movement.up = (update % 16) < 4;
    input.movement.down = (update % 16) >= 8 && (update % 16) < 12;
    input.rapid_fire = true;
    input.shield = (update % 5) < 2;

    // Load Probe on update 0. Twelve loaded updates later, cycle to Stinger,
    // then launch on the following update.
    input.special_load_cycle = (update == 0 || update == 12);
    input.special_launch = (update == 13);
    return input;
}

} // namespace

int main(int argc, char** argv) try {
    if (argc > 2) {
        std::cerr << "Usage: drone_session_probe [updates]\n";
        return 2;
    }

    const int updates = argc == 2 ? std::stoi(argv[1]) : 120;
    if (updates < 0 || updates > 100000) {
        throw std::runtime_error("updates must be 0..100000");
    }

    drone::gameplay::GameSession session{};
    drone::gameplay::GameSessionTargetContext targets{};
    targets.stinger_target = drone::gameplay::SpecialTargetGeometry{.x = 220, .width = 20};

    std::size_t fired = 0;
    std::size_t shield_sound_requests = 0;
    for (int update = 0; update < updates; ++update) {
        const auto result = drone::gameplay::step_game_session(
            session, scripted_input(update), targets);
        if (!result.advanced) {
            throw std::runtime_error("active session unexpectedly failed to advance");
        }
        fired += result.rapid_missile_fired ? 1u : 0u;
        shield_sound_requests += result.shield_sound_requested ? 1u : 0u;
    }

    const auto& e = session.encounter;
    const auto& c = session.campaign;
    std::cout
        << "updates=" << e.gameplay_updates
        << " total=" << session.total_gameplay_updates
        << " phase=" << e.gameplay_substep_phase
        << " scroll=" << e.world_scroll_row
        << " player=" << e.player.x << ',' << e.player.y << ',' << e.player.frame
        << " missiles=" << e.rapid_missiles.active_count
        << " cooldown=" << e.rapid_missiles.fire_cooldown
        << " fired=" << fired
        << " shield=" << e.shield.energy << ',' << (e.shield.active ? 1 : 0)
        << " shield_sfx=" << shield_sound_requests
        << " special=" << static_cast<int>(e.special_weapon.kind)
        << ',' << static_cast<int>(e.special_weapon.activity)
        << ',' << e.special_weapon.x << ',' << e.special_weapon.y
        << " bomb_gate=" << e.enemy_bomb_spawn_gate.counter
        << " lives=" << c.player_lifecycle.lives
        << " score=" << c.score.total << ',' << c.score.extra_life_progress
        << '\n';
    return 0;
} catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
}
