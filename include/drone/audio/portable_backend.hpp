#pragma once

#include <drone/audio/audio_event.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace drone::audio {

enum class OriginalAudioBackend : std::uint8_t {
    Win32DirectSound,
    DosHmiSos,
};

enum class AudioVoiceTopology : std::uint8_t {
    DedicatedBuffer,
    PerCuePreduplicatedPool,
    GlobalDynamicVoiceArray,
};

enum class AudioSaturationBehavior : std::uint8_t {
    NotApplicable,
    StealVoiceZero,
    ReturnFailure,
};

enum class AudioLoopEncoding : std::uint8_t {
    None,
    DirectSoundPlayFlags,
    HmiLoopCount,
};

enum class AudioVolumeEncoding : std::uint8_t {
    Win32GameScaleToDirectSoundAttenuation,
    DosHmiPackedChannels,
};

enum class AudioPresentationAttenuationOwner : std::uint8_t {
    AirSample,
    DigitalMaster,
};

struct PortableAudioBackendContract {
    OriginalAudioBackend backend{};

    // Transient-overlap topology. Win32's 20-buffer capacity is per reusable
    // cue pool; DOS HMI's 32-voice capacity is shared by the digital mixer.
    AudioVoiceTopology transient_voice_topology{};
    std::size_t transient_voice_capacity = 0;
    AudioSaturationBehavior transient_saturation{};

    AudioLoopEncoding loop_encoding{};
    AudioVolumeEncoding sample_volume_encoding{};
    AudioPresentationAttenuationOwner overlay_attenuation_owner{};

    bool stop_semantics_include_explicit_rewind = false;
    bool retained_runtime_voice_handle = false;
    bool supports_sample_rate_control = false;
    bool supports_digital_master_volume = false;
};

enum class AudioBackendPrimitive : std::uint8_t {
    Play,
    Stop,
    SetSampleVolume,
    SetSampleRate,
    SetDigitalMasterVolume,
};

// Platform-facing command produced by lowering one semantic AudioEvent. It is
// still sample-data-free: AudioCue remains the asset identity and the host
// resolves/imports the actual original or replacement data separately.
struct AudioBackendCommand {
    OriginalAudioBackend backend{};
    AudioBackendPrimitive primitive{};
    AudioCue cue{};

    AudioVoiceTopology voice_topology = AudioVoiceTopology::DedicatedBuffer;
    std::size_t voice_capacity = 0;
    AudioSaturationBehavior saturation = AudioSaturationBehavior::NotApplicable;

    AudioLoopEncoding loop_encoding = AudioLoopEncoding::None;
    std::uint32_t loop_value = 0;
    bool rewind_after_stop = false;

    // Native platform payload for parameterized controls: DirectSound
    // attenuation, HMI packed channel volume, sample rate Hz, or HMI master
    // level depending on primitive/backend.
    std::int64_t control_value = 0;
};

[[nodiscard]] const PortableAudioBackendContract&
portable_audio_backend_contract(OriginalAudioBackend backend) noexcept;

[[nodiscard]] bool audio_cue_available_on_original_backend(
    OriginalAudioBackend backend, AudioCue cue) noexcept;

// Returns nullopt when a semantic event carries a value domain that the
// selected historically faithful backend cannot represent without guessing.
[[nodiscard]] std::optional<AudioBackendCommand> lower_audio_event_for_original_backend(
    OriginalAudioBackend backend, const AudioEvent& event) noexcept;

} // namespace drone::audio
