#pragma once

#include <drone/gameplay/alien_accounting.hpp>
#include <drone/gameplay/boss_encounter.hpp>
#include <drone/gameplay/difficulty.hpp>
#include <drone/gameplay/drone_weapon_interaction.hpp>
#include <drone/gameplay/drone_objective.hpp>
#include <drone/gameplay/enemy_bomb.hpp>
#include <drone/gameplay/game_state.hpp>
#include <drone/gameplay/gemini_boss.hpp>
#include <drone/gameplay/input.hpp>
#include <drone/gameplay/lid_top_boss.hpp>
#include <drone/gameplay/mission_outcome.hpp>
#include <drone/gameplay/mission_progression.hpp>
#include <drone/gameplay/original_random.hpp>
#include <drone/gameplay/player.hpp>
#include <drone/gameplay/player_lifecycle.hpp>
#include <drone/gameplay/rapid_missile.hpp>
#include <drone/gameplay/scoring.hpp>
#include <drone/gameplay/shield.hpp>
#include <drone/gameplay/special_weapon.hpp>
#include <drone/gameplay/stinger_targeting.hpp>
#include <drone/gameplay/trajectory_encounter.hpp>
#include <drone/gameplay/trajectory_collision.hpp>
#include <drone/gameplay/trajectory_spawn.hpp>
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
    // Full-session initialization seeds the mission-wide counter to the same
    // seven primary actors as Win32 0x0041801E. Encounter folds accumulate on
    // top of this value and preserve the original mixed live-count quirks.
    std::int32_t alien_ships_total = canonical_initial_encounter_alien_ships_total;
};


struct GameRuntimeOptions {
    DifficultyLevel difficulty = DifficultyLevel::Beginner;
    bool demo_playback_mode = false;
    bool demo_recording_mode = false;
};

// State rebuilt for every encounter by the recovered reset architecture.
struct GameEncounterState {
    PlayerMotionState player{};
    PlayerShieldState shield{};
    RapidMissilePool rapid_missiles{};
    SpecialWeaponState special_weapon{};
    StingerTargetState stinger_target{};
    EnemyBombPool enemy_bombs{};
    EnemyBombSpawnGate enemy_bomb_spawn_gate{};
    TrajectoryEncounterState trajectories{};
    TrajectorySpawnSchedulerState trajectory_spawn{};
    StingerDisplayState stinger_display{};
    // Original 0x00466B04: encounter-local alien total used by the interstitial
    // hit/miss summary. It starts at the seven primary Loop actors and grows as
    // actors are inserted; recovered interstitials fold it into mission totals.
    std::int32_t encounter_alien_ships_total = canonical_initial_encounter_alien_ships_total;
    std::int32_t encounter_alien_ships_hit = 0;
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

// Encounter-owned producer facts not yet reconstructed inside GameSession.
// Stinger target *selection* is now owned; individual boss/hostile geometry and
// activity facts remain inputs until their movement/state machines are native.
struct GameSessionTargetContext {
    StingerTargetSelectionContext stinger_targets{};

    // Bomb redirection has an independently recovered external gate in the
    // original. Keep that gate explicit instead of assigning it a false name.
    bool redirect_bombs_to_attached_probe = false;

    // The player-death presentation/effect pool is not yet owned by the clean
    // simulation. The original respawn settlement tests its activity byte == 0.
    // A host/fidelity owner may therefore expose only that exact semantic fact
    // without forcing the gameplay core to invent the effect's pixel behavior.
    bool player_death_effect_inactive = false;

    // Immutable trajectory samples remain external asset data, but live
    // transient formation selection/timing/RNG are now session-owned. The
    // registered-only Mothership destruction gate remains an explicit fact
    // until that encounter is reconstructed. Rapid-missile collision needs only
    // immutable extracted trajectory-frame masks; hit production itself is now
    // owned by GameSession.
    const TrajectoryPathCatalogView* trajectory_paths = nullptr;
    const TrajectorySpriteMaskCatalogView* trajectory_sprite_masks = nullptr;
    bool mothership_destruction_active = false;

    // Both shareware bosses now own movement/combat natively. Callers provide
    // only immutable extracted sprite pixels required by the original opaque-
    // pixel collision primitives.
    const LidTopBossSpriteMaskView* lid_top_sprite_mask = nullptr;
    const GeminiBossSpriteMaskView* gemini_sprite_masks = nullptr;
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
    std::size_t enemy_bomb_player_hits = 0;
    std::size_t enemy_bomb_shield_absorptions = 0;
    std::optional<std::size_t> enemy_bomb_first_player_hit_index{};
    bool enemy_bomb_auto_launched_special = false;
    bool enemy_bomb_auto_launch_sound_requested = false;
    bool enemy_bomb_player_hit_sfx_requested = false;
    bool player_destruction_started = false;
    bool player_death_effect_requested = false;
    bool player_bomb_spawn_suppression_started = false;
    bool player_life_consumed = false;
    bool player_respawned = false;
    bool player_respawn_shield_reset = false;
    bool player_game_over_banner_requested = false;

    bool primary_trajectory_replenishment_checked = false;
    bool primary_trajectory_roll_forced_to_one = false;
    bool primary_trajectory_spawn_roll_passed = false;
    bool primary_trajectory_actor_replenished = false;
    bool primary_trajectory_group_reactivated = false;
    std::optional<std::uint8_t> primary_trajectory_actor_index{};
    std::int32_t primary_trajectory_entry_x = 0;
    std::int32_t primary_trajectory_entry_y = 0;
    std::int32_t encounter_alien_ships_total = 0;
    std::int32_t encounter_alien_ships_hit = 0;
    std::int32_t mission_alien_ships_total = 0;
    std::int32_t mission_alien_ships_hit = 0;
    bool encounter_alien_statistics_folded = false;
    std::optional<EncounterAlienStatistics> encounter_alien_statistics{};

    bool trajectory_group_spawned = false;
    bool trajectory_spawn_forced = false;
    bool trajectory_spawn_roll_passed = false;
    std::optional<std::uint8_t> trajectory_spawned_group{};
    std::optional<std::uint8_t> trajectory_spawn_sound_index{};
    TrajectoryPathFamily trajectory_spawn_runtime_family = TrajectoryPathFamily::Loop;
    std::int16_t trajectory_spawn_x_offset = 0;
    std::int16_t trajectory_spawn_y_offset = 0;
    bool trajectory_spawn_actor_offsets_randomized = false;
    std::size_t trajectory_actors_activated = 0;
    std::size_t trajectory_actors_escaped = 0;
    std::size_t trajectory_actors_destroyed = 0;
    std::size_t trajectory_groups_retired = 0;
    std::uint32_t trajectory_destruction_bursts = 0;
    std::int32_t trajectory_score_delta = 0;
    std::size_t trajectory_rapid_collisions = 0;
    std::size_t trajectory_rapid_missiles_consumed = 0;
    std::size_t trajectory_stinger_display_collisions = 0;
    std::size_t trajectory_direct_special_collisions = 0;
    bool trajectory_stinger_display_activated = false;

    bool boss_activated = false;
    std::optional<BossFamily> boss_activated_family{};
    std::size_t boss_destruction_transitions = 0;
    std::size_t boss_components_retired = 0;
    std::int32_t boss_score_delta = 0;
    bool lid_top_motion_stop_requested = false;
    bool lid_top_root_moved = false;
    bool lid_top_vertical_retreat_started = false;
    bool lid_top_enemy_bomb_spawned = false;
    std::optional<std::size_t> lid_top_enemy_bomb_spawn_index{};
    std::size_t lid_top_rapid_missiles_consumed = 0;
    std::size_t lid_top_rapid_top_opaque_collisions = 0;
    std::size_t lid_top_rapid_open_collisions = 0;
    bool lid_top_lid_opened = false;
    bool lid_top_lid_close_started = false;
    bool lid_top_special_closed_top_impact = false;
    bool lid_top_stinger_core_hit = false;
    bool gemini_root_moved = false;
    bool gemini_vertical_retreat_started = false;
    bool gemini_enemy_bomb_spawned = false;
    std::optional<std::size_t> gemini_enemy_bomb_spawn_index{};
    bool gemini_special_hit_side_a = false;
    bool gemini_special_hit_side_b = false;
    bool gemini_special_hit_head = false;
    bool gemini_special_hit_body = false;
    std::uint8_t gemini_special_damage = 0;
    bool gemini_stinger_display_activated = false;

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
    StingerTargetIdentity stinger_target_identity = StingerTargetIdentity::DummyCenter;
    std::int32_t stinger_target_desired_x = canonical_stinger_dummy_target_x;
    bool stinger_target_changed = false;

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
// life-loss path, and both shareware boss combat state machines are now
// continuously owned. Stinger target priority/retention is also native; Gemini
// and Lid/Top target geometry comes from the pre-boss-update session snapshot.
// Primary group-0 replenishment and transient formation selection are native;
// direct detonation/death visuals, presentation-side Gemini RNG consumption and
// remaining non-trajectory enemy actor producers remain later Phase-4 edges.
[[nodiscard]] GameSessionTickResult step_game_session(
    GameSession& session,
    const GameplayInputFrame& input,
    const GameSessionTargetContext& targets = {});

} // namespace drone::gameplay
