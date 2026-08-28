#include <drone/gameplay/stinger_targeting.hpp>

#include <cassert>

using namespace drone::gameplay;

int main() {
    // Loading/reset uses the original dummy target at x=160 with zero width.
    {
        StingerTargetState state{
            .identity = StingerTargetIdentity::Bomber,
            .geometry = {25, 30},
        };
        reset_stinger_target(state);
        assert(state.identity == StingerTargetIdentity::DummyCenter);
        assert(state.geometry.x == 160 && state.geometry.width == 0);
        assert(stinger_target_desired_x(state) == 160);
    }

    // Mothership hole targeting outranks all other candidate families.
    {
        StingerTargetState state{};
        StingerTargetSelectionContext context{};
        context.mothership_panel_active = true;
        context.mothership_hole = {12, 14};
        context.gemini_body_a_active = true;
        context.gemini_head_a = {200, 40};
        context.lid_top_top_active = true;
        context.lid_current_frame = 8;
        context.lid_top_top = {100, 68};
        const auto result = select_stinger_target(state, context, 147);
        assert(result.identity == StingerTargetIdentity::MothershipHole);
        assert(result.desired_x == 19);
        assert(result.target_changed);
    }

    // Gemini: one active side selects its corresponding head.
    {
        StingerTargetState state{};
        StingerTargetSelectionContext context{};
        context.gemini_body_a_active = true;
        context.gemini_head_a = {80, 43};
        context.gemini_head_b = {220, 43};
        auto result = select_stinger_target(state, context, 147);
        assert(result.identity == StingerTargetIdentity::GeminiHeadA);
        assert(result.desired_x == 101);

        context.gemini_body_a_active = false;
        context.gemini_body_b_active = true;
        result = select_stinger_target(state, context, 147);
        assert(result.identity == StingerTargetIdentity::GeminiHeadB);
        assert(result.desired_x == 241);
    }

    // Gemini: both active selects the head whose X is nearest player X. Width
    // does not participate in the distance comparison. Exact ties select B.
    {
        StingerTargetState state{};
        StingerTargetSelectionContext context{};
        context.gemini_body_a_active = true;
        context.gemini_body_b_active = true;
        context.gemini_head_a = {100, 43};
        context.gemini_head_b = {260, 43};
        auto result = select_stinger_target(state, context, 140);
        assert(result.identity == StingerTargetIdentity::GeminiHeadA);

        context.gemini_head_a.x = 100;
        context.gemini_head_b.x = 180;
        result = select_stinger_target(state, context, 140);
        assert(result.identity == StingerTargetIdentity::GeminiHeadB);
    }

    // Lid/Top is eligible only while the top/root is exactly active and the
    // lid animation frame has progressed strictly above 3.
    {
        StingerTargetState state{};
        StingerTargetSelectionContext context{};
        context.lid_top_top_active = true;
        context.lid_top_top = {50, 68};
        context.lid_current_frame = 3;
        auto result = select_stinger_target(state, context, 100);
        assert(result.identity == StingerTargetIdentity::DummyCenter);
        context.lid_current_frame = 4;
        result = select_stinger_target(state, context, 100);
        assert(result.identity == StingerTargetIdentity::LidTopTop);
        assert(result.desired_x == 84);
    }

    // Remaining priority is exact: Spidey -> slot2 -> Bomber -> later unknown
    // dynamic hostile. Disabling each earlier candidate exposes the next one.
    {
        StingerTargetState state{};
        StingerTargetSelectionContext context{};
        context.spidey_active = true;
        context.spidey = {10, 20};
        context.registered_slot2_active = true;
        context.registered_slot2 = {30, 20};
        context.bomber_active = true;
        context.bomber = {50, 20};
        context.unknown_dynamic_hostile_active = true;
        context.unknown_dynamic_hostile = {70, 20};

        auto result = select_stinger_target(state, context, 0);
        assert(result.identity == StingerTargetIdentity::Spidey);
        context.spidey_active = false;
        result = select_stinger_target(state, context, 0);
        assert(result.identity == StingerTargetIdentity::RegisteredSlot2);
        context.registered_slot2_active = false;
        result = select_stinger_target(state, context, 0);
        assert(result.identity == StingerTargetIdentity::Bomber);
        context.bomber_active = false;
        result = select_stinger_target(state, context, 0);
        assert(result.identity == StingerTargetIdentity::UnknownDynamicHostile);
    }

    // No qualifying branch retains the previous shared target pointer exactly.
    {
        StingerTargetState state{
            .identity = StingerTargetIdentity::GeminiHeadA,
            .geometry = {123, 43},
        };
        const auto result = select_stinger_target(state, {}, 147);
        assert(result.identity == StingerTargetIdentity::GeminiHeadA);
        assert(result.geometry.x == 123 && result.geometry.width == 43);
        assert(result.desired_x == 144);
        assert(!result.target_changed);
    }

    return 0;
}
