#include <drone/audio/audio_event.hpp>
#include <drone/gameplay/enemy_bomb.hpp>

#include <cassert>
#include <iostream>

namespace {

using namespace drone::audio;
using namespace drone::gameplay;

void arm_bomb(EnemyBombPool& pool, const std::size_t index, const std::int32_t x, const std::int32_t y) {
    pool.bombs[index].active = true;
    pool.bombs[index].x = x;
    pool.bombs[index].y = y;
    ++pool.active_count;
}

PlayerMotionState player_at(const std::int32_t x, const std::int32_t y) {
    PlayerMotionState player{};
    player.x = x;
    player.y = y;
    return player;
}

} // namespace

int main() {
    // One bomb can hit a launched Probe and then the player in the same slot.
    // Win32 stops probe3, plays explode4, then falls through to bigexp3.
    {
        EnemyBombPool bombs{};
        arm_bomb(bombs, 0, 101, 91);
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.kind = SpecialWeaponKind::Probe;
        special.x = 100;
        special.y = 100;
        auto player = player_at(100, 100);
        PlayerLifecycleState lifecycle{};
        lifecycle.player_active = true;
        EnemyBombSpawnGate gate{};

        const auto result = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, false, gate);
        const auto audio = result.audio_events.view();
        assert(audio.size() == 3);
        assert((audio[0] == AudioEvent{AudioCue::SpecialLaunch, AudioAction::StopAndRewind}));
        assert((audio[1] == AudioEvent{AudioCue::ProbeImpact, AudioAction::Play}));
        assert((audio[2] == AudioEvent{AudioCue::PlayerHitExplosion, AudioAction::Play}));
    }

    // A player hit in an earlier slot can auto-launch the loaded Probe. A later
    // bomb then stops that launch and consumes it. Preserve cross-slot order.
    {
        EnemyBombPool bombs{};
        arm_bomb(bombs, 0, 100, 91);
        arm_bomb(bombs, 1, 200, 91);
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LoadedTracking;
        special.kind = SpecialWeaponKind::Probe;
        special.x = 200;
        special.y = 100;
        auto player = player_at(100, 100);
        PlayerLifecycleState lifecycle{};
        lifecycle.player_active = true;
        EnemyBombSpawnGate gate{};

        const auto result = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, false, gate);
        const auto audio = result.audio_events.view();
        assert(audio.size() == 4);
        assert((audio[0] == AudioEvent{AudioCue::SpecialLaunch, AudioAction::Play}));
        assert((audio[1] == AudioEvent{AudioCue::PlayerHitExplosion, AudioAction::Play}));
        assert((audio[2] == AudioEvent{AudioCue::SpecialLaunch, AudioAction::StopAndRewind}));
        assert((audio[3] == AudioEvent{AudioCue::ProbeImpact, AudioAction::Play}));
    }

    // Stinger impact uses its own 20-voice pool after stopping probe3.wav.
    {
        EnemyBombPool bombs{};
        arm_bomb(bombs, 0, 201, 91);
        SpecialWeaponState special{};
        special.activity = SpecialWeaponActivity::LaunchedHoming;
        special.kind = SpecialWeaponKind::Stinger;
        special.x = 200;
        special.y = 100;
        auto player = player_at(20, 150);
        PlayerLifecycleState lifecycle{};
        lifecycle.player_active = true;
        EnemyBombSpawnGate gate{};

        const auto result = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, false, gate);
        const auto audio = result.audio_events.view();
        assert(audio.size() == 2);
        assert((audio[0] == AudioEvent{AudioCue::SpecialLaunch, AudioAction::StopAndRewind}));
        assert((audio[1] == AudioEvent{AudioCue::StingerImpact, AudioAction::Play}));
    }

    // Shield absorption only spawns the mini-explosion sprite at this site; it
    // has no DirectSound call in the recovered late-bomb branch.
    {
        EnemyBombPool bombs{};
        arm_bomb(bombs, 0, 100, 91);
        SpecialWeaponState special{};
        auto player = player_at(100, 100);
        PlayerLifecycleState lifecycle{};
        lifecycle.player_active = true;
        EnemyBombSpawnGate gate{};

        const auto result = process_enemy_bomb_late_collision_pass(
            bombs, special, player, lifecycle, true, gate);
        assert(result.shield_absorptions == 1);
        assert(result.audio_events.size == 0);
    }

    std::cout << "ordered impact audio tests passed\n";
    return 0;
}
