#include <drone/audio/presentation_audio.hpp>
#include <drone/gameplay/game_state.hpp>

#include <cassert>

using namespace drone::audio;
using drone::gameplay::GameState;

namespace {

void assert_event(const AudioEventQueue& queue, const std::size_t index,
                  const AudioCue cue, const AudioAction action,
                  const std::int32_t value = 0) {
    assert(index < queue.size);
    assert((queue.view()[index] == AudioEvent{cue, action, value}));
}

} // namespace

int main() {
    static_assert(original_main_menu_lowbees_start_volume == 0);
    static_assert(original_main_menu_lowbees_volume_cap == 80);
    static_assert(original_air_loop_menu_frequency_hz == 11025);
    static_assert(original_completion_credits_fade_start_volume == 100);
    static_assert(original_completion_credits_fade_end_volume == 0);

    const auto& lowbees = audio_cue_definition(AudioCue::MainMenuLowBees);
    assert(lowbees.original_asset == "lowbees.wav");
    assert(lowbees.voice_policy == AudioVoicePolicy::SingleBuffer);
    assert(lowbees.original_volume_0_to_100 == 0);
    assert(lowbees.directsound_play_flags == directsound_play_looping_flag);

    // Bootstrap/restart-armed menu entry: SetVolume(0) precedes Play(loop).
    MainMenuAudioRuntimeState menu{};
    auto events = begin_original_main_menu_audio(menu);
    assert(menu.lowbees_owned);
    assert(!menu.lowbees_restart_armed);
    assert(menu.lowbees_volume_0_to_100 == 0);
    assert(events.size == 2);
    assert_event(events, 0, AudioCue::MainMenuLowBees, AudioAction::SetVolume, 0);
    assert_event(events, 1, AudioCue::MainMenuLowBees, AudioAction::Play);

    // Re-entering while the ownership byte remains consumed must not reload or
    // restart the ambience.
    assert(begin_original_main_menu_audio(menu).size == 0);

    // Exact +1/menu-iteration fade, capped at 80.
    events = tick_original_main_menu_audio(menu);
    assert(menu.lowbees_volume_0_to_100 == 1);
    assert(events.size == 1);
    assert_event(events, 0, AudioCue::MainMenuLowBees, AudioAction::SetVolume, 1);
    menu.lowbees_volume_0_to_100 = 79;
    events = tick_original_main_menu_audio(menu);
    assert(menu.lowbees_volume_0_to_100 == 80);
    assert_event(events, 0, AudioCue::MainMenuLowBees, AudioAction::SetVolume, 80);
    assert(tick_original_main_menu_audio(menu).size == 0);

    // Instructions/high scores preserve the owned menu ambience.
    assert(leave_original_main_menu_audio(
               menu, static_cast<std::int32_t>(GameState::Instructions)).size == 0);
    assert(menu.lowbees_owned);
    assert(leave_original_main_menu_audio(
               menu, static_cast<std::int32_t>(GameState::HighScores)).size == 0);
    assert(menu.lowbees_owned);

    // Ordering information is one of the exact cleanup states. Ownership ends
    // and the restart byte is re-armed.
    events = leave_original_main_menu_audio(
        menu, static_cast<std::int32_t>(GameState::OrderingInformation));
    assert(events.size == 1);
    assert_event(events, 0, AudioCue::MainMenuLowBees, AudioAction::StopAndRewind);
    assert(!menu.lowbees_owned);
    assert(menu.lowbees_restart_armed);

    // The other established raw cleanup values use the same lifecycle branch.
    for (const auto raw : {0, 2, 13, -1}) {
        (void)begin_original_main_menu_audio(menu);
        assert(menu.lowbees_owned);
        events = leave_original_main_menu_audio(menu, raw);
        assert(events.size == 1);
        assert_event(events, 0, AudioCue::MainMenuLowBees, AudioAction::StopAndRewind);
        assert(menu.lowbees_restart_armed);
    }

    // Main-menu tail: raw 0 performs no air restart. Every non-zero value uses
    // Play -> SetVolume(0) -> SetFrequency(11025), in exactly that order.
    OriginalAudioRuntimeState audio{};
    audio.air_loop_volume_0_to_100 = 37;
    assert(original_main_menu_air_restart(audio, 0).size == 0);
    assert(audio.air_loop_volume_0_to_100 == 37);

    events = original_main_menu_air_restart(audio, 2);
    assert(events.size == 3);
    assert(audio.air_loop_volume_0_to_100 == 0);
    assert_event(events, 0, AudioCue::AirLoop, AudioAction::Play);
    assert_event(events, 1, AudioCue::AirLoop, AudioAction::SetVolume, 0);
    assert_event(events, 2, AudioCue::AirLoop, AudioAction::SetFrequency, 11025);

    // Pause/quit/nine-lives overlay audio shares one exact fade-to-stop path.
    audio.air_loop_volume_0_to_100 = 2;
    events = tick_original_gameplay_overlay_audio(audio);
    assert(audio.air_loop_volume_0_to_100 == 1);
    assert_event(events, 0, AudioCue::AirLoop, AudioAction::SetVolume, 1);
    events = tick_original_gameplay_overlay_audio(audio);
    assert(audio.air_loop_volume_0_to_100 == 0);
    assert_event(events, 0, AudioCue::AirLoop, AudioAction::SetVolume, 0);
    events = tick_original_gameplay_overlay_audio(audio);
    assert(audio.air_loop_volume_0_to_100 == 0);
    assert_event(events, 0, AudioCue::AirLoop, AudioAction::StopAndRewind);

    // Resume to active gameplay restarts air and restores the canonical 50.
    events = resume_original_gameplay_overlay_audio(audio);
    assert(events.size == 2);
    assert(audio.air_loop_volume_0_to_100 == 50);
    assert_event(events, 0, AudioCue::AirLoop, AudioAction::Play);
    assert_event(events, 1, AudioCue::AirLoop, AudioAction::SetVolume, 50);

    // Credits keep their local scalar at 100 throughout the visual scroll.
    // Fade execution starts only after the visual terminal condition and then
    // decrements before each SetVolume call: exactly 99..0 across 100 host
    // presentation iterations. The existing post-game modal completion boundary
    // owns the following StopAndRewind.
    CompletionCreditsAudioRuntimeState credits{};
    auto credits_step = tick_original_completion_credits_fade(credits);
    assert(credits_step.audio_events.size == 0);
    assert(!credits_step.fade_completed);
    assert(credits.volume_0_to_100 == 100);

    begin_original_completion_credits_fade(credits);
    assert(credits.fade_active);
    assert(!credits.fade_complete);
    assert(credits.volume_0_to_100 == 100);

    for (std::int32_t expected = 99; expected >= 0; --expected) {
        credits_step = tick_original_completion_credits_fade(credits);
        assert(credits_step.audio_events.size == 1);
        assert_event(credits_step.audio_events, 0, AudioCue::CompletionCredits,
                     AudioAction::SetVolume, expected);
        assert(credits.volume_0_to_100 == expected);
        assert(credits_step.fade_completed == (expected == 0));
    }
    assert(!credits.fade_active);
    assert(credits.fade_complete);
    credits_step = tick_original_completion_credits_fade(credits);
    assert(credits_step.audio_events.size == 0);
    assert(!credits_step.fade_completed);

    return 0;
}
