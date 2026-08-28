#pragma once

#include <drone/gameplay/boss_encounter.hpp>
#include <drone/gameplay/difficulty.hpp>
#include <drone/gameplay/drone_weapon_interaction.hpp>
#include <drone/gameplay/drone_objective.hpp>
#include <drone/gameplay/enemy_bomb.hpp>
#include <drone/gameplay/game_state.hpp>
#include <drone/gameplay/input.hpp>
#include <drone/gameplay/mission_outcome.hpp>
#include <drone/gameplay/mission_progression.hpp>
#include <drone/gameplay/original_random.hpp>
#include <drone/gameplay/player.hpp>
#include <drone/gameplay/player_lifecycle.hpp>
#include <drone/gameplay/rapid_missile.hpp>
#include <drone/gameplay/scoring.hpp>
#include <drone/gameplay/shield.hpp>
#include <drone/gameplay/special_weapon.hpp>
#include <drone/gameplay/trajectory_encounter.hpp>
#include <drone/gameplay/world_scroll.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace drone::gameplay {

// Campaign state survives the original encounter-only reset path. Keeping it
// separate from encounter state prevents the clean engine from recreating the
// original executable's monolithic global-memory ABI.
struct GameCampaignState {
    PlayerLifecycleState player_lifecycle{};
    ScoreState score{};
    MissionOutcomeState mission{};

    // These are session-level eligibility facts recovered from the post-game
    // path. Phase 4 owns them here even though their producers are integrated
    // in later milestones.
    bool high_score_disqualified = false;
    bool mothership_destroyed = false;
    std::int32_t alien_ships_hit = 0;
    std::int32_t alien_ships_total = 0;
};


struct GameRuntimeOptions {
    DifficultyLevel difficulty = DifficultyLevel::Beginner;
    bool demo_playback_mode = false;
};

// State rebuilt for every encounter by the recovered reset architecture.
struct GameEncounterState {
    PlayerMotionState player{};
    PlayerShieldState shield{};
    RapidMissilePool rapid_missiles{};
    SpecialWeaponState special_weapon{};
    EnemyBombPool enemy_bombs{};
    EnemyBombSpawnGate enemy_bomb_spawn_gate{};
    TrajectoryEncounterState trajectories{};
    BossEncounterState boss{};
    DroneObjectiveState drone{};

    std::int32_t gameplay_substep_phase = 0;
    std::int32_t world_scroll_row = canonical_world_scroll_initial_row;
    std::int32_t drone_settlement_tick = canonical_drone_settlement_tick_cap;
    std::uint64_t gameplay_updates = 0;
};

struct GameSession {
    GameSession();

    GameState state = GameState::ActiveGameplay;
    GameRuntimeOptions runtime{};
    GameCampaignState campaign{};
    GameEncounterState encounter{};

    // The original CRT RNG is process-global and is seeded once at startup, so
    // neither encounter-only nor full-campaign gameplay resets reseed it.
    OriginalRandomState original_random{};

    // Clean diagnostic counter for the whole run. This is intentionally not an
    // assertion about an original scalar/global.
    std::uint64_t total_gameplay_updates = 0;
};

struct SpecialTargetGeometry {
    std::int32_t x = 0;
    std::int16_t width = 0;
};

// Encounter-owned producer facts not yet reconstructed inside GameSession.
// Drone weapon collisions and Probe decode/disarm are now owned; hostile-target
// selection for Stingers remains external until enemy selection is integrated.
struct GameSessionTargetContext {
    std::optional<SpecialTargetGeometry> stinger_target{};

    // Bomb redirection has an independently recovered external gate in the
    // original. Keep that gate explicit instead of assigning it a false name.
    bool redirect_bombs_to_attached_probe = false;

    // Phase-4 encounter integration. Asset/path samples remain immutable input;
    // mutable 17-group lifecycle/actor state lives inside GameSession. A caller
    // may request one already-selected transient template activation at the
    // formation stage and provide collision hits detected by the exact sprite-
    // mask collision layer later in the same logical update.
    const TrajectoryPathCatalogView* trajectory_paths = nullptr;
    std::optional<std::uint8_t> trajectory_spawn_group{};
    std::int16_t trajectory_spawn_x_offset = 0;
    std::int16_t trajectory_spawn_y_offset = 0;
    std::span<const TrajectoryHitEvent> trajectory_hits{};

    // Exact boss-local collision/damage remains outside this owner. These are
    // already-validated destruction transitions, not broad-phase/raw hits.
    std::span<const SharewareBossDestructionTrigger> boss_destruction_triggers{};
};

struct GameSessionTickResult {
    bool advanced = false;
    std::uint64_t encounter_update = 0;
    std::uint64_t total_update = 0;
    std::int32_t gameplay_substep_phase = 0;
    bool animation_tick = false;

    bool rapid_missile_fired = false;
    std::size_t rapid_missiles_retired = 0;
    std::size_t enemy_bombs_retired = 0;
    bool enemy_bomb_hit_special_weapon = false;
    std::optional<std::size_t> enemy_bomb_special_hit_index{};
    bool enemy_bomb_probe_decode_reset = false;
    bool enemy_bomb_probe_phase2_interrupt_signal_requested = false;
    bool enemy_bomb_special_launch_sound_stop_requested = false;
    bool enemy_bomb_probe_impact_effect_requested = false;
    bool enemy_bomb_probe_impact_sound_requested = false;
    bool enemy_bomb_stinger_impact_effect_requested = false;
    bool enemy_bomb_stinger_impact_sound_requested = false;

    bool trajectory_group_spawned = false;
    std::size_t trajectory_actors_activated = 0;
    std::size_t trajectory_actors_escaped = 0;
    std::size_t trajectory_actors_destroyed = 0;
    std::size_t trajectory_groups_retired = 0;
    std::uint32_t trajectory_destruction_bursts = 0;
    std::int32_t trajectory_score_delta = 0;

    bool boss_activated = false;
    std::optional<BossFamily> boss_activated_family{};
    std::size_t boss_destruction_transitions = 0;
    std::size_t boss_components_retired = 0;
    std::int32_t boss_score_delta = 0;
    bool lid_top_motion_stop_requested = false;

    bool probe_decode_phase1_completed = false;
    bool probe_decode_completed = false;
    bool probe_decode_cleared = false;
    std::int32_t probe_decode_score_delta = 0;
    std::uint16_t probe_decode_completion_effect_random = 0;
    bool rapid_missile_hit_drone = false;
    std::optional<std::size_t> rapid_missile_drone_hit_index{};
    bool special_weapon_hit_drone = false;
    bool probe_attached_to_drone = false;
    bool stinger_hit_drone = false;
    std::int32_t probe_attachment_score_delta = 0;
    std::uint8_t drone_weapon_hit_explosion_spawns_requested = 0;

    bool drone_moved = false;
    bool drone_disarm_committed = false;
    bool drone_settlement_tick_reset = false;
    bool drone_hover_timeout_reached = false;
    bool drone_destruction_countdown_started = false;
    bool drone_destruction_countdown_advanced = false;
    bool drone_detonation_started = false;
    bool drone_detonation_outcome_committed = false;
    std::int32_t drone_detonation_score_delta = 0;
    bool drone_detonation_effect_tick = false;
    std::uint8_t drone_detonation_explosion_spawns_requested = 0;
    bool drone_detonation_settlement_reset = false;
    bool drone_detonation_settlement_advanced = false;
    bool drone_destruction_settled = false;
    bool drone_life_lost = false;
    bool drone_game_over_pending = false;
    bool drone_destruction_transition_started = false;
    bool drone_resolution_transition_started = false;
    std::optional<MissionInterstitialPlan> mission_interstitial{};
    std::optional<EncounterTransitionPlan> encounter_transition{};

    bool special_loaded = false;
    bool special_cycled = false;
    bool special_launched = false;

    bool shield_active = false;
    bool shield_sound_requested = false;
    bool extra_life_awarded = false;
};

// FullCampaign reconstructs a new run; EncounterOnly preserves campaign score,
// lives, mission outcomes/statistics and high-score eligibility while rebuilding
// per-encounter gameplay state. This is the clean ownership counterpart of the
// recovered 0x00417F50 full/encounter reset distinction.
void reset_game_session(GameSession& session, GameplaySessionResetScope scope);

// Execute one continuous active-gameplay update using only behavior that has
// already been recovered and independently tested. Trajectory groups, Probe
// attachment/decode/disarm timing, rapid-missile/Stinger Drone-hit producers,
// normal Drone objective travel/settlement, the timeout/countdown/detonation/
// life-loss path, and the shareware Lid/Top/Gemini lifecycle tails are now
// continuously owned. Transient formation selection, hostile Stinger target
// selection, direct detonation visuals, boss movement/attacks and remaining
// enemy collision producers remain explicit boundaries for later Phase 4.
[[nodiscard]] GameSessionTickResult step_game_session(
    GameSession& session,
    const GameplayInputFrame& input,
    const GameSessionTargetContext& targets = {});

} // namespace drone::gameplay
