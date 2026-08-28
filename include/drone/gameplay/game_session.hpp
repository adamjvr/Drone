#pragma once

#include <drone/gameplay/enemy_bomb.hpp>
#include <drone/gameplay/game_state.hpp>
#include <drone/gameplay/input.hpp>
#include <drone/gameplay/mission_outcome.hpp>
#include <drone/gameplay/mission_progression.hpp>
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

// State rebuilt for every encounter by the recovered reset architecture.
struct GameEncounterState {
    PlayerMotionState player{};
    PlayerShieldState shield{};
    RapidMissilePool rapid_missiles{};
    SpecialWeaponState special_weapon{};
    EnemyBombPool enemy_bombs{};
    EnemyBombSpawnGate enemy_bomb_spawn_gate{};
    TrajectoryEncounterState trajectories{};

    std::int32_t gameplay_substep_phase = 0;
    std::int32_t world_scroll_row = canonical_world_scroll_initial_row;
    std::int32_t drone_settlement_tick = 0;
    std::uint64_t gameplay_updates = 0;
};

struct GameSession {
    GameSession();

    GameState state = GameState::ActiveGameplay;
    GameCampaignState campaign{};
    GameEncounterState encounter{};

    // Clean diagnostic counter for the whole run. This is intentionally not an
    // assertion about an original scalar/global.
    std::uint64_t total_gameplay_updates = 0;
};

struct SpecialTargetGeometry {
    std::int32_t x = 0;
    std::int16_t width = 0;
};

// Encounter-owned target facts needed by already-recovered common projectile
// updates. Later Phase-4 encounter integration will produce this context from
// the session's reconstructed actor/object collections.
struct GameSessionTargetContext {
    std::int32_t drone_x = canonical_drone_session_initial_x;
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

    bool trajectory_group_spawned = false;
    std::size_t trajectory_actors_activated = 0;
    std::size_t trajectory_actors_escaped = 0;
    std::size_t trajectory_actors_destroyed = 0;
    std::size_t trajectory_groups_retired = 0;
    std::uint32_t trajectory_destruction_bursts = 0;
    std::int32_t trajectory_score_delta = 0;

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
// already been recovered and independently tested. Trajectory groups are now
// continuously owned and advanced when immutable path samples are supplied;
// transient formation selection and opaque-pixel hit detection remain explicit
// producers. Drone resolution and boss transitions are integrated by following
// Phase-4 milestones.
[[nodiscard]] GameSessionTickResult step_game_session(
    GameSession& session,
    const GameplayInputFrame& input,
    const GameSessionTargetContext& targets = {});

} // namespace drone::gameplay
