#include <drone/audio/voice_runtime.hpp>

#include <cassert>
#include <cstdint>
#include <vector>

using namespace drone::audio;

namespace {

AudioBackendCommand lower_or_die(const OriginalAudioBackend backend, const AudioEvent& event) {
    const auto lowered = lower_audio_event_for_original_backend(backend, event);
    assert(lowered.has_value());
    return *lowered;
}

} // namespace

int main() {
    // Win32 reusable pools are per cue, choose first inactive, and steal slot 0
    // only when that one cue's 20 buffers are all active.
    PortableAudioVoiceRuntime win32{OriginalAudioBackend::Win32DirectSound};
    const auto missile_play = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::RapidMissileFire, AudioAction::Play});

    std::vector<AudioRuntimeVoiceHandle> missile_handles;
    for (std::size_t i = 0; i < 20; ++i) {
        const auto result = win32.execute(missile_play);
        assert(result.applied());
        assert(result.voice.has_value());
        assert(result.voice->slot == i);
        assert(!result.saturated);
        assert(!result.stole_voice_zero);
        missile_handles.push_back(*result.voice);
    }
    assert(win32.active_voice_count(AudioCue::RapidMissileFire) == 20);

    const auto old_zero = missile_handles.front();
    const auto stolen = win32.execute(missile_play);
    assert(stolen.applied());
    assert(stolen.voice.has_value() && stolen.voice->slot == 0);
    assert(stolen.saturated && stolen.stole_voice_zero);
    assert(stolen.voice->generation != old_zero.generation);
    assert(!win32.complete_voice(old_zero));
    assert(win32.active_voice_count(AudioCue::RapidMissileFire) == 20);

    // Natural completion makes that exact slot the next first-free selection.
    assert(win32.complete_voice(missile_handles[5]));
    const auto reused_five = win32.execute(missile_play);
    assert(reused_five.applied());
    assert(reused_five.voice.has_value() && reused_five.voice->slot == 5);
    assert(!reused_five.saturated);

    // The 20-buffer capacity is per cue, not a global Win32 mixer capacity.
    const auto shield_play = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::ShieldPulse, AudioAction::Play});
    const auto shield_first = win32.execute(shield_play);
    assert(shield_first.applied());
    assert(shield_first.voice.has_value() && shield_first.voice->slot == 0);
    assert(win32.active_voice_count(AudioCue::ShieldPulse) == 1);
    assert(win32.active_voice_count() == 21);

    // DirectSound dedicated-buffer controls can be applied before Play and are
    // properties of the resource, so Play preserves them.
    const auto lowbees_volume_zero = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::MainMenuLowBees, AudioAction::SetVolume, 0});
    assert(win32.execute(lowbees_volume_zero).applied());
    const auto lowbees_play = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::MainMenuLowBees, AudioAction::Play});
    const auto lowbees_started = win32.execute(lowbees_play);
    assert(lowbees_started.applied() && lowbees_started.voice.has_value());
    const auto* lowbees_state = win32.voice_state(*lowbees_started.voice);
    assert(lowbees_state != nullptr && lowbees_state->active);
    assert(lowbees_state->has_sample_volume && lowbees_state->sample_volume == -3000);
    assert(lowbees_state->loop_encoding == AudioLoopEncoding::DirectSoundPlayFlags);
    assert(lowbees_state->loop_value == 1);

    const auto air_rate = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::AirLoop, AudioAction::SetFrequency, 11025});
    assert(win32.execute(air_rate).applied());
    const auto air_play_win32 = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::AirLoop, AudioAction::Play});
    const auto air_started_win32 = win32.execute(air_play_win32);
    assert(air_started_win32.applied() && air_started_win32.voice.has_value());
    const auto* air_win32_state = win32.voice_state(*air_started_win32.voice);
    assert(air_win32_state != nullptr && air_win32_state->has_sample_rate);
    assert(air_win32_state->sample_rate_hz == 11025);

    const auto air_stop_win32 = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::AirLoop, AudioAction::StopAndRewind});
    assert(win32.execute(air_stop_win32).applied());
    air_win32_state = win32.voice_state(*air_started_win32.voice);
    assert(air_win32_state != nullptr && !air_win32_state->active);
    assert(air_win32_state->rewound_after_stop);

    // Pool control without a concrete slot would be ambiguous and is rejected
    // rather than silently applying to an arbitrary DirectSound voice.
    auto invalid_pool_control = missile_play;
    invalid_pool_control.primitive = AudioBackendPrimitive::SetSampleVolume;
    invalid_pool_control.control_value = -1500;
    assert(win32.execute(invalid_pool_control).status ==
           AudioBackendExecutionStatus::UnsupportedCommand);

    // DOS has one global 32-voice array. Allocation is first inactive and a
    // saturated mixer fails rather than stealing any active voice.
    PortableAudioVoiceRuntime dos{OriginalAudioBackend::DosHmiSos};
    assert(dos.has_digital_master_volume());
    assert(dos.digital_master_volume() == 0x7FFF);

    const auto dos_one_shot = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::RapidMissileFire, AudioAction::Play});
    std::vector<AudioRuntimeVoiceHandle> dos_handles;
    for (std::size_t i = 0; i < 32; ++i) {
        const auto result = dos.execute(dos_one_shot);
        assert(result.applied());
        assert(result.voice.has_value() && result.voice->slot == i);
        assert(!result.saturated && !result.stole_voice_zero);
        dos_handles.push_back(*result.voice);
    }
    assert(dos.active_voice_count() == 32);
    const auto* dos_one_shot_state = dos.voice_state(dos_handles.front());
    assert(dos_one_shot_state != nullptr);
    assert(dos_one_shot_state->loop_encoding == AudioLoopEncoding::HmiLoopCount);
    assert(dos_one_shot_state->loop_value == 0);
    const auto dos_full = dos.execute(dos_one_shot);
    assert(dos_full.status == AudioBackendExecutionStatus::VoiceUnavailable);
    assert(dos_full.saturated);
    assert(!dos_full.voice.has_value());
    assert(dos.active_voice_count() == 32);

    const auto old_dos_ten = dos_handles[10];
    assert(dos.complete_voice(old_dos_ten));
    const auto dos_reused_ten = dos.execute(dos_one_shot);
    assert(dos_reused_ten.applied());
    assert(dos_reused_ten.voice.has_value() && dos_reused_ten.voice->slot == 10);
    assert(dos_reused_ten.voice->generation != old_dos_ten.generation);
    assert(!dos.complete_voice(old_dos_ten));

    // Capacity is one global DOS mixer budget across cues, not 32 voices per cue.
    PortableAudioVoiceRuntime dos_global{OriginalAudioBackend::DosHmiSos};
    for (std::size_t i = 0; i < 31; ++i) {
        assert(dos_global.execute(dos_one_shot).applied());
    }
    const auto dos_shield = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::ShieldPulse, AudioAction::Play});
    const auto dos_global_last = dos_global.execute(dos_shield);
    assert(dos_global_last.applied());
    assert(dos_global_last.voice.has_value() && dos_global_last.voice->slot == 31);
    const auto dos_hint = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::DroneHintOneShot, AudioAction::Play});
    const auto dos_global_full = dos_global.execute(dos_hint);
    assert(dos_global_full.status == AudioBackendExecutionStatus::VoiceUnavailable);
    assert(dos_global.active_voice_count() == 32);
    assert(dos_global.active_voice_count(AudioCue::RapidMissileFire) == 31);
    assert(dos_global.active_voice_count(AudioCue::ShieldPulse) == 1);

    // Reset gives a clean HMI mixer and restores its established full master.
    dos.reset();
    assert(dos.active_voice_count() == 0);
    assert(dos.digital_master_volume() == 0x7FFF);

    const auto air_play_dos = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::Play});
    const auto air_started_dos = dos.execute(air_play_dos);
    assert(air_started_dos.applied() && air_started_dos.voice.has_value());
    const auto* air_dos_state = dos.voice_state(*air_started_dos.voice);
    assert(air_dos_state != nullptr && air_dos_state->active);
    assert(air_dos_state->loop_encoding == AudioLoopEncoding::HmiLoopCount);
    assert(air_dos_state->loop_value == 0xFFFFFFFFu);
    assert(!air_dos_state->has_sample_volume);

    const auto dos_air_volume = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::SetVolume,
                   static_cast<std::int32_t>(0x30003000u),
                   AudioValueDomain::DosHmiPackedChannelVolume});
    const auto volume_result = dos.execute(dos_air_volume);
    assert(volume_result.applied());
    assert(volume_result.voice == air_started_dos.voice);

    const auto dos_air_rate = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::SetFrequency, 11025});
    assert(dos.execute(dos_air_rate).applied());
    air_dos_state = dos.voice_state(*air_started_dos.voice);
    assert(air_dos_state != nullptr);
    assert(air_dos_state->has_sample_volume &&
           air_dos_state->sample_volume == static_cast<std::int64_t>(0x30003000u));
    assert(air_dos_state->has_sample_rate && air_dos_state->sample_rate_hz == 11025);

    const auto dos_master = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::SetMasterVolume, 0x1234,
                   AudioValueDomain::DosHmiMasterVolume15Bit});
    assert(dos.execute(dos_master).applied());
    assert(dos.digital_master_volume() == 0x1234);
    // Master attenuation leaves Air voice ownership untouched.
    assert(dos.voice_state(*air_started_dos.voice)->active);

    const auto air_stop_dos = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::StopAndRewind});
    const auto stopped_dos = dos.execute(air_stop_dos);
    assert(stopped_dos.applied());
    assert(stopped_dos.voice == air_started_dos.voice);
    air_dos_state = dos.voice_state(*air_started_dos.voice);
    assert(air_dos_state != nullptr && !air_dos_state->active);
    assert(!air_dos_state->rewound_after_stop);
    assert(dos.execute(dos_air_volume).status ==
           AudioBackendExecutionStatus::TargetVoiceNotFound);

    // A runtime never accepts commands lowered for the other original backend.
    assert(dos.execute(air_play_win32).status == AudioBackendExecutionStatus::BackendMismatch);
    assert(win32.execute(air_play_dos).status == AudioBackendExecutionStatus::BackendMismatch);

    return 0;
}
