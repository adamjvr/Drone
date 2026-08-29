#include <drone/audio/audio_event.hpp>
#include <drone/audio/original_directsound.hpp>
#include <drone/audio/original_hmi.hpp>
#include <drone/audio/portable_backend.hpp>
#include <drone/gameplay/game_session.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>

using namespace drone::audio;

int main() {
    static_assert(original_sfx_voice_pool_capacity == 20);
    static_assert(original_directsound_attenuation(100) == 0);
    static_assert(original_directsound_attenuation(90) == -300);
    static_assert(original_directsound_attenuation(70) == -900);
    static_assert(original_directsound_attenuation(50) == -1500);

    std::array<std::uint32_t, original_sfx_voice_pool_capacity> status{};
    status.fill(directsound_status_playing);
    status[7] = 0;
    assert(select_original_sfx_voice(status) == 7);

    // The helper compares raw status exactly to 1; extra DirectSound status bits
    // therefore make a slot reusable in the original implementation.
    status.fill(directsound_status_playing);
    status[3] = 3;
    assert(select_original_sfx_voice(status) == 3);

    status.fill(directsound_status_playing);
    assert(select_original_sfx_voice(status) == 0);

    const auto& missile = audio_cue_definition(AudioCue::RapidMissileFire);
    assert(missile.original_asset == "missile.wav");
    assert(missile.voice_policy == AudioVoicePolicy::ReusablePool20);
    assert(missile.original_volume_0_to_100 == 50);
    assert(missile.original_frequency_hz == 22050);

    const auto& shield = audio_cue_definition(AudioCue::ShieldPulse);
    assert(shield.original_asset == "shields.wav");
    assert(shield.original_frequency_hz == 11025);

    const auto& launch = audio_cue_definition(AudioCue::SpecialLaunch);
    assert(launch.original_asset == "probe3.wav");
    assert(launch.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(launch.original_volume_0_to_100 == 70);

    const auto& player_hit = audio_cue_definition(AudioCue::PlayerHitExplosion);
    assert(player_hit.original_asset == "bigexp3.wav");
    assert(player_hit.original_frequency_hz == 15000);

    const auto& bomb = audio_cue_definition(AudioCue::EnemyBombFire);
    assert(bomb.original_asset == "missile.wav");
    assert(bomb.voice_policy == AudioVoicePolicy::ReusablePool20);
    assert(bomb.original_volume_0_to_100 == 50);
    assert(bomb.original_frequency_hz == 0);

    const auto& results = audio_cue_definition(AudioCue::ResultsChoral);
    assert(results.original_asset == "choral.wav");
    assert(results.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(results.directsound_play_flags == 0);

    const auto& air_loop = audio_cue_definition(AudioCue::AirLoop);
    assert(air_loop.original_asset == "air.wav");
    assert(air_loop.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(air_loop.original_volume_0_to_100 == original_air_loop_loaded_volume);
    assert(air_loop.directsound_play_flags == directsound_play_looping_flag);
    static_assert(original_air_loop_loaded_volume == 50);
    static_assert(original_air_loop_volume_cap == 50);
    static_assert(original_air_loop_restart_volume == 0);
    static_assert(original_air_loop_fade_boundary == 60);
    static_assert(original_air_loop_menu_frequency_hz == 11025);
    static_assert(original_main_menu_lowbees_start_volume == 0);
    static_assert(original_main_menu_lowbees_volume_cap == 80);

    const auto& menu_loop = audio_cue_definition(AudioCue::MainMenuLowBees);
    assert(menu_loop.original_asset == "lowbees.wav");
    assert(menu_loop.original_volume_0_to_100 == 0);
    assert(menu_loop.directsound_play_flags == directsound_play_looping_flag);

    const auto& drone_loop = audio_cue_definition(AudioCue::DroneApproachLoop);
    assert(drone_loop.original_asset == "drone.wav");
    assert(drone_loop.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(drone_loop.original_volume_0_to_100 == original_drone_loop_loaded_volume);
    assert(drone_loop.directsound_play_flags == directsound_play_looping_flag);
    static_assert(original_drone_loop_loaded_volume == 90);
    static_assert(original_drone_loop_start_volume == 0);
    static_assert(original_drone_loop_approach_volume_cap == 80);
    static_assert(original_drone_loop_phase2_decode_volume == 60);
    static_assert(original_drone_loop_interrupted_decode_volume == 80);

    const auto& drone_hint = audio_cue_definition(AudioCue::DroneHintOneShot);
    assert(drone_hint.original_asset == "hintdron.wav");
    assert(drone_hint.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(drone_hint.original_volume_0_to_100 == original_hintdron_volume);
    assert(drone_hint.directsound_play_flags == 0);

    const auto& parachute = audio_cue_definition(AudioCue::ParachuteOneShot);
    assert(parachute.original_asset == "parachut.wav");
    assert(parachute.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(parachute.original_volume_0_to_100 == original_parachut_volume);
    assert(parachute.directsound_play_flags == 0);
    static_assert(original_hintdron_volume == 80);
    static_assert(original_parachut_volume == 60);

    const auto& lid_top_loop = audio_cue_definition(AudioCue::LidTopBossLoop);
    assert(lid_top_loop.original_asset == "retro1.wav");
    assert(lid_top_loop.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(lid_top_loop.original_volume_0_to_100 == 70);
    assert(lid_top_loop.directsound_play_flags == directsound_play_looping_flag);

    const auto& gemini_loop = audio_cue_definition(AudioCue::GeminiBossLoop);
    assert(gemini_loop.original_asset == "gemini.wav");
    assert(gemini_loop.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(gemini_loop.original_volume_0_to_100 == 100);
    assert(gemini_loop.directsound_play_flags == directsound_play_looping_flag);

    const auto& level1 = audio_cue_definition(AudioCue::LidTopLevel1Cadence);
    assert(level1.original_asset == "level1.wav");
    assert(level1.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(level1.original_volume_0_to_100 == original_lid_top_level1_volume);
    assert(level1.directsound_play_flags == 0);

    const auto& level2 = audio_cue_definition(AudioCue::GeminiLevel2Cadence);
    assert(level2.original_asset == "level2.wav");
    assert(level2.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(level2.original_volume_0_to_100 == original_gemini_level2_volume);
    assert(level2.directsound_play_flags == 0);
    static_assert(original_boss_traversal_sound_period == 8);

    const auto& ordering = audio_cue_definition(AudioCue::OrderingInformation);
    assert(ordering.original_asset == "thunder2.wav");
    assert(ordering.original_volume_0_to_100 == -1);
    assert(ordering.directsound_play_flags == directsound_play_looping_flag);

    const auto& credits = audio_cue_definition(AudioCue::CompletionCredits);
    assert(credits.original_asset == "credits.wav");
    assert(credits.directsound_play_flags == directsound_play_looping_flag);

    const auto loop_sites = original_directsound_loop_call_sites();
    assert(loop_sites.size() == 13);
    const auto literal_count = std::count_if(
        loop_sites.begin(), loop_sites.end(), [](const OriginalLoopCallSite& site) {
            return site.flag_proof == OriginalLoopFlagProof::LiteralOne;
        });
    const auto register_count = std::count_if(
        loop_sites.begin(), loop_sites.end(), [](const OriginalLoopCallSite& site) {
            return site.flag_proof == OriginalLoopFlagProof::RegisterProvenOne;
        });
    assert(literal_count == 8);
    assert(register_count == 5);
    const auto has_site = [&](const std::uint32_t va, const std::string_view asset) {
        return std::any_of(loop_sites.begin(), loop_sites.end(), [&](const auto& site) {
            return site.call_site_va == va && site.original_asset == asset;
        });
    };
    assert(has_site(0x0040474Du, "credits.wav"));
    assert(has_site(0x0040C8F3u, "air.wav"));
    assert(has_site(0x0040E52Fu, "drone.wav"));
    assert(has_site(0x00418B14u, "lowbees.wav"));
    assert(has_site(0x0041B75Bu, "thunder2.wav"));
    const auto unresolved = std::find_if(
        loop_sites.begin(), loop_sites.end(), [](const OriginalLoopCallSite& site) {
            return site.owner == OriginalLoopPlaybackOwner::RegisteredBossSlot2Unresolved;
        });
    assert(unresolved != loop_sites.end());
    assert(unresolved->call_site_va == 0x00407AA3u);
    assert(unresolved->original_asset.empty());

    assert(audio_cue_definition(AudioCue::ExplosionVariant2).original_asset == "explode2.wav");
    assert(audio_cue_definition(AudioCue::ExplosionVariant2).original_volume_0_to_100 == 60);
    assert(audio_cue_definition(AudioCue::ExplosionVariant3).original_asset == "explode3.wav");
    assert(audio_cue_definition(AudioCue::ExplosionVariant3).original_volume_0_to_100 == 50);
    assert(audio_cue_definition(AudioCue::ExplosionVariant4).original_asset == "explode4.wav");
    assert(audio_cue_definition(AudioCue::ExplosionVariant4).original_volume_0_to_100 == 50);

    OriginalAudioRuntimeState variant{};
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant2);
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant2);
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant3);
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant4);
    assert(next_original_explosion_sfx_cue(variant) == AudioCue::ExplosionVariant2);

    drone::gameplay::GameSession session{};
    (void)next_original_explosion_sfx_cue(session.original_audio);
    (void)next_original_explosion_sfx_cue(session.original_audio);
    assert(session.original_audio.explosion_sfx_variant_cycle == 2);
    session.original_audio.drone_loop_volume_0_to_100 = 60;
    session.original_audio.air_loop_volume_0_to_100 = 17;
    drone::gameplay::reset_game_session(
        session, drone::gameplay::GameplaySessionResetScope::FullCampaign);
    assert(session.original_audio.explosion_sfx_variant_cycle == 2);
    // Win32 0x00440278 is process-global just like the explosion selector;
    // campaign/encounter rebuilds do not reconstruct that audio scalar.
    assert(session.original_audio.drone_loop_volume_0_to_100 == 60);
    assert(session.original_audio.air_loop_volume_0_to_100 == 17);

    assert(trajectory_flight_cue(0) == AudioCue::TrajectoryFlight01);
    assert(trajectory_flight_cue(13) == AudioCue::TrajectoryFlight14);

    static_assert(original_trajectory_squad_volume == 80);
    const auto trajectory_pools = original_trajectory_pool_initializations();
    assert(trajectory_pools.size() == 14);
    for (std::size_t i = 0; i < trajectory_pools.size(); ++i) {
        const auto& pool = trajectory_pools[i];
        const auto cue = trajectory_flight_cue(static_cast<std::uint8_t>(i));
        const auto& definition = audio_cue_definition(cue);
        assert(pool.original_asset == definition.original_asset);
        assert(pool.original_volume_0_to_100 == original_trajectory_squad_volume);
        assert(pool.original_frequency_hz == 0);
        assert(definition.voice_policy == AudioVoicePolicy::ReusablePool20);
        assert(definition.original_volume_0_to_100 == original_trajectory_squad_volume);
        assert(definition.original_frequency_hz == 0);
        assert(definition.directsound_play_flags == 0);
        assert(pool.pool_storage_end_va > pool.pool_storage_begin_va);
        assert((pool.pool_storage_end_va - pool.pool_storage_begin_va) / sizeof(std::uint32_t) ==
               original_sfx_voice_pool_capacity);
    }
    assert(trajectory_pools.front().load_call_site_va == 0x0041FBE6u);
    assert(trajectory_pools.front().filename_literal_va == 0x0042BEF4u);
    assert(trajectory_pools.front().pool_storage_begin_va == 0x0042F1A8u);
    assert(trajectory_pools.front().pool_storage_end_va == 0x0042F1F8u);
    assert(trajectory_pools.back().load_call_site_va == 0x0041FFD8u);
    assert(trajectory_pools.back().filename_literal_va == 0x0042BE58u);
    assert(trajectory_pools.back().pool_storage_begin_va == 0x00441770u);
    assert(trajectory_pools.back().pool_storage_end_va == 0x004417C0u);

    // HMI S.O.S. middleware capability is intentionally modeled separately
    // from Drone's Win32 DirectSound pool construction. The public SOS 4.x
    // headers expose descriptor-backed samples, 32-voice library capacity and
    // independent volume/rate/pan/loop controls, but do not prove which voice
    // count or allocation/steal policy Drone selected in its DOS executable.
    const auto& hmi = original_hmi_sos_api_contract();
    static_assert(original_hmi_default_mixer_channels == 32);
    static_assert(original_hmi_max_voice_capability == 32);
    static_assert(original_hmi_sample_flag_active == 0x8000);
    static_assert(original_hmi_sample_flag_processed == 0x4000);
    static_assert(original_hmi_sample_flag_done == 0x2000);
    static_assert(original_hmi_sample_flag_loop == 0x1000);
    static_assert(original_hmi_pan_left == 0);
    static_assert(original_hmi_pan_center == 0x8000);
    static_assert(original_hmi_pan_right == 0xFFFF);
    assert(hmi.default_mixer_channels == 32);
    assert(hmi.max_voice_capability == 32);
    assert(hmi.descriptor_backed_start);
    assert(hmi.explicit_stop_by_handle);
    assert(hmi.explicit_stop_all_samples);
    assert(hmi.sample_done_query);
    assert(hmi.descriptor_has_volume);
    assert(hmi.descriptor_has_loop_count);
    assert(hmi.descriptor_has_sample_rate);
    assert(hmi.descriptor_has_pan);
    assert(hmi.descriptor_has_priority);
    assert(hmi.descriptor_has_completion_callbacks);
    assert(hmi.runtime_set_volume);
    assert(hmi.runtime_set_sample_rate);
    assert(hmi.runtime_set_pan);
    assert(!hmi.api_requires_preduplicated_sample_pool);
    assert(hmi.max_voice_capability != original_sfx_voice_pool_capacity);

    // Canonical DRONE_SW.EXE embeds the HMI mixer and explicitly configures
    // 32 x 0xF0-byte voice records (0x1E00 bytes). StartSample scans from
    // voice zero for the first inactive record and returns -1 when saturated;
    // it does not perform the Win32 slot-0 steal fallback.
    const auto& dos_hmi = original_drone_dos_hmi_runtime_contract();
    static_assert(original_drone_dos_hmi_voice_count == 32);
    static_assert(original_drone_dos_hmi_voice_record_size == 0xF0);
    static_assert(original_drone_dos_hmi_voice_storage_bytes == 32 * 0xF0);
    static_assert(original_drone_dos_hmi_infinite_loop_count == 0xFFFFFFFFu);
    static_assert(original_hmi_pack_equal_channel_volume(0x5200) == 0x52005200u);
    assert(dos_hmi.configured_voice_count == 32);
    assert(dos_hmi.voice_record_size == 0xF0);
    assert(dos_hmi.voice_storage_bytes == 0x1E00);
    assert(dos_hmi.first_inactive_voice_wins);
    assert(dos_hmi.saturation_policy == OriginalDroneDosVoiceSaturationPolicy::ReturnFailure);
    assert(!dos_hmi.priority_used_for_voice_selection);
    assert(dos_hmi.start_copies_full_sample_descriptor);
    assert(dos_hmi.start_returns_voice_index);
    assert(dos_hmi.packed_left_right_16_volume);
    assert(!dos_hmi.universal_normalized_volume_mapping);
    assert(dos_hmi.one_shot_loop_count == 0);
    assert(dos_hmi.infinite_loop_count == 0xFFFFFFFFu);
    assert(dos_hmi.infinite_loop_uses_loop_count_field);
    assert(dos_hmi.retained_voice_handles_for_runtime_control);
    assert(original_drone_dos_hmi_start_sample_va == 0x0008AC82u);
    assert(original_drone_dos_hmi_stop_sample_va == 0x0008AE02u);
    assert(original_drone_dos_hmi_set_sample_volume_va == 0x0008AFC1u);
    assert(original_drone_dos_hmi_set_sample_rate_va == 0x0008B2A7u);
    assert(original_drone_dos_hmi_sample_done_va == 0x0008B549u);
    assert(original_drone_dos_hmi_init_driver_va == 0x0008D4AFu);
    assert(original_drone_dos_hmi_voice_count_write_va == 0x0008D78Au);
    assert(dos_hmi.master_digital_volume_control_va == 0x0008AF88u);
    assert(dos_hmi.master_volume_full == 0x7FFFu);
    assert(dos_hmi.master_volume_mask == 0x7FFFu);
    assert(dos_hmi.master_control_is_distinct_from_sample_volume);

    // DOS presentation ownership diverges materially from Win32. Lowbees is
    // a retained main-menu voice, while pause/quit/nine-lives attenuate the
    // HMI-wide digital master and leave the gameplay Air voice running.
    const auto& dos_presentation = original_drone_dos_presentation_audio_contract();
    assert(dos_presentation.lowbees_descriptor_va == 0x004CBE0u);
    assert(dos_presentation.lowbees_voice_handle_va == 0x004CBF0u);
    assert(dos_presentation.lowbees_restart_byte_va == 0x0083881u);
    assert(dos_presentation.lowbees_start_call_va == 0x00081709u);
    assert(dos_presentation.lowbees_stop_call_va == 0x00082C73u);
    assert(dos_presentation.ordering_lowbees_stop_call_va == 0x00082E2Fu);
    assert(dos_presentation.lowbees_initial_level == 0);
    assert(dos_presentation.lowbees_fade_step == 0x007Du);
    assert(dos_presentation.lowbees_fade_threshold == 0x7000u);
    assert(dos_presentation.lowbees_terminal_written_level == 0x704Eu);
    assert(dos_presentation.lowbees_loop_count == 0xFFFFFFFFu);

    assert(dos_presentation.air_descriptor_va == 0x004CBBCu);
    assert(dos_presentation.air_voice_handle_va == 0x004CCC8u);
    assert(dos_presentation.air_start_call_va == 0x00077737u);
    assert(dos_presentation.master_digital_volume_control_va == 0x0008AF88u);
    assert(dos_presentation.master_full_level == 0x7FFFu);
    assert(dos_presentation.active_master_fade_step == 0x0096u);
    assert(dos_presentation.overlay_master_fade_step == 0x015Eu);
    assert(dos_presentation.overlay_full_start_terminal_remainder == 0x00D9u);
    assert(dos_presentation.pause_raw_state == 5);
    assert(dos_presentation.quit_confirmation_raw_state == 6);
    assert(dos_presentation.nine_lives_raw_state == 99);
    assert(!dos_presentation.overlay_directly_controls_air_sample);
    assert(dos_presentation.air_voice_continues_through_overlay);
    assert(!dos_presentation.resume_state_2_restarts_air);
    assert(dos_presentation.resume_state_2_restores_master_full);
    assert(dos_presentation.teardown_air_sample_done_call_va == 0x0007DF24u);
    assert(dos_presentation.teardown_air_stop_call_va == 0x0007DF38u);
    assert(dos_presentation.teardown_checks_sample_done_before_stop);
    assert(!dos_presentation.air_survives_gameplay_to_menu_transition);

    assert(dos_presentation.menu_transition_count == 7);

    // The portable contract preserves original backend differences instead of
    // pretending DirectSound and HMI share one hidden mixer policy.
    const auto& win32_backend =
        portable_audio_backend_contract(OriginalAudioBackend::Win32DirectSound);
    assert(win32_backend.transient_voice_topology ==
           AudioVoiceTopology::PerCuePreduplicatedPool);
    assert(win32_backend.transient_voice_capacity == 20);
    assert(win32_backend.transient_saturation == AudioSaturationBehavior::StealVoiceZero);
    assert(win32_backend.loop_encoding == AudioLoopEncoding::DirectSoundPlayFlags);
    assert(win32_backend.sample_volume_encoding ==
           AudioVolumeEncoding::Win32GameScaleToDirectSoundAttenuation);
    assert(win32_backend.overlay_attenuation_owner ==
           AudioPresentationAttenuationOwner::AirSample);
    assert(win32_backend.stop_semantics_include_explicit_rewind);
    assert(!win32_backend.supports_digital_master_volume);

    const auto& dos_backend =
        portable_audio_backend_contract(OriginalAudioBackend::DosHmiSos);
    assert(dos_backend.transient_voice_topology == AudioVoiceTopology::GlobalDynamicVoiceArray);
    assert(dos_backend.transient_voice_capacity == 32);
    assert(dos_backend.transient_saturation == AudioSaturationBehavior::ReturnFailure);
    assert(dos_backend.loop_encoding == AudioLoopEncoding::HmiLoopCount);
    assert(dos_backend.sample_volume_encoding == AudioVolumeEncoding::DosHmiPackedChannels);
    assert(dos_backend.overlay_attenuation_owner ==
           AudioPresentationAttenuationOwner::DigitalMaster);
    assert(!dos_backend.stop_semantics_include_explicit_rewind);
    assert(dos_backend.retained_runtime_voice_handle);
    assert(dos_backend.supports_digital_master_volume);

    std::array<std::uint32_t, original_drone_dos_hmi_voice_count> dos_voice_flags{};
    dos_voice_flags.fill(original_hmi_sample_flag_active);
    dos_voice_flags[11] = original_hmi_sample_flag_done;
    const auto dos_voice = select_original_drone_dos_hmi_voice(dos_voice_flags);
    assert(dos_voice.has_value() && *dos_voice == 11);
    dos_voice_flags.fill(original_hmi_sample_flag_active);
    assert(!select_original_drone_dos_hmi_voice(dos_voice_flags).has_value());

    // Value-domain tagging is part of the interface: current Win32 producers
    // keep their exact game-scale volume scalars, while DOS faithful callers
    // can supply native HMI payloads without ambiguous unit conversion.
    const AudioEvent win32_volume{AudioCue::DroneApproachLoop, AudioAction::SetVolume, 60};
    assert(win32_volume.value_domain == AudioValueDomain::Win32GameVolume0To100);
    const AudioEvent frequency{AudioCue::AirLoop, AudioAction::SetFrequency, 11025};
    assert(frequency.value_domain == AudioValueDomain::FrequencyHz);
    const AudioEvent dos_volume{AudioCue::DroneApproachLoop, AudioAction::SetVolume,
                                static_cast<std::int32_t>(0x41004100u),
                                AudioValueDomain::DosHmiPackedChannelVolume};
    const AudioEvent dos_master{AudioCue::AirLoop, AudioAction::SetMasterVolume, 0x7FFF,
                                AudioValueDomain::DosHmiMasterVolume15Bit};

    auto command = lower_audio_event_for_original_backend(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::RapidMissileFire, AudioAction::Play});
    assert(command.has_value());
    assert(command->voice_topology == AudioVoiceTopology::PerCuePreduplicatedPool);
    assert(command->voice_capacity == 20);
    assert(command->saturation == AudioSaturationBehavior::StealVoiceZero);
    assert(command->loop_encoding == AudioLoopEncoding::DirectSoundPlayFlags);
    assert(command->loop_value == 0);

    command = lower_audio_event_for_original_backend(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::AirLoop, AudioAction::Play});
    assert(command.has_value() && command->loop_value == directsound_play_looping_flag);

    assert(audio_cue_available_on_original_backend(OriginalAudioBackend::DosHmiSos,
                                                   AudioCue::AirLoop));
    assert(!audio_cue_available_on_original_backend(OriginalAudioBackend::DosHmiSos,
                                                    AudioCue::ResultsHiphop));
    assert(!audio_cue_available_on_original_backend(OriginalAudioBackend::DosHmiSos,
                                                    AudioCue::CompletionCredits));
    assert(!lower_audio_event_for_original_backend(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::CompletionCredits, AudioAction::Play}).has_value());

    command = lower_audio_event_for_original_backend(
        OriginalAudioBackend::DosHmiSos, AudioEvent{AudioCue::AirLoop, AudioAction::Play});
    assert(command.has_value());
    assert(command->voice_topology == AudioVoiceTopology::GlobalDynamicVoiceArray);
    assert(command->voice_capacity == 32);
    assert(command->saturation == AudioSaturationBehavior::ReturnFailure);
    assert(command->loop_encoding == AudioLoopEncoding::HmiLoopCount);
    assert(command->loop_value == 0xFFFFFFFFu);

    command = lower_audio_event_for_original_backend(
        OriginalAudioBackend::Win32DirectSound,
        AudioEvent{AudioCue::AirLoop, AudioAction::StopAndRewind});
    assert(command.has_value() && command->primitive == AudioBackendPrimitive::Stop);
    assert(command->rewind_after_stop);
    command = lower_audio_event_for_original_backend(
        OriginalAudioBackend::DosHmiSos,
        AudioEvent{AudioCue::AirLoop, AudioAction::StopAndRewind});
    assert(command.has_value() && !command->rewind_after_stop);

    command = lower_audio_event_for_original_backend(
        OriginalAudioBackend::Win32DirectSound, win32_volume);
    assert(command.has_value());
    assert(command->primitive == AudioBackendPrimitive::SetSampleVolume);
    assert(command->control_value == -1200);
    assert(!lower_audio_event_for_original_backend(OriginalAudioBackend::DosHmiSos,
                                                   win32_volume).has_value());

    command = lower_audio_event_for_original_backend(OriginalAudioBackend::DosHmiSos,
                                                      dos_volume);
    assert(command.has_value());
    assert(command->primitive == AudioBackendPrimitive::SetSampleVolume);
    assert(command->control_value == 0x41004100u);
    assert(!lower_audio_event_for_original_backend(OriginalAudioBackend::Win32DirectSound,
                                                   dos_volume).has_value());

    command = lower_audio_event_for_original_backend(OriginalAudioBackend::DosHmiSos,
                                                      frequency);
    assert(command.has_value());
    assert(command->primitive == AudioBackendPrimitive::SetSampleRate);
    assert(command->control_value == 11025);

    command = lower_audio_event_for_original_backend(OriginalAudioBackend::DosHmiSos,
                                                      dos_master);
    assert(command.has_value());
    assert(command->primitive == AudioBackendPrimitive::SetDigitalMasterVolume);
    assert(command->control_value == 0x7FFF);
    assert(!lower_audio_event_for_original_backend(OriginalAudioBackend::Win32DirectSound,
                                                   dos_master).has_value());

    const auto* menu = dos_presentation.menu_transitions;
    assert(menu != nullptr);
    assert(menu[0].selection == OriginalDroneDosMenuSelection::PlayGame);
    assert(menu[0].writes_raw_state && menu[0].raw_state == 2);
    assert(menu[0].stops_lowbees && menu[0].frees_lowbees_descriptor &&
           menu[0].rearms_lowbees_restart);
    assert(menu[1].selection == OriginalDroneDosMenuSelection::Instructions);
    assert(menu[1].writes_raw_state && menu[1].raw_state == 3);
    assert(!menu[1].stops_lowbees && menu[1].modal_function_va == 0x00085A04u &&
           menu[1].modal_returns_raw_state_4);
    assert(menu[2].selection == OriginalDroneDosMenuSelection::OrderingInformation);
    assert(menu[2].writes_raw_state && menu[2].raw_state == 7);
    assert(menu[2].stops_lowbees && menu[2].uses_ordering_specific_cleanup &&
           menu[2].modal_function_va == 0x00085D10u && menu[2].modal_returns_raw_state_4);
    assert(menu[3].selection == OriginalDroneDosMenuSelection::HighScores);
    assert(menu[3].writes_raw_state && menu[3].raw_state == 8);
    assert(!menu[3].stops_lowbees && menu[3].modal_function_va == 0x00086E04u &&
           menu[3].modal_returns_raw_state_4);
    assert(menu[4].selection == OriginalDroneDosMenuSelection::ConfigureJoystick);
    assert(!menu[4].writes_raw_state && !menu[4].stops_lowbees);
    assert(menu[5].selection == OriginalDroneDosMenuSelection::PlayDemo);
    assert(menu[5].writes_raw_state && menu[5].raw_state == 13 && menu[5].stops_lowbees);
    assert(menu[6].selection == OriginalDroneDosMenuSelection::Quit);
    assert(menu[6].writes_raw_state && menu[6].raw_state == 0 && menu[6].stops_lowbees);

    AudioEventQueue queue{};
    assert(queue.push({AudioCue::RapidMissileFire, AudioAction::Play}));
    assert(queue.push({AudioCue::SpecialLaunch, AudioAction::StopAndRewind}));
    assert(queue.push({AudioCue::DroneApproachLoop, AudioAction::SetVolume, 60}));
    assert(queue.push({AudioCue::AirLoop, AudioAction::SetFrequency, 11025}));
    assert(queue.size == 4);
    assert(!queue.overflowed);
    assert((queue.view()[0] == AudioEvent{AudioCue::RapidMissileFire, AudioAction::Play}));
    assert((queue.view()[1] == AudioEvent{AudioCue::SpecialLaunch, AudioAction::StopAndRewind}));
    assert((queue.view()[2] == AudioEvent{AudioCue::DroneApproachLoop, AudioAction::SetVolume, 60}));
    assert((queue.view()[3] == AudioEvent{AudioCue::AirLoop, AudioAction::SetFrequency, 11025}));

    for (std::size_t i = queue.size; i < game_session_audio_event_capacity; ++i) {
        assert(queue.push({AudioCue::ShieldPulse, AudioAction::Play}));
    }
    assert(!queue.push({AudioCue::ShieldPulse, AudioAction::Play}));
    assert(queue.overflowed);

    return 0;
}
