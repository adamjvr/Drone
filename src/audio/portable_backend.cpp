#include <drone/audio/portable_backend.hpp>

#include <drone/audio/original_directsound.hpp>
#include <drone/audio/original_hmi.hpp>

namespace drone::audio {
namespace {

constexpr PortableAudioBackendContract win32_contract{
    .backend = OriginalAudioBackend::Win32DirectSound,
    .transient_voice_topology = AudioVoiceTopology::PerCuePreduplicatedPool,
    .transient_voice_capacity = original_sfx_voice_pool_capacity,
    .transient_saturation = AudioSaturationBehavior::StealVoiceZero,
    .loop_encoding = AudioLoopEncoding::DirectSoundPlayFlags,
    .sample_volume_encoding = AudioVolumeEncoding::Win32GameScaleToDirectSoundAttenuation,
    .overlay_attenuation_owner = AudioPresentationAttenuationOwner::AirSample,
    .stop_semantics_include_explicit_rewind = true,
    .retained_runtime_voice_handle = false,
    .supports_sample_rate_control = true,
    .supports_digital_master_volume = false,
};

constexpr PortableAudioBackendContract dos_contract{
    .backend = OriginalAudioBackend::DosHmiSos,
    .transient_voice_topology = AudioVoiceTopology::GlobalDynamicVoiceArray,
    .transient_voice_capacity = original_drone_dos_hmi_voice_count,
    .transient_saturation = AudioSaturationBehavior::ReturnFailure,
    .loop_encoding = AudioLoopEncoding::HmiLoopCount,
    .sample_volume_encoding = AudioVolumeEncoding::DosHmiPackedChannels,
    .overlay_attenuation_owner = AudioPresentationAttenuationOwner::DigitalMaster,
    .stop_semantics_include_explicit_rewind = false,
    .retained_runtime_voice_handle = true,
    .supports_sample_rate_control = true,
    .supports_digital_master_volume = true,
};

bool dos_cue_uses_infinite_loop(const AudioCue cue) noexcept {
    switch (cue) {
    case AudioCue::AirLoop:
    case AudioCue::MainMenuLowBees:
    case AudioCue::DroneApproachLoop:
    case AudioCue::LidTopBossLoop:
    case AudioCue::GeminiBossLoop:
    case AudioCue::OrderingInformation:
        return true;
    default:
        return false;
    }
}

AudioBackendCommand base_command(const OriginalAudioBackend backend,
                                 const AudioEvent& event) noexcept {
    AudioBackendCommand command{};
    command.backend = backend;
    command.cue = event.cue;

    if (backend == OriginalAudioBackend::Win32DirectSound) {
        const auto& definition = audio_cue_definition(event.cue);
        if (definition.voice_policy == AudioVoicePolicy::ReusablePool20) {
            command.voice_topology = AudioVoiceTopology::PerCuePreduplicatedPool;
            command.voice_capacity = original_sfx_voice_pool_capacity;
            command.saturation = AudioSaturationBehavior::StealVoiceZero;
        } else {
            command.voice_topology = AudioVoiceTopology::DedicatedBuffer;
            command.voice_capacity = 1;
            command.saturation = AudioSaturationBehavior::NotApplicable;
        }
    } else {
        command.voice_topology = AudioVoiceTopology::GlobalDynamicVoiceArray;
        command.voice_capacity = original_drone_dos_hmi_voice_count;
        command.saturation = AudioSaturationBehavior::ReturnFailure;
    }
    return command;
}

} // namespace

const PortableAudioBackendContract&
portable_audio_backend_contract(const OriginalAudioBackend backend) noexcept {
    return backend == OriginalAudioBackend::Win32DirectSound ? win32_contract : dos_contract;
}

bool audio_cue_available_on_original_backend(const OriginalAudioBackend backend,
                                             const AudioCue cue) noexcept {
    if (backend == OriginalAudioBackend::Win32DirectSound) return true;

    // The checked-in cross-build asset census has no DOS counterpart for these
    // two Win32 cues. Reject them instead of fabricating a DOS asset mapping.
    return cue != AudioCue::ResultsHiphop && cue != AudioCue::CompletionCredits;
}

std::optional<AudioBackendCommand> lower_audio_event_for_original_backend(
    const OriginalAudioBackend backend, const AudioEvent& event) noexcept {
    if (!audio_cue_available_on_original_backend(backend, event.cue)) return std::nullopt;

    auto command = base_command(backend, event);
    const auto& definition = audio_cue_definition(event.cue);

    switch (event.action) {
    case AudioAction::Play:
        command.primitive = AudioBackendPrimitive::Play;
        if (backend == OriginalAudioBackend::Win32DirectSound) {
            command.loop_encoding = AudioLoopEncoding::DirectSoundPlayFlags;
            command.loop_value = definition.directsound_play_flags;
        } else {
            command.loop_encoding = AudioLoopEncoding::HmiLoopCount;
            command.loop_value = dos_cue_uses_infinite_loop(event.cue)
                                     ? original_drone_dos_hmi_infinite_loop_count
                                     : original_drone_dos_hmi_one_shot_loop_count;
        }
        return command;

    case AudioAction::StopAndRewind:
        command.primitive = AudioBackendPrimitive::Stop;
        command.rewind_after_stop = backend == OriginalAudioBackend::Win32DirectSound;
        return command;

    case AudioAction::SetVolume:
        command.primitive = AudioBackendPrimitive::SetSampleVolume;
        if (backend == OriginalAudioBackend::Win32DirectSound &&
            event.value_domain == AudioValueDomain::Win32GameVolume0To100) {
            command.control_value = original_directsound_attenuation(event.value);
            return command;
        }
        if (backend == OriginalAudioBackend::DosHmiSos &&
            event.value_domain == AudioValueDomain::DosHmiPackedChannelVolume) {
            command.control_value = static_cast<std::uint32_t>(event.value);
            return command;
        }
        return std::nullopt;

    case AudioAction::SetFrequency:
        if (event.value_domain != AudioValueDomain::FrequencyHz) return std::nullopt;
        command.primitive = AudioBackendPrimitive::SetSampleRate;
        command.control_value = event.value;
        return command;

    case AudioAction::SetMasterVolume:
        if (backend != OriginalAudioBackend::DosHmiSos ||
            event.value_domain != AudioValueDomain::DosHmiMasterVolume15Bit) {
            return std::nullopt;
        }
        command.primitive = AudioBackendPrimitive::SetDigitalMasterVolume;
        command.control_value = static_cast<std::uint32_t>(event.value) &
                                original_drone_dos_hmi_master_volume_mask;
        return command;
    }

    return std::nullopt;
}

} // namespace drone::audio
