#pragma once

#include <drone/audio/voice_runtime.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace drone::audio {

inline constexpr std::uint32_t portable_audio_gain_one_q30 = 1u << 30;
inline constexpr std::uint64_t portable_audio_cursor_one_q32 = std::uint64_t{1} << 32;

struct PortablePcmSample {
    // Canonical host-independent decode format. Asset importers may convert
    // original WAV/CLV payloads into signed 16-bit mono/stereo frames before
    // handing them to the mixer; no OS audio object is stored here.
    std::uint32_t sample_rate_hz = 0;
    std::uint8_t channels = 0; // 1 = mono, 2 = stereo
    std::vector<std::int16_t> interleaved_samples{};

    // Original-backend load-time properties. A zero default playback rate means
    // use sample_rate_hz. Q30 unity is exactly 1.0.
    std::uint32_t default_playback_rate_hz = 0;
    std::uint32_t default_left_gain_q30 = portable_audio_gain_one_q30;
    std::uint32_t default_right_gain_q30 = portable_audio_gain_one_q30;

    [[nodiscard]] std::size_t frame_count() const noexcept {
        return channels == 0 ? 0 : interleaved_samples.size() / channels;
    }
};

struct PortableStereoGainQ30 {
    std::uint32_t left = portable_audio_gain_one_q30;
    std::uint32_t right = portable_audio_gain_one_q30;
};

// Deterministic gain helpers used by asset importers and by the renderer. The
// DirectSound helper accepts only the exact 0..100 game-scale attenuation family
// established in Drone: attenuation = 30 * (volume - 100).
[[nodiscard]] std::optional<std::uint32_t> portable_directsound_gain_q30(
    std::int64_t attenuation_hundredths_db) noexcept;
[[nodiscard]] std::optional<std::uint32_t> portable_win32_game_volume_gain_q30(
    std::int32_t volume_0_to_100) noexcept;
[[nodiscard]] std::optional<PortableStereoGainQ30> portable_hmi_packed_gain_q30(
    std::uint32_t packed_left_right_volume) noexcept;
[[nodiscard]] std::optional<std::uint32_t> portable_hmi_master_gain_q30(
    std::uint32_t master_volume_15_bit) noexcept;

enum class AudioSampleMixerCommandStatus : std::uint8_t {
    Applied,
    SampleUnavailable,
    InvalidCommandValue,
    RuntimeRejected,
};

struct AudioSampleMixerCommandResult {
    AudioSampleMixerCommandStatus status = AudioSampleMixerCommandStatus::RuntimeRejected;
    AudioBackendExecutionResult runtime{};

    [[nodiscard]] constexpr bool applied() const noexcept {
        return status == AudioSampleMixerCommandStatus::Applied && runtime.applied();
    }
};

enum class AudioSampleRenderStatus : std::uint8_t {
    Rendered,
    InvalidOutputRate,
    InvalidOutputBuffer,
};

struct AudioSampleRenderResult {
    AudioSampleRenderStatus status = AudioSampleRenderStatus::Rendered;
    std::size_t frames_rendered = 0;
    std::size_t voices_completed = 0;
};

struct AudioRenderVoiceState {
    bool active = false;
    AudioRuntimeVoiceHandle handle{};
    std::uint64_t cursor_q32 = 0;
};

// Host-independent signed-16-bit stereo renderer above PortableAudioVoiceRuntime.
// It owns no device, thread, decoder or wall clock. Commands update deterministic
// voice state; render_stereo_i16() advances sample cursors only when the caller
// explicitly asks it to render frames.
class PortableAudioSampleMixer {
public:
    explicit PortableAudioSampleMixer(OriginalAudioBackend backend) noexcept;

    [[nodiscard]] OriginalAudioBackend backend() const noexcept { return runtime_.backend(); }
    [[nodiscard]] const PortableAudioVoiceRuntime& voice_runtime() const noexcept {
        return runtime_;
    }

    void reset_voices() noexcept;

    // Samples may be installed/replaced only while that cue has no active voice.
    // This keeps sample identity stable for every live handle/generation.
    [[nodiscard]] bool set_sample(AudioCue cue, PortablePcmSample sample);
    [[nodiscard]] bool clear_sample(AudioCue cue) noexcept;
    [[nodiscard]] bool has_sample(AudioCue cue) const noexcept;
    [[nodiscard]] const PortablePcmSample* sample(AudioCue cue) const noexcept;

    [[nodiscard]] AudioSampleMixerCommandResult execute(
        const AudioBackendCommand& command) noexcept;

    // Output is interleaved stereo signed-16-bit. The buffer length must be even.
    // Mixing is integer/fixed-point and ends with signed-16-bit saturation.
    [[nodiscard]] AudioSampleRenderResult render_stereo_i16(
        std::span<std::int16_t> interleaved_stereo_output,
        std::uint32_t output_sample_rate_hz) noexcept;

    [[nodiscard]] std::size_t active_render_voice_count() const noexcept;
    [[nodiscard]] const AudioRenderVoiceState* render_voice_state(
        const AudioRuntimeVoiceHandle& handle) const noexcept;

private:
    using Win32CueRenderVoices =
        std::array<AudioRenderVoiceState, portable_win32_max_cue_voice_capacity>;

    [[nodiscard]] bool valid_sample(const PortablePcmSample& sample) const noexcept;
    [[nodiscard]] bool valid_command_value(const AudioBackendCommand& command) const noexcept;
    [[nodiscard]] AudioRenderVoiceState* render_voice_slot(
        const AudioRuntimeVoiceHandle& handle) noexcept;
    [[nodiscard]] const AudioRenderVoiceState* render_voice_slot(
        const AudioRuntimeVoiceHandle& handle) const noexcept;
    void begin_render_voice(const AudioRuntimeVoiceHandle& handle) noexcept;
    void retire_render_voice(const AudioRuntimeVoiceHandle& handle) noexcept;
    void retire_win32_dedicated_cue(AudioCue cue) noexcept;

    PortableAudioVoiceRuntime runtime_;
    std::array<std::optional<PortablePcmSample>, portable_audio_cue_count> samples_{};
    std::array<Win32CueRenderVoices, portable_audio_cue_count> win32_render_voices_{};
    std::array<AudioRenderVoiceState, portable_dos_hmi_voice_capacity> dos_render_voices_{};
};

} // namespace drone::audio
