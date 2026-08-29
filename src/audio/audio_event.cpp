#include <drone/audio/audio_event.hpp>

#include <array>

namespace drone::audio {
namespace {

using Policy = AudioVoicePolicy;

constexpr std::array<AudioCueDefinition, 23> definitions{{
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
}};

static_assert(definitions.size() == static_cast<std::size_t>(AudioCue::MissionDetonate) + 1);

} // namespace

const AudioCueDefinition& audio_cue_definition(const AudioCue cue) noexcept {
    return definitions[static_cast<std::size_t>(cue)];
}

AudioCue trajectory_flight_cue(const std::uint8_t sound_index) noexcept {
    const auto clamped = sound_index < 14 ? sound_index : std::uint8_t{13};
    return static_cast<AudioCue>(
        static_cast<std::uint8_t>(AudioCue::TrajectoryFlight01) + clamped);
}

} // namespace drone::audio
