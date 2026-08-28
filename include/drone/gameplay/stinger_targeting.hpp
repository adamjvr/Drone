#pragma once

#include <cstdint>

namespace drone::gameplay {

// Win32 initializes the shared special-target pointer to a small zero-filled
// dummy object whose X coordinate is explicitly written to 160. Its width is
// zero, so the homing target coordinate is exactly the screen center.
inline constexpr std::int32_t canonical_stinger_dummy_target_x = 160;

struct StingerTargetGeometry {
    std::int32_t x = 0;
    std::int16_t width = 0;
};

enum class StingerTargetIdentity : std::uint8_t {
    DummyCenter,
    MothershipHole,
    GeminiHeadA,
    GeminiHeadB,
    LidTopTop,
    Spidey,
    RegisteredSlot2,
    Bomber,
    UnknownDynamicHostile,
};

struct StingerTargetState {
    StingerTargetIdentity identity = StingerTargetIdentity::DummyCenter;
    StingerTargetGeometry geometry{canonical_stinger_dummy_target_x, 0};
};

// These are already-owned/current encounter facts used by the original target
// priority chain. Geometry can remain supplied by an actor/fidelity owner until
// the corresponding boss movement is reconstructed; selection itself is no
// longer a semantic host decision.
struct StingerTargetSelectionContext {
    // Mothership panel activity at 0x00446DD2 gates direct targeting of the
    // hole entity at 0x00433700 and outranks every boss family below.
    bool mothership_panel_active = false;
    StingerTargetGeometry mothership_hole{};

    bool gemini_body_a_active = false;
    bool gemini_body_b_active = false;
    StingerTargetGeometry gemini_head_a{};
    StingerTargetGeometry gemini_head_b{};

    bool lid_top_top_active = false;
    std::uint8_t lid_current_frame = 0;
    StingerTargetGeometry lid_top_top{};

    bool spidey_active = false;
    StingerTargetGeometry spidey{};

    bool registered_slot2_active = false;
    StingerTargetGeometry registered_slot2{};

    bool bomber_active = false;
    StingerTargetGeometry bomber{};

    // Win32 has one later active common-entity owner at 0x00459F90 whose
    // selected target geometry is reached indirectly through 0x00495CE8.
    // Its exact proper identity remains unresolved, so keep the clean name
    // conservative while preserving its priority slot exactly.
    bool unknown_dynamic_hostile_active = false;
    StingerTargetGeometry unknown_dynamic_hostile{};
};

struct StingerTargetSelectionResult {
    StingerTargetIdentity identity = StingerTargetIdentity::DummyCenter;
    StingerTargetGeometry geometry{};
    std::int32_t desired_x = canonical_stinger_dummy_target_x;
    bool target_changed = false;
};

void reset_stinger_target(StingerTargetState& state) noexcept;

// Reproduce Win32 0x0040DF47..0x0040E04E. If no candidate branch qualifies,
// the original leaves the shared target pointer untouched; this function does
// the same by retaining `state`.
[[nodiscard]] StingerTargetSelectionResult select_stinger_target(
    StingerTargetState& state,
    const StingerTargetSelectionContext& context,
    std::int32_t player_x) noexcept;

[[nodiscard]] std::int32_t stinger_target_desired_x(
    const StingerTargetState& state) noexcept;

} // namespace drone::gameplay
