#pragma once

#include <drone/audio/portable_backend.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace drone::audio {

inline constexpr std::size_t portable_audio_cue_count =
    static_cast<std::size_t>(AudioCue::ParachuteOneShot) + 1;
inline constexpr std::size_t portable_win32_max_cue_voice_capacity = 20;
inline constexpr std::size_t portable_dos_hmi_voice_capacity = 32;

enum class AudioBackendExecutionStatus : std::uint8_t {
    Applied,
    VoiceUnavailable,
    TargetVoiceNotFound,
    BackendMismatch,
    UnsupportedCommand,
};

struct AudioRuntimeVoiceHandle {
    OriginalAudioBackend backend{};
    AudioCue cue{};
    std::uint16_t slot = 0;
    std::uint32_t generation = 0;

    friend constexpr bool operator==(const AudioRuntimeVoiceHandle&,
                                     const AudioRuntimeVoiceHandle&) = default;
};

struct AudioRuntimeVoiceState {
    bool active = false;
    AudioCue cue = AudioCue::RapidMissileFire;
    std::uint32_t generation = 0;
    std::uint64_t start_sequence = 0;

    AudioLoopEncoding loop_encoding = AudioLoopEncoding::None;
    std::uint32_t loop_value = 0;

    bool has_sample_volume = false;
    std::int64_t sample_volume = 0;
    bool has_sample_rate = false;
    std::int64_t sample_rate_hz = 0;

    // True only when the most recent explicit Stop operation on this resource
    // included the original DirectSound rewind-to-zero semantic.
    bool rewound_after_stop = false;
};

struct AudioBackendExecutionResult {
    AudioBackendExecutionStatus status = AudioBackendExecutionStatus::UnsupportedCommand;
    std::optional<AudioRuntimeVoiceHandle> voice{};
    bool saturated = false;
    bool stole_voice_zero = false;

    [[nodiscard]] constexpr bool applied() const noexcept {
        return status == AudioBackendExecutionStatus::Applied;
    }
};

// Host-independent execution state for the already-lowered original-backend
// commands. This class models ownership/arbitration/control state only; it does
// not contain PCM data or advance playback cursors. Hosts retire naturally
// completed voices through complete_voice().
class PortableAudioVoiceRuntime {
public:
    explicit PortableAudioVoiceRuntime(OriginalAudioBackend backend) noexcept;

    [[nodiscard]] OriginalAudioBackend backend() const noexcept { return backend_; }

    void reset() noexcept;

    [[nodiscard]] AudioBackendExecutionResult execute(
        const AudioBackendCommand& command) noexcept;

    // Mark a voice naturally complete (for example, after the host reaches the
    // end of a one-shot sample). Generation validation prevents an old handle
    // from retiring a slot that has already been reused/stolen.
    [[nodiscard]] bool complete_voice(const AudioRuntimeVoiceHandle& handle) noexcept;

    [[nodiscard]] const AudioRuntimeVoiceState* voice_state(
        const AudioRuntimeVoiceHandle& handle) const noexcept;

    [[nodiscard]] std::size_t active_voice_count() const noexcept;
    [[nodiscard]] std::size_t active_voice_count(AudioCue cue) const noexcept;

    [[nodiscard]] bool has_digital_master_volume() const noexcept {
        return has_digital_master_volume_;
    }
    [[nodiscard]] std::int64_t digital_master_volume() const noexcept {
        return digital_master_volume_;
    }

private:
    using Win32CueVoices =
        std::array<AudioRuntimeVoiceState, portable_win32_max_cue_voice_capacity>;

    [[nodiscard]] AudioBackendExecutionResult execute_play(
        const AudioBackendCommand& command) noexcept;
    [[nodiscard]] AudioBackendExecutionResult execute_stop(
        const AudioBackendCommand& command) noexcept;
    [[nodiscard]] AudioBackendExecutionResult execute_sample_volume(
        const AudioBackendCommand& command) noexcept;
    [[nodiscard]] AudioBackendExecutionResult execute_sample_rate(
        const AudioBackendCommand& command) noexcept;
    [[nodiscard]] AudioBackendExecutionResult execute_master_volume(
        const AudioBackendCommand& command) noexcept;

    [[nodiscard]] AudioRuntimeVoiceState* dos_retained_voice(AudioCue cue) noexcept;
    [[nodiscard]] const AudioRuntimeVoiceState* dos_retained_voice(AudioCue cue) const noexcept;
    [[nodiscard]] bool validate_win32_play_contract(const AudioBackendCommand& command) const noexcept;
    [[nodiscard]] bool validate_dos_play_contract(const AudioBackendCommand& command) const noexcept;

    OriginalAudioBackend backend_{};
    std::array<Win32CueVoices, portable_audio_cue_count> win32_voices_{};
    std::array<AudioRuntimeVoiceState, portable_dos_hmi_voice_capacity> dos_voices_{};
    std::array<std::optional<AudioRuntimeVoiceHandle>, portable_audio_cue_count>
        dos_retained_handles_{};

    std::uint64_t next_start_sequence_ = 1;
    bool has_digital_master_volume_ = false;
    std::int64_t digital_master_volume_ = 0;
};

} // namespace drone::audio
