#pragma once

#include <drone/audio/original_directsound.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace drone::audio {

enum class AudioCue : std::uint8_t {
    RapidMissileFire,
    ShieldPulse,
    SpecialLoadCycle,
    SpecialLaunch,
    ProbeImpact,
    StingerImpact,
    PlayerHitExplosion,
    TrajectoryFlight01,
    TrajectoryFlight02,
    TrajectoryFlight03,
    TrajectoryFlight04,
    TrajectoryFlight05,
    TrajectoryFlight06,
    TrajectoryFlight07,
    TrajectoryFlight08,
    TrajectoryFlight09,
    TrajectoryFlight10,
    TrajectoryFlight11,
    TrajectoryFlight12,
    TrajectoryFlight13,
    TrajectoryFlight14,
    MissionDeepness,
    MissionDetonate,
};

enum class AudioAction : std::uint8_t {
    Play,
    StopAndRewind,
};

enum class AudioVoicePolicy : std::uint8_t {
    SingleBuffer,
    ReusablePool20,
};

struct AudioCueDefinition {
    std::string_view original_asset{};
    AudioVoicePolicy voice_policy = AudioVoicePolicy::SingleBuffer;
    std::int32_t original_volume_0_to_100 = -1; // -1 = not yet established here
    std::uint32_t original_frequency_hz = 0;     // 0 = preserve source/default
    std::uint32_t directsound_play_flags = 0;
};

struct AudioEvent {
    AudioCue cue = AudioCue::RapidMissileFire;
    AudioAction action = AudioAction::Play;

    friend constexpr bool operator==(const AudioEvent&, const AudioEvent&) = default;
};

inline constexpr std::size_t game_session_audio_event_capacity = 32;

struct AudioEventQueue {
    std::array<AudioEvent, game_session_audio_event_capacity> events{};
    std::size_t size = 0;
    bool overflowed = false;

    [[nodiscard]] bool push(const AudioEvent event) noexcept {
        if (size >= events.size()) {
            overflowed = true;
            return false;
        }
        events[size++] = event;
        return true;
    }

    [[nodiscard]] std::span<const AudioEvent> view() const noexcept {
        return std::span<const AudioEvent>(events.data(), size);
    }
};

[[nodiscard]] const AudioCueDefinition& audio_cue_definition(AudioCue cue) noexcept;

// Win32 transient-wave selection consumes rand()%14 and uses that value to
// choose one of fourteen 20-voice Squad pools. This helper keeps the existing
// gameplay sound_index 0..13 mapping type-safe without embedding asset bytes.
[[nodiscard]] AudioCue trajectory_flight_cue(std::uint8_t sound_index) noexcept;

} // namespace drone::audio
