#include <drone/audio/sample_mixer.hpp>

#include <algorithm>
#include <array>
#include <limits>

namespace drone::audio {
namespace {

constexpr std::size_t cue_index(const AudioCue cue) noexcept {
    return static_cast<std::size_t>(cue);
}

// Q30 amplitude corresponding to Drone's exact DirectSound attenuation family
// 30 * (volume - 100), volume 0..100. Values are precomputed from
// 10^(attenuation_hundredths_db / 2000) and rounded once, so render results do
// not depend on a platform libm implementation.
constexpr std::array<std::uint32_t, 101> win32_game_volume_gain_q30{{
    33954698u, 35147939u, 36383114u, 37661696u, 38985209u, 40355234u, 41773404u, 43241412u,
    44761009u, 46334008u, 47962285u, 49647784u, 51392515u, 53198559u, 55068072u, 57003283u,
    59006502u, 61080118u, 63226606u, 65448526u, 67748529u, 70129359u, 72593857u, 75144962u,
    77785719u, 80519278u, 83348899u, 86277960u, 89309955u, 92448500u, 95697341u, 99060353u,
    102541548u, 106145080u, 109875248u, 113736503u, 117733450u, 121870858u, 126153664u, 130586977u,
    135176087u, 139926467u, 144843787u, 149933911u, 155202914u, 160657080u, 166302918u, 172147163u,
    178196787u, 184459009u, 190941298u, 197651389u, 204597287u, 211787279u, 219229942u, 226934158u,
    234909116u, 243164331u, 251709652u, 260555275u, 269711752u, 279190007u, 289001349u, 299157483u,
    309670525u, 320553018u, 331817945u, 343478747u, 355549334u, 368044108u, 380977976u, 394366367u,
    408225256u, 422571176u, 437421243u, 452793173u, 468705306u, 485176626u, 502226784u, 519876121u,
    538145694u, 557057300u, 576633501u, 596897651u, 617873928u, 639587356u, 662063842u, 685330200u,
    709414188u, 734344540u, 760150998u, 786864351u, 814516469u, 843140343u, 872770121u, 903441154u,
    935190034u, 968054638u, 1002074175u, 1037289233u, 1073741824u,
}};

constexpr std::uint32_t hmi_max_channel = 0x7FFFu;

std::uint32_t hmi_linear_gain_q30(const std::uint32_t value) noexcept {
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(value) * portable_audio_gain_one_q30 + hmi_max_channel / 2u) /
        hmi_max_channel);
}

bool voice_loops(const AudioRuntimeVoiceState& voice) noexcept {
    if (voice.loop_encoding == AudioLoopEncoding::DirectSoundPlayFlags) {
        return voice.loop_value == 1u;
    }
    if (voice.loop_encoding == AudioLoopEncoding::HmiLoopCount) {
        return voice.loop_value == 0xFFFFFFFFu;
    }
    return false;
}

std::uint32_t voice_playback_rate_hz(const AudioRuntimeVoiceState& voice,
                                     const PortablePcmSample& sample) noexcept {
    if (voice.has_sample_rate && voice.sample_rate_hz > 0 &&
        voice.sample_rate_hz <= std::numeric_limits<std::uint32_t>::max()) {
        return static_cast<std::uint32_t>(voice.sample_rate_hz);
    }
    return sample.default_playback_rate_hz != 0 ? sample.default_playback_rate_hz
                                                : sample.sample_rate_hz;
}

PortableStereoGainQ30 voice_sample_gain_q30(const OriginalAudioBackend backend,
                                             const AudioRuntimeVoiceState& voice,
                                             const PortablePcmSample& sample) noexcept {
    if (!voice.has_sample_volume) {
        return {sample.default_left_gain_q30, sample.default_right_gain_q30};
    }

    if (backend == OriginalAudioBackend::Win32DirectSound) {
        const auto gain = portable_directsound_gain_q30(voice.sample_volume);
        if (gain.has_value()) return {*gain, *gain};
        return {0, 0};
    }

    const auto gain = portable_hmi_packed_gain_q30(
        static_cast<std::uint32_t>(voice.sample_volume));
    return gain.value_or(PortableStereoGainQ30{0, 0});
}

std::int32_t sample_at(const PortablePcmSample& sample, const std::size_t frame,
                       const std::size_t channel) noexcept {
    if (sample.channels == 1) return sample.interleaved_samples[frame];
    return sample.interleaved_samples[frame * 2 + channel];
}

std::int32_t interpolate_sample(const PortablePcmSample& sample,
                                const std::uint64_t cursor_q32,
                                const std::size_t channel,
                                const bool looping) noexcept {
    const auto frames = sample.frame_count();
    if (frames == 0) return 0;

    const auto frame0 = static_cast<std::size_t>(cursor_q32 >> 32);
    const auto frac = static_cast<std::uint32_t>(cursor_q32 & 0xFFFFFFFFu);
    const auto next = frame0 + 1 < frames ? frame0 + 1 : (looping ? 0 : frame0);
    const auto s0 = sample_at(sample, frame0, channel);
    const auto s1 = sample_at(sample, next, channel);
    const auto delta = static_cast<std::int64_t>(s1) - s0;
    const auto interpolated_delta =
        (delta * static_cast<std::int64_t>(frac)) / static_cast<std::int64_t>(portable_audio_cursor_one_q32);
    return static_cast<std::int32_t>(static_cast<std::int64_t>(s0) + interpolated_delta);
}

std::int32_t apply_gain_q30(const std::int32_t sample,
                            const std::uint32_t gain_q30) noexcept {
    const auto scaled = static_cast<std::int64_t>(sample) * gain_q30;
    return static_cast<std::int32_t>(scaled / portable_audio_gain_one_q30);
}

std::uint32_t combine_gain_q30(const std::uint32_t first,
                               const std::uint32_t second) noexcept {
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(first) * second + (portable_audio_gain_one_q30 / 2u)) /
        portable_audio_gain_one_q30);
}

std::int16_t saturate_i16(const std::int64_t value) noexcept {
    if (value > std::numeric_limits<std::int16_t>::max()) {
        return std::numeric_limits<std::int16_t>::max();
    }
    if (value < std::numeric_limits<std::int16_t>::min()) {
        return std::numeric_limits<std::int16_t>::min();
    }
    return static_cast<std::int16_t>(value);
}

} // namespace

std::optional<std::uint32_t> portable_directsound_gain_q30(
    const std::int64_t attenuation_hundredths_db) noexcept {
    if (attenuation_hundredths_db < -3000 || attenuation_hundredths_db > 0 ||
        attenuation_hundredths_db % 30 != 0) {
        return std::nullopt;
    }
    const auto volume = static_cast<std::size_t>(attenuation_hundredths_db / 30 + 100);
    return win32_game_volume_gain_q30[volume];
}

std::optional<std::uint32_t> portable_win32_game_volume_gain_q30(
    const std::int32_t volume_0_to_100) noexcept {
    if (volume_0_to_100 < 0 || volume_0_to_100 > 100) return std::nullopt;
    return win32_game_volume_gain_q30[static_cast<std::size_t>(volume_0_to_100)];
}

std::optional<PortableStereoGainQ30> portable_hmi_packed_gain_q30(
    const std::uint32_t packed_left_right_volume) noexcept {
    const auto left = packed_left_right_volume & 0xFFFFu;
    const auto right = packed_left_right_volume >> 16;
    if (left > hmi_max_channel || right > hmi_max_channel) return std::nullopt;
    return PortableStereoGainQ30{hmi_linear_gain_q30(left), hmi_linear_gain_q30(right)};
}

std::optional<std::uint32_t> portable_hmi_master_gain_q30(
    const std::uint32_t master_volume_15_bit) noexcept {
    if (master_volume_15_bit > hmi_max_channel) return std::nullopt;
    return hmi_linear_gain_q30(master_volume_15_bit);
}

PortableAudioSampleMixer::PortableAudioSampleMixer(
    const OriginalAudioBackend backend) noexcept
    : runtime_(backend) {}

void PortableAudioSampleMixer::reset_voices() noexcept {
    runtime_.reset();
    for (auto& cue : win32_render_voices_) {
        for (auto& voice : cue) voice = {};
    }
    for (auto& voice : dos_render_voices_) voice = {};
}

bool PortableAudioSampleMixer::valid_sample(const PortablePcmSample& sample) const noexcept {
    return sample.sample_rate_hz != 0 && (sample.channels == 1 || sample.channels == 2) &&
           !sample.interleaved_samples.empty() &&
           sample.interleaved_samples.size() % sample.channels == 0 &&
           sample.default_left_gain_q30 <= portable_audio_gain_one_q30 &&
           sample.default_right_gain_q30 <= portable_audio_gain_one_q30;
}

bool PortableAudioSampleMixer::set_sample(const AudioCue cue, PortablePcmSample sample_value) {
    if (!valid_sample(sample_value) || runtime_.active_voice_count(cue) != 0) return false;
    samples_[cue_index(cue)] = std::move(sample_value);
    return true;
}

bool PortableAudioSampleMixer::clear_sample(const AudioCue cue) noexcept {
    if (runtime_.active_voice_count(cue) != 0) return false;
    samples_[cue_index(cue)].reset();
    return true;
}

bool PortableAudioSampleMixer::has_sample(const AudioCue cue) const noexcept {
    return samples_[cue_index(cue)].has_value();
}

const PortablePcmSample* PortableAudioSampleMixer::sample(const AudioCue cue) const noexcept {
    const auto& value = samples_[cue_index(cue)];
    return value.has_value() ? &*value : nullptr;
}

bool PortableAudioSampleMixer::valid_command_value(
    const AudioBackendCommand& command) const noexcept {
    switch (command.primitive) {
    case AudioBackendPrimitive::Play:
        if (command.loop_encoding == AudioLoopEncoding::DirectSoundPlayFlags) {
            return command.loop_value == 0u || command.loop_value == 1u;
        }
        if (command.loop_encoding == AudioLoopEncoding::HmiLoopCount) {
            return command.loop_value == 0u || command.loop_value == 0xFFFFFFFFu;
        }
        return false;
    case AudioBackendPrimitive::Stop:
        return true;
    case AudioBackendPrimitive::SetSampleVolume:
        if (command.backend == OriginalAudioBackend::Win32DirectSound) {
            return portable_directsound_gain_q30(command.control_value).has_value();
        }
        return portable_hmi_packed_gain_q30(
                   static_cast<std::uint32_t>(command.control_value))
            .has_value();
    case AudioBackendPrimitive::SetSampleRate:
        return command.control_value > 0 &&
               command.control_value <= std::numeric_limits<std::uint32_t>::max();
    case AudioBackendPrimitive::SetDigitalMasterVolume:
        return command.backend == OriginalAudioBackend::DosHmiSos &&
               portable_hmi_master_gain_q30(
                   static_cast<std::uint32_t>(command.control_value))
                   .has_value();
    }
    return false;
}

AudioRenderVoiceState* PortableAudioSampleMixer::render_voice_slot(
    const AudioRuntimeVoiceHandle& handle) noexcept {
    if (handle.backend != backend()) return nullptr;
    if (backend() == OriginalAudioBackend::Win32DirectSound) {
        if (handle.slot >= portable_win32_max_cue_voice_capacity) return nullptr;
        return &win32_render_voices_[cue_index(handle.cue)][handle.slot];
    }
    if (handle.slot >= dos_render_voices_.size()) return nullptr;
    return &dos_render_voices_[handle.slot];
}

const AudioRenderVoiceState* PortableAudioSampleMixer::render_voice_slot(
    const AudioRuntimeVoiceHandle& handle) const noexcept {
    if (handle.backend != backend()) return nullptr;
    if (backend() == OriginalAudioBackend::Win32DirectSound) {
        if (handle.slot >= portable_win32_max_cue_voice_capacity) return nullptr;
        return &win32_render_voices_[cue_index(handle.cue)][handle.slot];
    }
    if (handle.slot >= dos_render_voices_.size()) return nullptr;
    return &dos_render_voices_[handle.slot];
}

void PortableAudioSampleMixer::begin_render_voice(
    const AudioRuntimeVoiceHandle& handle) noexcept {
    auto* render = render_voice_slot(handle);
    if (render == nullptr) return;
    render->active = true;
    render->handle = handle;
    render->cursor_q32 = 0;
}

void PortableAudioSampleMixer::retire_render_voice(
    const AudioRuntimeVoiceHandle& handle) noexcept {
    auto* render = render_voice_slot(handle);
    if (render != nullptr && render->active && render->handle == handle) {
        *render = {};
    }
}

void PortableAudioSampleMixer::retire_win32_dedicated_cue(const AudioCue cue) noexcept {
    auto& render = win32_render_voices_[cue_index(cue)][0];
    render = {};
}

AudioSampleMixerCommandResult PortableAudioSampleMixer::execute(
    const AudioBackendCommand& command) noexcept {
    if (!valid_command_value(command)) {
        return {.status = AudioSampleMixerCommandStatus::InvalidCommandValue};
    }
    if (command.primitive == AudioBackendPrimitive::Play && !has_sample(command.cue)) {
        return {.status = AudioSampleMixerCommandStatus::SampleUnavailable};
    }

    const auto runtime_result = runtime_.execute(command);
    if (!runtime_result.applied()) {
        return {.status = AudioSampleMixerCommandStatus::RuntimeRejected,
                .runtime = runtime_result};
    }

    if (command.primitive == AudioBackendPrimitive::Play && runtime_result.voice.has_value()) {
        begin_render_voice(*runtime_result.voice);
    } else if (command.primitive == AudioBackendPrimitive::Stop) {
        if (runtime_result.voice.has_value()) {
            retire_render_voice(*runtime_result.voice);
        } else if (backend() == OriginalAudioBackend::Win32DirectSound) {
            retire_win32_dedicated_cue(command.cue);
        }
    }

    return {.status = AudioSampleMixerCommandStatus::Applied,
            .runtime = runtime_result};
}

AudioSampleRenderResult PortableAudioSampleMixer::render_stereo_i16(
    const std::span<std::int16_t> output,
    const std::uint32_t output_sample_rate_hz) noexcept {
    if (output_sample_rate_hz == 0) {
        return {.status = AudioSampleRenderStatus::InvalidOutputRate};
    }
    if (output.size() % 2 != 0) {
        return {.status = AudioSampleRenderStatus::InvalidOutputBuffer};
    }

    std::fill(output.begin(), output.end(), std::int16_t{0});
    const auto frame_count = output.size() / 2;
    std::size_t completed = 0;

    const auto render_one = [&](AudioRenderVoiceState& render,
                                const std::size_t out_frame,
                                std::int64_t& left_sum,
                                std::int64_t& right_sum) {
        if (!render.active) return;
        const auto* voice = runtime_.voice_state(render.handle);
        if (voice == nullptr || !voice->active) {
            render = {};
            return;
        }
        const auto* pcm = sample(render.handle.cue);
        if (pcm == nullptr || pcm->frame_count() == 0) {
            (void)runtime_.complete_voice(render.handle);
            render = {};
            ++completed;
            return;
        }

        const auto looping = voice_loops(*voice);
        const auto total_q32 = static_cast<std::uint64_t>(pcm->frame_count()) *
                               portable_audio_cursor_one_q32;
        if (render.cursor_q32 >= total_q32) {
            if (looping) {
                render.cursor_q32 %= total_q32;
            } else {
                (void)runtime_.complete_voice(render.handle);
                render = {};
                ++completed;
                return;
            }
        }

        auto gain = voice_sample_gain_q30(backend(), *voice, *pcm);
        if (backend() == OriginalAudioBackend::DosHmiSos) {
            const auto master = portable_hmi_master_gain_q30(
                static_cast<std::uint32_t>(runtime_.digital_master_volume()))
                                    .value_or(0u);
            gain.left = combine_gain_q30(gain.left, master);
            gain.right = combine_gain_q30(gain.right, master);
        }

        const auto left = interpolate_sample(*pcm, render.cursor_q32, 0, looping);
        const auto right = interpolate_sample(
            *pcm, render.cursor_q32, pcm->channels == 1 ? 0 : 1, looping);
        left_sum += apply_gain_q30(left, gain.left);
        right_sum += apply_gain_q30(right, gain.right);

        const auto rate = voice_playback_rate_hz(*voice, *pcm);
        const auto step_q32 =
            (static_cast<std::uint64_t>(rate) * portable_audio_cursor_one_q32) /
            output_sample_rate_hz;
        render.cursor_q32 += step_q32;

        if (looping) {
            render.cursor_q32 %= total_q32;
        } else if (render.cursor_q32 >= total_q32) {
            const auto handle = render.handle;
            (void)runtime_.complete_voice(handle);
            render = {};
            ++completed;
        }
        (void)out_frame;
    };

    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        std::int64_t left_sum = 0;
        std::int64_t right_sum = 0;

        if (backend() == OriginalAudioBackend::Win32DirectSound) {
            for (auto& cue : win32_render_voices_) {
                for (auto& render : cue) {
                    render_one(render, frame, left_sum, right_sum);
                }
            }
        } else {
            for (auto& render : dos_render_voices_) {
                render_one(render, frame, left_sum, right_sum);
            }
        }

        output[frame * 2] = saturate_i16(left_sum);
        output[frame * 2 + 1] = saturate_i16(right_sum);
    }

    return {
        .status = AudioSampleRenderStatus::Rendered,
        .frames_rendered = frame_count,
        .voices_completed = completed,
    };
}

std::size_t PortableAudioSampleMixer::active_render_voice_count() const noexcept {
    std::size_t count = 0;
    if (backend() == OriginalAudioBackend::Win32DirectSound) {
        for (const auto& cue : win32_render_voices_) {
            count += static_cast<std::size_t>(std::count_if(
                cue.begin(), cue.end(), [](const auto& voice) { return voice.active; }));
        }
    } else {
        count = static_cast<std::size_t>(std::count_if(
            dos_render_voices_.begin(), dos_render_voices_.end(),
            [](const auto& voice) { return voice.active; }));
    }
    return count;
}

const AudioRenderVoiceState* PortableAudioSampleMixer::render_voice_state(
    const AudioRuntimeVoiceHandle& handle) const noexcept {
    const auto* render = render_voice_slot(handle);
    if (render == nullptr || !render->active || render->handle != handle) return nullptr;
    return render;
}

} // namespace drone::audio
