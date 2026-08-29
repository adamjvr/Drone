#include <drone/audio/audio_event.hpp>
#include <drone/audio/original_directsound.hpp>
#include <drone/gameplay/game_session.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>

using namespace drone::audio;

int main() {
    static_assert(original_sfx_voice_pool_capacity == 20);
    static_assert(original_directsound_attenuation(100) == 0);
    static_assert(original_directsound_attenuation(90) == -300);
    static_assert(original_directsound_attenuation(70) == -900);
    static_assert(original_directsound_attenuation(50) == -1500);

    std::array<std::uint32_t, original_sfx_voice_pool_capacity> status{};
    status.fill(directsound_status_playing);
    status[7] = 0;
    assert(select_original_sfx_voice(status) == 7);

    // The helper compares raw status exactly to 1; extra DirectSound status bits
    // therefore make a slot reusable in the original implementation.
    status.fill(directsound_status_playing);
    status[3] = 3;
    assert(select_original_sfx_voice(status) == 3);

    status.fill(directsound_status_playing);
    assert(select_original_sfx_voice(status) == 0);

    const auto& missile = audio_cue_definition(AudioCue::RapidMissileFire);
    assert(missile.original_asset == "missile.wav");
    assert(missile.voice_policy == AudioVoicePolicy::ReusablePool20);
    assert(missile.original_volume_0_to_100 == 50);
    assert(missile.original_frequency_hz == 22050);

    const auto& shield = audio_cue_definition(AudioCue::ShieldPulse);
    assert(shield.original_asset == "shields.wav");
    assert(shield.original_frequency_hz == 11025);

    const auto& launch = audio_cue_definition(AudioCue::SpecialLaunch);
    assert(launch.original_asset == "probe3.wav");
    assert(launch.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(launch.original_volume_0_to_100 == 70);

    const auto& player_hit = audio_cue_definition(AudioCue::PlayerHitExplosion);
    assert(player_hit.original_asset == "bigexp3.wav");
    assert(player_hit.original_frequency_hz == 15000);

    const auto& bomb = audio_cue_definition(AudioCue::EnemyBombFire);
    assert(bomb.original_asset == "missile.wav");
    assert(bomb.voice_policy == AudioVoicePolicy::ReusablePool20);
    assert(bomb.original_volume_0_to_100 == 50);
    assert(bomb.original_frequency_hz == 0);

    const auto& results = audio_cue_definition(AudioCue::ResultsChoral);
    assert(results.original_asset == "choral.wav");
    assert(results.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(results.directsound_play_flags == 0);

    const auto& lid_top_loop = audio_cue_definition(AudioCue::LidTopBossLoop);
    assert(lid_top_loop.original_asset == "retro1.wav");
    assert(lid_top_loop.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(lid_top_loop.original_volume_0_to_100 == 70);
    assert(lid_top_loop.directsound_play_flags == directsound_play_looping_flag);

    const auto& gemini_loop = audio_cue_definition(AudioCue::GeminiBossLoop);
    assert(gemini_loop.original_asset == "gemini.wav");
    assert(gemini_loop.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(gemini_loop.original_volume_0_to_100 == 100);
    assert(gemini_loop.directsound_play_flags == directsound_play_looping_flag);

    const auto& ordering = audio_cue_definition(AudioCue::OrderingInformation);
    assert(ordering.original_asset == "thunder2.wav");
    assert(ordering.original_volume_0_to_100 == -1);
    assert(ordering.directsound_play_flags == directsound_play_looping_flag);

    const auto& credits = audio_cue_definition(AudioCue::CompletionCredits);
    assert(credits.original_asset == "credits.wav");
    assert(credits.directsound_play_flags == directsound_play_looping_flag);

    const auto loop_sites = original_directsound_loop_call_sites();
    assert(loop_sites.size() == 13);
    const auto literal_count = std::count_if(
        loop_sites.begin(), loop_sites.end(), [](const OriginalLoopCallSite& site) {
            return site.flag_proof == OriginalLoopFlagProof::LiteralOne;
        });
    const auto register_count = std::count_if(
        loop_sites.begin(), loop_sites.end(), [](const OriginalLoopCallSite& site) {
            return site.flag_proof == OriginalLoopFlagProof::RegisterProvenOne;
        });
    assert(literal_count == 8);
    assert(register_count == 5);
    const auto has_site = [&](const std::uint32_t va, const std::string_view asset) {
        return std::any_of(loop_sites.begin(), loop_sites.end(), [&](const auto& site) {
            return site.call_site_va == va && site.original_asset == asset;
        });
    };
    assert(has_site(0x0040474Du, "credits.wav"));
    assert(has_site(0x0040C8F3u, "air.wav"));
    assert(has_site(0x0040E52Fu, "drone.wav"));
    assert(has_site(0x00418B14u, "lowbees.wav"));
    assert(has_site(0x0041B75Bu, "thunder2.wav"));
    const auto unresolved = std::find_if(
        loop_sites.begin(), loop_sites.end(), [](const OriginalLoopCallSite& site) {
            return site.owner == OriginalLoopPlaybackOwner::RegisteredBossSlot2Unresolved;
        });
    assert(unresolved != loop_sites.end());
    assert(unresolved->call_site_va == 0x00407AA3u);
    assert(unresolved->original_asset.empty());

    assert(audio_cue_definition(AudioCue::ExplosionVariant2).original_asset == "explode2.wav");
    assert(audio_cue_definition(AudioCue::ExplosionVariant2).original_volume_0_to_100 == 60);
    assert(audio_cue_definition(AudioCue::ExplosionVariant3).original_asset == "explode3.wav");
    assert(audio_cue_definition(AudioCue::ExplosionVariant3).original_volume_0_to_100 == 50);
    assert(audio_cue_definition(AudioCue::ExplosionVariant4).original_asset == "explode4.wav");
    assert(audio_cue_definition(AudioCue::ExplosionVariant4).original_volume_0_to_100 == 50);

    OriginalAudioRuntimeState variant{};
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant2);
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant2);
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant3);
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant4);
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant2);

    drone::gameplay::GameSession session{};
    (void)next_original_explosion_sfx_cue(session.original_audio);
    (void)next_original_explosion_sfx_cue(session.original_audio);
    assert(session.original_audio.explosion_sfx_variant_cycle == 2);
    drone::gameplay::reset_game_session(
        session, drone::gameplay::GameplaySessionResetScope::FullCampaign);
    assert(session.original_audio.explosion_sfx_variant_cycle == 2);

    assert(trajectory_flight_cue(0) == AudioCue::TrajectoryFlight01);
    assert(trajectory_flight_cue(13) == AudioCue::TrajectoryFlight14);

    AudioEventQueue queue{};
    assert(queue.push({AudioCue::RapidMissileFire, AudioAction::Play}));
    assert(queue.push({AudioCue::SpecialLaunch, AudioAction::StopAndRewind}));
    assert(queue.size == 2);
    assert(!queue.overflowed);
    assert((queue.view()[0] == AudioEvent{AudioCue::RapidMissileFire, AudioAction::Play}));
    assert((queue.view()[1] == AudioEvent{AudioCue::SpecialLaunch, AudioAction::StopAndRewind}));

    for (std::size_t i = queue.size; i < game_session_audio_event_capacity; ++i) {
        assert(queue.push({AudioCue::ShieldPulse, AudioAction::Play}));
    }
    assert(!queue.push({AudioCue::ShieldPulse, AudioAction::Play}));
    assert(queue.overflowed);

    return 0;
}
