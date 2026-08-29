#include <drone/gameplay/player_death_effect.hpp>

#include <cassert>
#include <iostream>

using namespace drone::gameplay;

int main() {
    {
        PlayerMotionState player{};
        player.x = 147;
        player.y = 175;
        player.horizontal_motion = 1;
        PlayerDeathEffectState effect{};
        trigger_player_death_effect(effect, player);
        assert(effect.x == 137);
        assert(effect.y == 167);
        assert(effect.motion_x == 1);
        assert(effect.motion_y == 0);
        assert(effect.frame == -6);
        assert(effect.activity == PlayerDeathEffectActivity::PreRoll);
    }

    // Non-phase-2 updates do not move or animate the singleton.
    {
        PlayerDeathEffectState effect{};
        effect.activity = PlayerDeathEffectActivity::PreRoll;
        effect.frame = -6;
        effect.x = 10;
        effect.motion_x = 1;
        const auto r = step_player_death_effect(effect, 1);
        assert(!r.advanced);
        assert(effect.x == 10 && effect.frame == -6);
    }

    // Six phase-2 advances convert pre-roll frame -6 into visible frame 0.
    {
        PlayerDeathEffectState effect{};
        effect.activity = PlayerDeathEffectActivity::PreRoll;
        effect.frame = -6;
        for (int i = 0; i < 5; ++i) {
            const auto r = step_player_death_effect(effect, 2);
            assert(r.advanced && !r.became_visible);
            assert(effect.activity == PlayerDeathEffectActivity::PreRoll);
        }
        const auto r = step_player_death_effect(effect, 2);
        assert(r.became_visible);
        assert(effect.frame == 0);
        assert(player_death_effect_visible(effect));
    }

    // Frames 0..26 are visible; incrementing to terminal frame 27 retires it.
    {
        PlayerDeathEffectState effect{};
        effect.activity = PlayerDeathEffectActivity::Visible;
        effect.frame = 25;
        auto r = step_player_death_effect(effect, 2);
        assert(!r.retired_at_terminal_frame);
        assert(effect.frame == 26 && player_death_effect_visible(effect));
        r = step_player_death_effect(effect, 2);
        assert(r.retired_at_terminal_frame);
        assert(effect.frame == 27 && player_death_effect_inactive(effect));
    }

    // Bounds are strict: x==319 remains active, x==320 clears. The original
    // then continues to frame logic, so -1->0 can briefly restore visibility.
    {
        PlayerDeathEffectState effect{};
        effect.activity = PlayerDeathEffectActivity::PreRoll;
        effect.frame = -1;
        effect.x = 319;
        effect.motion_x = 1;
        const auto r = step_player_death_effect(effect, 2);
        assert(r.cleared_out_of_bounds);
        assert(r.became_visible);
        assert(effect.x == 320);
        assert(effect.frame == 0);
        assert(player_death_effect_visible(effect));
    }

    std::cout << "Drone player-death effect tests passed\n";
    return 0;
}
