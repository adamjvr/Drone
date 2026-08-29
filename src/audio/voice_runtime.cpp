#include <drone/audio/voice_runtime.hpp>

#include <drone/audio/original_hmi.hpp>

#include <algorithm>

namespace drone::audio {
namespace {

constexpr std::size_t cue_index(const AudioCue cue) noexcept {
    return static_cast<std::size_t>(cue);
}

void begin_voice(AudioRuntimeVoiceState& voice, const AudioCue cue,
                 const AudioBackendCommand& command, const std::uint64_t sequence,
                 const bool reset_controls) noexcept {
    voice.active = true;
    voice.cue = cue;
    ++voice.generation;
    if (voice.generation == 0) ++voice.generation;
    voice.start_sequence = sequence;
    voice.loop_encoding = command.loop_encoding;
    voice.loop_value = command.loop_value;
    voice.rewound_after_stop = false;
    if (reset_controls) {
        voice.has_sample_volume = false;
        voice.sample_volume = 0;
        voice.has_sample_rate = false;
        voice.sample_rate_hz = 0;
    }
}

AudioRuntimeVoiceHandle make_handle(const OriginalAudioBackend backend,
                                    const AudioRuntimeVoiceState& voice,
                                    const std::size_t slot) noexcept {
    return AudioRuntimeVoiceHandle{
        .backend = backend,
        .cue = voice.cue,
        .slot = static_cast<std::uint16_t>(slot),
        .generation = voice.generation,
    };
}

} // namespace

PortableAudioVoiceRuntime::PortableAudioVoiceRuntime(
    const OriginalAudioBackend backend) noexcept
    : backend_(backend) {
    reset();
}

void PortableAudioVoiceRuntime::reset() noexcept {
    for (auto& cue : win32_voices_) {
        for (auto& voice : cue) voice = {};
    }
    for (auto& voice : dos_voices_) voice = {};
    for (auto& handle : dos_retained_handles_) handle.reset();
    next_start_sequence_ = 1;

    if (backend_ == OriginalAudioBackend::DosHmiSos) {
        has_digital_master_volume_ = true;
        digital_master_volume_ = original_drone_dos_hmi_master_volume_full;
    } else {
        has_digital_master_volume_ = false;
        digital_master_volume_ = 0;
    }
}

AudioBackendExecutionResult PortableAudioVoiceRuntime::execute(
    const AudioBackendCommand& command) noexcept {
    if (command.backend != backend_) {
        return {.status = AudioBackendExecutionStatus::BackendMismatch};
    }

    switch (command.primitive) {
    case AudioBackendPrimitive::Play:
        return execute_play(command);
    case AudioBackendPrimitive::Stop:
        return execute_stop(command);
    case AudioBackendPrimitive::SetSampleVolume:
        return execute_sample_volume(command);
    case AudioBackendPrimitive::SetSampleRate:
        return execute_sample_rate(command);
    case AudioBackendPrimitive::SetDigitalMasterVolume:
        return execute_master_volume(command);
    }
    return {.status = AudioBackendExecutionStatus::UnsupportedCommand};
}

bool PortableAudioVoiceRuntime::validate_win32_play_contract(
    const AudioBackendCommand& command) const noexcept {
    if (command.voice_topology == AudioVoiceTopology::DedicatedBuffer) {
        return command.voice_capacity == 1 &&
               command.saturation == AudioSaturationBehavior::NotApplicable &&
               command.loop_encoding == AudioLoopEncoding::DirectSoundPlayFlags;
    }
    if (command.voice_topology == AudioVoiceTopology::PerCuePreduplicatedPool) {
        return command.voice_capacity == portable_win32_max_cue_voice_capacity &&
               command.saturation == AudioSaturationBehavior::StealVoiceZero &&
               command.loop_encoding == AudioLoopEncoding::DirectSoundPlayFlags;
    }
    return false;
}

bool PortableAudioVoiceRuntime::validate_dos_play_contract(
    const AudioBackendCommand& command) const noexcept {
    return command.voice_topology == AudioVoiceTopology::GlobalDynamicVoiceArray &&
           command.voice_capacity == portable_dos_hmi_voice_capacity &&
           command.saturation == AudioSaturationBehavior::ReturnFailure &&
           command.loop_encoding == AudioLoopEncoding::HmiLoopCount;
}

AudioBackendExecutionResult PortableAudioVoiceRuntime::execute_play(
    const AudioBackendCommand& command) noexcept {
    if (backend_ == OriginalAudioBackend::Win32DirectSound) {
        if (!validate_win32_play_contract(command)) {
            return {.status = AudioBackendExecutionStatus::UnsupportedCommand};
        }

        auto& voices = win32_voices_[cue_index(command.cue)];
        const std::size_t capacity = command.voice_capacity;
        std::size_t selected = capacity;
        for (std::size_t i = 0; i < capacity; ++i) {
            if (!voices[i].active) {
                selected = i;
                break;
            }
        }

        bool saturated = false;
        bool stole_zero = false;
        if (selected == capacity) {
            if (command.saturation != AudioSaturationBehavior::StealVoiceZero) {
                return {.status = AudioBackendExecutionStatus::VoiceUnavailable,
                        .saturated = true};
            }
            selected = 0;
            saturated = true;
            stole_zero = true;
        }

        // DirectSound buffer properties belong to the buffer resource and
        // survive Play calls. Do not clear previously applied volume/rate.
        begin_voice(voices[selected], command.cue, command, next_start_sequence_++, false);
        return {
            .status = AudioBackendExecutionStatus::Applied,
            .voice = make_handle(backend_, voices[selected], selected),
            .saturated = saturated,
            .stole_voice_zero = stole_zero,
        };
    }

    if (!validate_dos_play_contract(command)) {
        return {.status = AudioBackendExecutionStatus::UnsupportedCommand};
    }

    std::size_t selected = dos_voices_.size();
    for (std::size_t i = 0; i < dos_voices_.size(); ++i) {
        if (!dos_voices_[i].active) {
            selected = i;
            break;
        }
    }
    if (selected == dos_voices_.size()) {
        return {.status = AudioBackendExecutionStatus::VoiceUnavailable,
                .saturated = true};
    }

    // HMI StartSample copies a complete descriptor into the selected 0xF0
    // voice record, so runtime control fields from a previous occupant do not
    // survive slot reuse.
    begin_voice(dos_voices_[selected], command.cue, command, next_start_sequence_++, true);
    const auto handle = make_handle(backend_, dos_voices_[selected], selected);
    dos_retained_handles_[cue_index(command.cue)] = handle;
    return {.status = AudioBackendExecutionStatus::Applied, .voice = handle};
}

AudioBackendExecutionResult PortableAudioVoiceRuntime::execute_stop(
    const AudioBackendCommand& command) noexcept {
    if (backend_ == OriginalAudioBackend::Win32DirectSound) {
        if (command.voice_topology != AudioVoiceTopology::DedicatedBuffer ||
            command.voice_capacity != 1) {
            // Current semantic Stop events target original dedicated resources.
            // A pooled DirectSound stop would require an explicit runtime slot.
            return {.status = AudioBackendExecutionStatus::UnsupportedCommand};
        }
        auto& voice = win32_voices_[cue_index(command.cue)][0];
        voice.cue = command.cue;
        voice.active = false;
        voice.rewound_after_stop = command.rewind_after_stop;
        return {.status = AudioBackendExecutionStatus::Applied};
    }

    auto* voice = dos_retained_voice(command.cue);
    if (voice == nullptr) {
        return {.status = AudioBackendExecutionStatus::TargetVoiceNotFound};
    }
    const auto handle = dos_retained_handles_[cue_index(command.cue)];
    voice->active = false;
    voice->rewound_after_stop = false;
    dos_retained_handles_[cue_index(command.cue)].reset();
    return {.status = AudioBackendExecutionStatus::Applied, .voice = handle};
}

AudioBackendExecutionResult PortableAudioVoiceRuntime::execute_sample_volume(
    const AudioBackendCommand& command) noexcept {
    if (backend_ == OriginalAudioBackend::Win32DirectSound) {
        if (command.voice_topology != AudioVoiceTopology::DedicatedBuffer ||
            command.voice_capacity != 1) {
            return {.status = AudioBackendExecutionStatus::UnsupportedCommand};
        }
        auto& voice = win32_voices_[cue_index(command.cue)][0];
        voice.cue = command.cue;
        voice.has_sample_volume = true;
        voice.sample_volume = command.control_value;
        return {.status = AudioBackendExecutionStatus::Applied};
    }

    auto* voice = dos_retained_voice(command.cue);
    if (voice == nullptr) {
        return {.status = AudioBackendExecutionStatus::TargetVoiceNotFound};
    }
    voice->has_sample_volume = true;
    voice->sample_volume = command.control_value;
    return {.status = AudioBackendExecutionStatus::Applied,
            .voice = dos_retained_handles_[cue_index(command.cue)]};
}

AudioBackendExecutionResult PortableAudioVoiceRuntime::execute_sample_rate(
    const AudioBackendCommand& command) noexcept {
    if (backend_ == OriginalAudioBackend::Win32DirectSound) {
        if (command.voice_topology != AudioVoiceTopology::DedicatedBuffer ||
            command.voice_capacity != 1) {
            return {.status = AudioBackendExecutionStatus::UnsupportedCommand};
        }
        auto& voice = win32_voices_[cue_index(command.cue)][0];
        voice.cue = command.cue;
        voice.has_sample_rate = true;
        voice.sample_rate_hz = command.control_value;
        return {.status = AudioBackendExecutionStatus::Applied};
    }

    auto* voice = dos_retained_voice(command.cue);
    if (voice == nullptr) {
        return {.status = AudioBackendExecutionStatus::TargetVoiceNotFound};
    }
    voice->has_sample_rate = true;
    voice->sample_rate_hz = command.control_value;
    return {.status = AudioBackendExecutionStatus::Applied,
            .voice = dos_retained_handles_[cue_index(command.cue)]};
}

AudioBackendExecutionResult PortableAudioVoiceRuntime::execute_master_volume(
    const AudioBackendCommand& command) noexcept {
    if (backend_ != OriginalAudioBackend::DosHmiSos ||
        !has_digital_master_volume_) {
        return {.status = AudioBackendExecutionStatus::UnsupportedCommand};
    }
    digital_master_volume_ = command.control_value;
    return {.status = AudioBackendExecutionStatus::Applied};
}

AudioRuntimeVoiceState* PortableAudioVoiceRuntime::dos_retained_voice(
    const AudioCue cue) noexcept {
    const auto& retained = dos_retained_handles_[cue_index(cue)];
    if (!retained.has_value()) return nullptr;
    if (retained->slot >= dos_voices_.size()) return nullptr;
    auto& voice = dos_voices_[retained->slot];
    if (!voice.active || voice.cue != cue || voice.generation != retained->generation) {
        return nullptr;
    }
    return &voice;
}

const AudioRuntimeVoiceState* PortableAudioVoiceRuntime::dos_retained_voice(
    const AudioCue cue) const noexcept {
    const auto& retained = dos_retained_handles_[cue_index(cue)];
    if (!retained.has_value()) return nullptr;
    if (retained->slot >= dos_voices_.size()) return nullptr;
    const auto& voice = dos_voices_[retained->slot];
    if (!voice.active || voice.cue != cue || voice.generation != retained->generation) {
        return nullptr;
    }
    return &voice;
}

bool PortableAudioVoiceRuntime::complete_voice(
    const AudioRuntimeVoiceHandle& handle) noexcept {
    if (handle.backend != backend_) return false;

    if (backend_ == OriginalAudioBackend::Win32DirectSound) {
        if (handle.slot >= portable_win32_max_cue_voice_capacity) return false;
        auto& voice = win32_voices_[cue_index(handle.cue)][handle.slot];
        if (!voice.active || voice.cue != handle.cue ||
            voice.generation != handle.generation) {
            return false;
        }
        voice.active = false;
        voice.rewound_after_stop = false;
        return true;
    }

    if (handle.slot >= dos_voices_.size()) return false;
    auto& voice = dos_voices_[handle.slot];
    if (!voice.active || voice.cue != handle.cue ||
        voice.generation != handle.generation) {
        return false;
    }
    voice.active = false;
    const auto& retained = dos_retained_handles_[cue_index(handle.cue)];
    if (retained.has_value() && *retained == handle) {
        dos_retained_handles_[cue_index(handle.cue)].reset();
    }
    return true;
}

const AudioRuntimeVoiceState* PortableAudioVoiceRuntime::voice_state(
    const AudioRuntimeVoiceHandle& handle) const noexcept {
    if (handle.backend != backend_) return nullptr;
    if (backend_ == OriginalAudioBackend::Win32DirectSound) {
        if (handle.slot >= portable_win32_max_cue_voice_capacity) return nullptr;
        const auto& voice = win32_voices_[cue_index(handle.cue)][handle.slot];
        if (voice.cue != handle.cue || voice.generation != handle.generation) return nullptr;
        return &voice;
    }
    if (handle.slot >= dos_voices_.size()) return nullptr;
    const auto& voice = dos_voices_[handle.slot];
    if (voice.cue != handle.cue || voice.generation != handle.generation) return nullptr;
    return &voice;
}

std::size_t PortableAudioVoiceRuntime::active_voice_count() const noexcept {
    if (backend_ == OriginalAudioBackend::Win32DirectSound) {
        std::size_t count = 0;
        for (const auto& cue : win32_voices_) {
            count += static_cast<std::size_t>(std::count_if(
                cue.begin(), cue.end(), [](const auto& voice) { return voice.active; }));
        }
        return count;
    }
    return static_cast<std::size_t>(std::count_if(
        dos_voices_.begin(), dos_voices_.end(), [](const auto& voice) { return voice.active; }));
}

std::size_t PortableAudioVoiceRuntime::active_voice_count(const AudioCue cue) const noexcept {
    if (backend_ == OriginalAudioBackend::Win32DirectSound) {
        const auto& voices = win32_voices_[cue_index(cue)];
        return static_cast<std::size_t>(std::count_if(
            voices.begin(), voices.end(), [](const auto& voice) { return voice.active; }));
    }
    return static_cast<std::size_t>(std::count_if(
        dos_voices_.begin(), dos_voices_.end(),
        [cue](const auto& voice) { return voice.active && voice.cue == cue; }));
}

} // namespace drone::audio
