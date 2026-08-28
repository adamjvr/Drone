#pragma once

#include <drone/gameplay/difficulty.hpp>
#include <drone/gameplay/original_random.hpp>
#include <drone/gameplay/player.hpp>
#include <drone/gameplay/scoring.hpp>

#include <cstdint>

namespace drone::gameplay {

// The Win32 build stores the selected special weapon directly in the common
// entity's current-frame byte (+0x140): frame 0 is Probe.jba and frame 1 is
// Redprobe.jba. Keep the numeric values stable because they are original data.
enum class SpecialWeaponKind : std::uint8_t {
    Probe = 0,
    Stinger = 1,
};

// Established values of the special entity's common activity/state byte
// (+0x142). Values 4 and 10 were promoted during Phase 2 after their
// producer/consumer paths were recovered: 4 is the visible hole-interaction
// state; 10 is a one-update consumed/impact terminal state.
enum class SpecialWeaponActivity : std::uint8_t {
    Inactive = 0,
    LoadedTracking = 1,
    ProbeAttachedDecoding = 2,
    LaunchedHoming = 3,
    HoleInteraction = 4,
    ImpactConsumed = 10,
};

// Separate global decode status byte 0x0045BEEA. The values are intentionally
// non-sequential because they are original protocol values consumed directly by
// the state-2 decoder and HUD paths.
enum class ProbeDecodeStatus : std::uint8_t {
    Phase1Decoding = 0,
    Complete = 1,
    Phase2Disarming = 3,
};

struct ProbeDecodeState {
    ProbeDecodeStatus status = ProbeDecodeStatus::Phase1Decoding;
    std::uint16_t phase1_elapsed = 0;
    std::uint16_t phase1_threshold = 0;
    std::uint16_t phase2_elapsed = 0;
    std::uint16_t phase2_threshold = 0;
};

struct ProbeDecodeTickResult {
    bool phase1_completed = false;
    bool disarm_completed = false;
    std::int32_t score_delta = 0;
    // The original consumes rand()%60+40 on completion for an effect/sound
    // parameter. Keeping the draw visible preserves shared RNG sequencing even
    // though presentation owns the downstream effect.
    std::uint16_t completion_effect_random = 0;
};

// Narrow semantic reconstruction of the special projectile entity rooted at
// Win32 0x0045A148. The original is a full 0x154-byte common entity; only
// established fields needed by the reconstructed input/homing lifecycle are
// represented here.
struct SpecialWeaponState {
    std::int32_t x = 146; // original initializer X
    std::int32_t y = 182; // original initializer Y
    std::int32_t motion_x = 0;
    std::int32_t motion_y = 0;
    SpecialWeaponKind kind = SpecialWeaponKind::Probe; // +0x140
    SpecialWeaponActivity activity = SpecialWeaponActivity::Inactive; // +0x142
    bool out_of_bounds = true; // semantic view of +0x143

    // Original special-entity scratch fields +0x36 and +0x3C are used as a
    // switch debounce/progress counter and threshold while state == 1.
    std::int16_t switch_progress = 0;
    std::int16_t switch_threshold = 12;

    // These counters/status bytes are separate globals in the original, but
    // semantically belong to the one active Probe decoder and therefore live
    // beside the clean special-weapon state rather than in a raw global table.
    ProbeDecodeState probe_decode{};
};

// While loaded/tracking, the original increments +0x36 until it reaches the
// +0x3C threshold (canonical initialization: 12). This gates Down-key weapon
// cycling, not launching.
void advance_special_weapon_switch_progress(SpecialWeaponState& state);

// Down Arrow when the special entity is inactive enters state 1, places its Y
// origin at player.y + 7, clears +0x143, and resets switch progress. X is
// refreshed from the player by the subsequent tracking update.
bool load_special_weapon(
    SpecialWeaponState& state,
    const PlayerMotionState& player,
    bool load_requested,
    bool player_active);

// Down Arrow while state 1 can cycle frame 0 <-> 1 only after the recovered
// switch-progress threshold has been reached. Successful cycling resets the
// progress counter to zero.
bool toggle_loaded_special_weapon(
    SpecialWeaponState& state,
    bool toggle_requested,
    bool player_active);

// Up Arrow while state 1 advances the common activity byte to state 3. The
// original caller also starts probe3.wav; audio remains outside this pure
// gameplay state helper.
bool launch_special_weapon(
    SpecialWeaponState& state,
    bool launch_requested,
    bool player_active);

// State 1 is re-anchored to player+(14,7) each update before the common special
// movement block runs. State 1 and state 3 then move Y upward by two and steer
// X by exactly one pixel toward an already-selected target X coordinate.
// Returns false if the state is not one of those two established movable states.
bool step_special_weapon_homing(
    SpecialWeaponState& state,
    const PlayerMotionState& player,
    std::int32_t target_x);

// Exact target-X calculations recovered from the Win32 special update path.
// Stingers use target.x + target.width/2. The normal blue-probe path aims at
// Drone entity X + 4 (not its geometric center).
std::int32_t stinger_target_x(std::int32_t target_x, std::int16_t target_width);
std::int32_t probe_drone_target_x(std::int32_t drone_x);

// A launched blue probe that collides with the Drone changes the special
// entity activity byte from 3 to 2 and begins the separate decode/disarm timer
// machinery. This helper models only the established entity transition.
bool attach_probe_to_drone(SpecialWeaponState& state);

// State 2 is pinned horizontally to the Drone during the common update. The
// original uses Drone.x + 5 here (distinct from the launched Probe target X of
// Drone.x + 4). Returns false unless state is ProbeAttachedDecoding.
bool pin_attached_probe_to_drone(SpecialWeaponState& state, std::int32_t drone_x);

// A launched projectile that collides with the active hole.jba entity under
// the recovered gating conditions enters state 4. Audio/rendering side effects
// are outside this pure state helper.
bool enter_special_weapon_hole_interaction(SpecialWeaponState& state);

// Numerous collision paths place a consumed projectile in state 10. The common
// next-update dispatcher stops the launch sound and immediately writes state 0.
// This helper models only that proven state transition.
bool settle_special_weapon_terminal_state(SpecialWeaponState& state);

// Initialize the exact two-stage Probe timing after a frame-0 Drone collision.
// Demo playback uses fixed 210/150 thresholds. Live play consumes two values
// from the recovered MSVC CRT RNG and applies (rand()%70+450/300)*difficulty.
void initialize_probe_decode_timing(
    ProbeDecodeState& decode,
    DifficultyLevel difficulty,
    bool demo_playback_mode,
    OriginalRandomState& random) noexcept;

// Advance the attached Probe decoder once per active gameplay update. Phase 1
// completion writes status 3, resets phase-2 elapsed, and then phase 2 advances
// to tick 1 in the same update. Final completion awards +500 to both score
// accumulators, writes status 1, and consumes the original completion RNG draw.
[[nodiscard]] ProbeDecodeTickResult step_probe_decode(
    SpecialWeaponState& state,
    OriginalRandomState& random,
    ScoreState& score) noexcept;

// The normal Drone settlement path clears status 1 and both elapsed counters
// once the objective has moved beyond Y=230. Thresholds are left intact, like
// the original globals, until the next attachment overwrites them.
[[nodiscard]] bool clear_completed_probe_decode(SpecialWeaponState& state) noexcept;

} // namespace drone::gameplay
