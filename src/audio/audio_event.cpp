#include <drone/audio/audio_event.hpp>

#include <array>

namespace drone::audio {
namespace {

using Policy = AudioVoicePolicy;

constexpr std::array<AudioCueDefinition, 37> definitions{{
    // RapidMissileFire
    {"missile.wav", Policy::ReusablePool20, 50, original_missile_frequency_hz, 0},
    // ShieldPulse
    {"shields.wav", Policy::ReusablePool20, 50, original_shield_frequency_hz, 0},
    // SpecialLoadCycle
    {"ignite2.wav", Policy::SingleBuffer, 90, 0, 0},
    // SpecialLaunch
    {"probe3.wav", Policy::SingleBuffer, 70, 0, 0},
    // ProbeImpact
    {"explode4.wav", Policy::ReusablePool20, 50, 0, 0},
    // StingerImpact
    {"stinger1.wav", Policy::ReusablePool20, 100, 0, 0},
    // PlayerHitExplosion
    {"bigexp3.wav", Policy::ReusablePool20, 90, original_bigexp3_frequency_hz, 0},
    // EnemyBombFire: the boss-bomb pool duplicates the same missile.wav base
    // but preserves source/default frequency rather than forcing 22050 Hz.
    {"missile.wav", Policy::ReusablePool20, 50, 0, 0},
    // Win32 0x00402900 explosion-SFX cycle.
    {"explode2.wav", Policy::ReusablePool20, 60, 0, 0},
    {"explode3.wav", Policy::ReusablePool20, 50, 0, 0},
    {"explode4.wav", Policy::ReusablePool20, 50, 0, 0},
    // Squad1..14 flight pools
    {"squad1.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad2.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad3.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad4.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad5.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad6.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad7.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad8.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad9.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad10.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad11.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad12.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad13.wav", Policy::ReusablePool20, -1, 0, 0},
    {"squad14.wav", Policy::ReusablePool20, -1, 0, 0},
    // Mission interstitials
    {"deepness.wav", Policy::SingleBuffer, 90, 0, 0},
    {"detonate.wav", Policy::SingleBuffer, 90, 0, 0},
    // Persistent air ambience. The dedicated slot is loaded at 50 and loops;
    // state-2 transition paths control its live scalar through SetVolume.
    {"air.wav", Policy::SingleBuffer, original_air_loop_loaded_volume, 0,
     directsound_play_looping_flag},
    // Drone approach loop. The slot is loaded at 90, but the live start path
    // immediately overrides it to 0 and then controls it through SetVolume.
    {"drone.wav", Policy::SingleBuffer, original_drone_loop_loaded_volume, 0,
     directsound_play_looping_flag},
    // Shareware-reachable boss encounter loops. The original starts these in
    // the dedicated encounter initializers and stops them from the boss update
    // at the proven destruction transition, before resource release.
    {"retro1.wav", Policy::SingleBuffer, 70, 0, directsound_play_looping_flag},
    {"gemini.wav", Policy::SingleBuffer, 100, 0, directsound_play_looping_flag},
    // Results chooses exactly one of these at Win32 0x00411726..0x0041176C.
    // These are deliberately one-shot Play(flags=0), unlike the loop-owned
    // presentation/encounter sounds cataloged by original_directsound.cpp.
    {"choral.wav", Policy::SingleBuffer, -1, 0, 0},
    {"suspense.wav", Policy::SingleBuffer, -1, 0, 0},
    {"moon.wav", Policy::SingleBuffer, -1, 0, 0},
    {"hiphop.wav", Policy::SingleBuffer, -1, 0, 0},
    // Ordering Information and completion credits each own a stack-local
    // DirectSound slot and start it with flags=1. Neither writes an initial
    // volume before Play, so source/default volume remains explicit unknown.
    {"thunder2.wav", Policy::SingleBuffer, -1, 0, directsound_play_looping_flag},
    {"credits.wav", Policy::SingleBuffer, -1, 0, directsound_play_looping_flag},
}};

static_assert(definitions.size() == static_cast<std::size_t>(AudioCue::CompletionCredits) + 1);

} // namespace

const AudioCueDefinition& audio_cue_definition(const AudioCue cue) noexcept {
    return definitions[static_cast<std::size_t>(cue)];
}

AudioCue trajectory_flight_cue(const std::uint8_t sound_index) noexcept {
    const auto clamped = sound_index < 14 ? sound_index : std::uint8_t{13};
    return static_cast<AudioCue>(
        static_cast<std::uint8_t>(AudioCue::TrajectoryFlight01) + clamped);
}

AudioCue next_original_explosion_sfx_cue(OriginalAudioRuntimeState& state) noexcept {
    auto next = static_cast<std::uint8_t>(state.explosion_sfx_variant_cycle + 1u);
    if (next == 5u) next = 1u;
    state.explosion_sfx_variant_cycle = next;
    switch (next) {
    case 1:
    case 2:
        return AudioCue::ExplosionVariant2;
    case 3:
        return AudioCue::ExplosionVariant3;
    case 4:
    default:
        return AudioCue::ExplosionVariant4;
    }
}

} // namespace drone::audio
