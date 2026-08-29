#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace drone::audio {

// Win32 0x00420020 operates on exactly twenty reusable DirectSound buffers.
inline constexpr std::size_t original_sfx_voice_pool_capacity = 20;
inline constexpr std::uint32_t directsound_status_playing = 1;
inline constexpr std::uint32_t directsound_play_looping_flag = 1;

enum class OriginalLoopPlaybackOwner : std::uint8_t {
    BomberBoss,
    CompletionCredits,
    GeminiBoss,
    RegisteredBossSlot2Unresolved,
    State2AirStart,
    State2DroneLoop,
    SpideyBoss,
    LidTopBoss,
    MainMenuLowBees,
    MainMenuAirRestart,
    OrderingInformation,
    PostEncounterThunderStart,
    PostEncounterAirRestart,
};

enum class OriginalLoopFlagProof : std::uint8_t {
    LiteralOne,
    RegisterProvenOne,
};

struct OriginalLoopCallSite {
    std::uint32_t call_site_va = 0;
    // Address of the global integer that stores the DirectSound slot index.
    // Zero denotes a stack-local slot owned by a synchronous modal routine.
    std::uint32_t slot_storage_va = 0;
    std::string_view original_asset{}; // empty when the canonical binary never loads the slot
    OriginalLoopPlaybackOwner owner = OriginalLoopPlaybackOwner::BomberBoss;
    OriginalLoopFlagProof flag_proof = OriginalLoopFlagProof::LiteralOne;
};

// Canonical Win32 shareware Play(..., flags=1) sites. Eight callers push a
// literal 1; five more propagate a register that is proven to equal 1 on the
// path reaching the call. The unresolved registered boss slot is retained as
// evidence rather than assigned an invented asset.
[[nodiscard]] std::span<const OriginalLoopCallSite>
original_directsound_loop_call_sites() noexcept;

// The original pool helper restarts the first voice whose raw GetStatus value
// is not exactly DSBSTATUS_PLAYING (1). If every entry is exactly 1, voice 0 is
// stolen/restarted. No round-robin cursor is maintained.
[[nodiscard]] std::size_t select_original_sfx_voice(
    const std::array<std::uint32_t, original_sfx_voice_pool_capacity>& raw_status) noexcept;

// Win32 0x00406780 converts the game's 0..100 volume scale to DirectSound's
// hundredths-of-a-decibel attenuation with 30 * (volume - 100).
[[nodiscard]] constexpr std::int32_t original_directsound_attenuation(
    const std::int32_t volume_0_to_100) noexcept {
    return 30 * (volume_0_to_100 - 100);
}

inline constexpr std::uint32_t original_missile_frequency_hz = 22050;
inline constexpr std::uint32_t original_shield_frequency_hz = 11025;
inline constexpr std::uint32_t original_bigexp3_frequency_hz = 15000;

// air.wav is loaded at game volume 50. The active state-2 start path also
// writes scalar 50, while post-encounter/menu restart paths deliberately
// restart it at zero and let state 2 rebuild the ambience one unit at a time.
inline constexpr std::int32_t original_air_loop_loaded_volume = 50;
inline constexpr std::int32_t original_air_loop_volume_cap = 50;
inline constexpr std::int32_t original_air_loop_restart_volume = 0;
inline constexpr std::int32_t original_air_loop_fade_boundary = 60;
inline constexpr std::uint32_t original_air_loop_menu_frequency_hz = 11025;

// lowbees.wav is the independently owned main-menu ambience. run_main_menu
// explicitly starts it at zero and raises the local volume by one per menu
// loop until the original cap of 80.
inline constexpr std::int32_t original_main_menu_lowbees_start_volume = 0;
inline constexpr std::int32_t original_main_menu_lowbees_volume_cap = 80;

// run_completion_credits starts credits.wav without an explicit SetVolume.
// Its separate local fade scalar is initialized to 100 and, only after the
// visual scroll has completed, is decremented before each SetVolume call.
// Therefore the first explicit volume write is 99 and the last is 0.
inline constexpr std::int32_t original_completion_credits_fade_start_volume = 100;
inline constexpr std::int32_t original_completion_credits_fade_end_volume = 0;

// drone.wav is loaded at volume 90. The live approach path starts it looping
// at Y=-117, immediately writes volume 0, ramps toward 80 on eligible phase-2
// updates, switches to 60 when Probe decoding enters phase 2, and restores 80
// if that phase is interrupted by an enemy bomb.
inline constexpr std::int32_t original_drone_loop_loaded_volume = 90;
inline constexpr std::int32_t original_drone_loop_start_volume = 0;
inline constexpr std::int32_t original_drone_loop_approach_volume_cap = 80;
inline constexpr std::int32_t original_drone_loop_phase2_decode_volume = 60;
inline constexpr std::int32_t original_drone_loop_interrupted_decode_volume = 80;

} // namespace drone::audio
