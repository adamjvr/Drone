#include <drone/audio/sample_mixer.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

namespace {

using namespace drone::audio;

AudioBackendCommand lower_or_die(const OriginalAudioBackend backend,
                                 const AudioEvent& event) {
    const auto lowered = lower_audio_event_for_original_backend(backend, event);
    assert(lowered.has_value());
    return *lowered;
}

PortablePcmSample mono_sample(const std::uint32_t rate,
                              std::initializer_list<std::int16_t> frames) {
    PortablePcmSample sample{};
    sample.sample_rate_hz = rate;
    sample.channels = 1;
    sample.interleaved_samples.assign(frames.begin(), frames.end());
    return sample;
}

std::int32_t apply_q30(const std::int32_t sample, const std::uint32_t gain) {
    return static_cast<std::int32_t>(
        (static_cast<std::int64_t>(sample) * gain) / portable_audio_gain_one_q30);
}

} // namespace

int main() {
    using namespace drone::audio;

    // Fixed-point gain helpers are exact and reject values outside the proven
    // original domains rather than guessing a conversion.
    assert(portable_win32_game_volume_gain_q30(100) == portable_audio_gain_one_q30);
    assert(portable_win32_game_volume_gain_q30(50) == 190941298u);
    assert(portable_directsound_gain_q30(0) == portable_audio_gain_one_q30);
    assert(portable_directsound_gain_q30(-1500) == 190941298u);
    assert(!portable_directsound_gain_q30(-1499).has_value());
    assert(!portable_win32_game_volume_gain_q30(-1).has_value());

    const auto hmi_half = portable_hmi_packed_gain_q30(0x40004000u);
    assert(hmi_half.has_value());
    assert(hmi_half->left == hmi_half->right);
    assert(!portable_hmi_packed_gain_q30(0x80000000u).has_value());
    assert(portable_hmi_master_gain_q30(0x7FFFu) == portable_audio_gain_one_q30);
    assert(!portable_hmi_master_gain_q30(0x8000u).has_value());

    // Missing PCM never consumes a policy-runtime voice.
    PortableAudioSampleMixer win32{OriginalAudioBackend::Win32DirectSound};
    const auto missile_play = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::RapidMissileFire, AudioAction::Play});
    const auto missing = win32.execute(missile_play);
    assert(missing.status == AudioSampleMixerCommandStatus::SampleUnavailable);
    assert(win32.voice_runtime().active_voice_count() == 0);

    // Native-rate one-shot rendering is exact and natural end-of-sample retires
    // both the renderer cursor and the policy voice.
    assert(win32.set_sample(AudioCue::RapidMissileFire,
                            mono_sample(4, {0, 1000, 2000, 3000})));
    const auto missile_started = win32.execute(missile_play);
    assert(missile_started.applied() && missile_started.runtime.voice.has_value());
    std::array<std::int16_t, 8> native_output{};
    const auto native_render = win32.render_stereo_i16(native_output, 4);
    assert(native_render.status == AudioSampleRenderStatus::Rendered);
    assert(native_render.frames_rendered == 4);
    assert(native_render.voices_completed == 1);
    assert((native_output == std::array<std::int16_t, 8>{
                                 0, 0, 1000, 1000, 2000, 2000, 3000, 3000}));
    assert(win32.active_render_voice_count() == 0);
    assert(win32.voice_runtime().active_voice_count() == 0);

    // A sample cannot be replaced while a live generation owns it.
    const auto missile_again = win32.execute(missile_play);
    assert(missile_again.applied());
    assert(!win32.set_sample(AudioCue::RapidMissileFire, mono_sample(4, {7, 8})));
    std::array<std::int16_t, 8> finish_again{};
    assert(win32.render_stereo_i16(finish_again, 4).voices_completed == 1);
    assert(win32.set_sample(AudioCue::RapidMissileFire, mono_sample(4, {7, 8})));

    // Q32.32 cursor stepping + integer linear interpolation. A 2-Hz default
    // playback rate rendered at 4 Hz advances half a source frame per output.
    auto resample = mono_sample(4, {0, 1000, 2000});
    resample.default_playback_rate_hz = 2;
    assert(win32.set_sample(AudioCue::ProbeImpact, std::move(resample)));
    const auto probe_play = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::ProbeImpact, AudioAction::Play});
    assert(win32.execute(probe_play).applied());
    std::array<std::int16_t, 12> resampled{};
    const auto resampled_result = win32.render_stereo_i16(resampled, 4);
    assert(resampled_result.voices_completed == 1);
    assert((resampled == std::array<std::int16_t, 12>{
                             0, 0, 500, 500, 1000, 1000,
                             1500, 1500, 2000, 2000, 2000, 2000}));

    // Win32 dedicated-buffer volume controls override the sample's load-time
    // default gain. Game volume 50 is DirectSound -1500 hundredths dB.
    assert(win32.set_sample(AudioCue::MainMenuLowBees, mono_sample(1, {10000})));
    const auto lowbees_volume = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::MainMenuLowBees, AudioAction::SetVolume, 50});
    assert(win32.execute(lowbees_volume).applied());
    const auto lowbees_play = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::MainMenuLowBees, AudioAction::Play});
    assert(win32.execute(lowbees_play).applied());
    std::array<std::int16_t, 2> attenuated{};
    assert(win32.render_stereo_i16(attenuated, 1).status == AudioSampleRenderStatus::Rendered);
    const auto expected_50 = apply_q30(10000, *portable_win32_game_volume_gain_q30(50));
    assert(attenuated[0] == expected_50 && attenuated[1] == expected_50);
    // Loop voice remains alive after one complete sample-length render frame.
    assert(win32.active_render_voice_count() == 1);

    // Explicit Stop removes the render cursor immediately; DirectSound rewind
    // semantics remain in PortableAudioVoiceRuntime.
    const auto lowbees_stop = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::MainMenuLowBees, AudioAction::StopAndRewind});
    assert(win32.execute(lowbees_stop).applied());
    assert(win32.active_render_voice_count() == 0);

    // Full-sample looping wraps in Q32.32 space without natural completion.
    assert(win32.set_sample(AudioCue::AirLoop, mono_sample(2, {1000, 2000})));
    const auto air_play_win32 = lower_or_die(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::AirLoop, AudioAction::Play});
    const auto air_started = win32.execute(air_play_win32);
    assert(air_started.applied() && air_started.runtime.voice.has_value());
    std::array<std::int16_t, 10> looped{};
    const auto looped_result = win32.render_stereo_i16(looped, 2);
    assert(looped_result.voices_completed == 0);
    assert((looped == std::array<std::int16_t, 10>{
                          1000, 1000, 2000, 2000, 1000, 1000,
                          2000, 2000, 1000, 1000}));
    assert(win32.render_voice_state(*air_started.runtime.voice) != nullptr);

    // Summing is done before signed-16-bit saturation.
    PortableAudioSampleMixer clipping{OriginalAudioBackend::Win32DirectSound};
    assert(clipping.set_sample(AudioCue::RapidMissileFire, mono_sample(1, {25000})));
    assert(clipping.execute(missile_play).applied());
    assert(clipping.execute(missile_play).applied());
    std::array<std::int16_t, 2> clipped{};
    const auto clipped_result = clipping.render_stereo_i16(clipped, 1);
    assert(clipped_result.voices_completed == 2);
    assert(clipped[0] == 32767 && clipped[1] == 32767);

    // DOS sample-channel volume and digital master volume are independent
    // linear fixed-point stages. Air ownership survives master attenuation.
    PortableAudioSampleMixer dos{OriginalAudioBackend::DosHmiSos};
    assert(dos.set_sample(AudioCue::AirLoop, mono_sample(1, {10000})));
    const auto air_play_dos = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::Play});
    const auto dos_air = dos.execute(air_play_dos);
    assert(dos_air.applied() && dos_air.runtime.voice.has_value());

    const auto dos_sample_volume = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::SetVolume,
                   static_cast<std::int32_t>(0x40002000u),
                   AudioValueDomain::DosHmiPackedChannelVolume});
    assert(dos.execute(dos_sample_volume).applied());
    const auto dos_master = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::SetMasterVolume, 0x4000,
                   AudioValueDomain::DosHmiMasterVolume15Bit});
    assert(dos.execute(dos_master).applied());

    std::array<std::int16_t, 2> dos_attenuated{};
    assert(dos.render_stereo_i16(dos_attenuated, 1).status ==
           AudioSampleRenderStatus::Rendered);
    const auto sample_gain = *portable_hmi_packed_gain_q30(0x40002000u);
    const auto master_gain = *portable_hmi_master_gain_q30(0x4000u);
    const auto expected_left = apply_q30(apply_q30(10000, sample_gain.left), master_gain);
    const auto expected_right = apply_q30(apply_q30(10000, sample_gain.right), master_gain);
    // Renderer combines Q30 stages before the sample multiply, so permit the
    // one-LSB difference possible versus two sequential integer truncations.
    assert(dos_attenuated[0] >= expected_left - 1 && dos_attenuated[0] <= expected_left + 1);
    assert(dos_attenuated[1] >= expected_right - 1 && dos_attenuated[1] <= expected_right + 1);
    assert(dos.voice_runtime().voice_state(*dos_air.runtime.voice)->active);
    assert(dos.active_render_voice_count() == 1);

    // Rate controls override a sample's default playback rate on the retained
    // DOS voice without changing its sample identity.
    const auto dos_rate = lower_or_die(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::SetFrequency, 2});
    assert(dos.execute(dos_rate).applied());
    const auto* dos_render_state = dos.render_voice_state(*dos_air.runtime.voice);
    assert(dos_render_state != nullptr);
    const auto cursor_before = dos_render_state->cursor_q32;
    std::array<std::int16_t, 2> rate_frame{};
    assert(dos.render_stereo_i16(rate_frame, 4).status == AudioSampleRenderStatus::Rendered);
    dos_render_state = dos.render_voice_state(*dos_air.runtime.voice);
    assert(dos_render_state != nullptr);
    assert(dos_render_state->cursor_q32 ==
           (cursor_before + portable_audio_cursor_one_q32 / 2) % portable_audio_cursor_one_q32);

    // Invalid render buffers/rates and out-of-contract native controls are
    // rejected without mutating voice allocation.
    std::array<std::int16_t, 3> odd{};
    assert(dos.render_stereo_i16(odd, 44100).status == AudioSampleRenderStatus::InvalidOutputBuffer);
    std::array<std::int16_t, 2> valid_size{};
    assert(dos.render_stereo_i16(valid_size, 0).status == AudioSampleRenderStatus::InvalidOutputRate);
    auto invalid_volume = dos_sample_volume;
    invalid_volume.control_value = static_cast<std::int64_t>(0x80008000u);
    assert(dos.execute(invalid_volume).status == AudioSampleMixerCommandStatus::InvalidCommandValue);

    return 0;
}
