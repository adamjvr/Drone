#include <drone/audio/original_directsound.hpp>

#include <array>

namespace drone::audio {


namespace {

constexpr std::array<OriginalTrajectoryPoolInitialization, 14> trajectory_pool_initializations{{
    {0x0041FBE6u, 0x0042BEF4u, 0x0042F1A8u, 0x0042F1F8u, "squad1.wav", original_trajectory_squad_volume, 0},
    {0x0041FC34u, 0x0042BEE8u, 0x00440F58u, 0x00440FA8u, "squad2.wav", original_trajectory_squad_volume, 0},
    {0x0041FC81u, 0x0042BEDCu, 0x0045A2A8u, 0x0045A2F8u, "squad3.wav", original_trajectory_squad_volume, 0},
    {0x0041FCCFu, 0x0042BED0u, 0x00467390u, 0x004673E0u, "squad4.wav", original_trajectory_squad_volume, 0},
    {0x0041FD1Du, 0x0042BEC4u, 0x004339B0u, 0x00433A00u, "squad5.wav", original_trajectory_squad_volume, 0},
    {0x0041FD6Au, 0x0042BEB8u, 0x004461E0u, 0x00446230u, "squad6.wav", original_trajectory_squad_volume, 0},
    {0x0041FDB8u, 0x0042BEACu, 0x0047FE48u, 0x0047FE98u, "squad7.wav", original_trajectory_squad_volume, 0},
    {0x0041FE06u, 0x0042BEA0u, 0x00438A70u, 0x00438AC0u, "squad8.wav", original_trajectory_squad_volume, 0},
    {0x0041FE53u, 0x0042BE94u, 0x00446290u, 0x004462E0u, "squad9.wav", original_trajectory_squad_volume, 0},
    {0x0041FEA1u, 0x0042BE88u, 0x00440FA8u, 0x00440FF8u, "squad10.wav", original_trajectory_squad_volume, 0},
    {0x0041FEEFu, 0x0042BE7Cu, 0x00432488u, 0x004324D8u, "squad11.wav", original_trajectory_squad_volume, 0},
    {0x0041FF3Cu, 0x0042BE70u, 0x004677F0u, 0x00467840u, "squad12.wav", original_trajectory_squad_volume, 0},
    {0x0041FF8Au, 0x0042BE64u, 0x004629F8u, 0x00462A48u, "squad13.wav", original_trajectory_squad_volume, 0},
    {0x0041FFD8u, 0x0042BE58u, 0x00441770u, 0x004417C0u, "squad14.wav", original_trajectory_squad_volume, 0},
}};

constexpr std::array<OriginalLoopCallSite, 13> loop_call_sites{{
    {0x00404498u, 0x00466C88u, "bomber1.wav",
     OriginalLoopPlaybackOwner::BomberBoss, OriginalLoopFlagProof::LiteralOne},
    {0x0040474Du, 0x00000000u, "credits.wav",
     OriginalLoopPlaybackOwner::CompletionCredits, OriginalLoopFlagProof::LiteralOne},
    {0x00405FA1u, 0x00495CE4u, "gemini.wav",
     OriginalLoopPlaybackOwner::GeminiBoss, OriginalLoopFlagProof::LiteralOne},
    {0x00407AA3u, 0x0042EFE0u, "",
     OriginalLoopPlaybackOwner::RegisteredBossSlot2Unresolved,
     OriginalLoopFlagProof::LiteralOne},
    {0x0040C8F3u, 0x0043F5F4u, "air.wav",
     OriginalLoopPlaybackOwner::State2AirStart, OriginalLoopFlagProof::RegisterProvenOne},
    {0x0040E52Fu, 0x0047E280u, "drone.wav",
     OriginalLoopPlaybackOwner::State2DroneLoop, OriginalLoopFlagProof::RegisterProvenOne},
    {0x00415C72u, 0x00454B00u, "spidey.wav",
     OriginalLoopPlaybackOwner::SpideyBoss, OriginalLoopFlagProof::LiteralOne},
    {0x00417323u, 0x00438C18u, "retro1.wav",
     OriginalLoopPlaybackOwner::LidTopBoss, OriginalLoopFlagProof::LiteralOne},
    {0x00418B14u, 0x0053C4E8u, "lowbees.wav",
     OriginalLoopPlaybackOwner::MainMenuLowBees, OriginalLoopFlagProof::LiteralOne},
    {0x0041A3EEu, 0x0043F5F4u, "air.wav",
     OriginalLoopPlaybackOwner::MainMenuAirRestart, OriginalLoopFlagProof::RegisterProvenOne},
    {0x0041B75Bu, 0x00000000u, "thunder2.wav",
     OriginalLoopPlaybackOwner::OrderingInformation, OriginalLoopFlagProof::LiteralOne},
    {0x0041E298u, 0x0042EFE8u, "thunder2.wav",
     OriginalLoopPlaybackOwner::PostEncounterThunderStart,
     OriginalLoopFlagProof::RegisterProvenOne},
    {0x0041E395u, 0x0043F5F4u, "air.wav",
     OriginalLoopPlaybackOwner::PostEncounterAirRestart,
     OriginalLoopFlagProof::RegisterProvenOne},
}};

} // namespace

std::span<const OriginalTrajectoryPoolInitialization>
original_trajectory_pool_initializations() noexcept {
    return trajectory_pool_initializations;
}

std::span<const OriginalLoopCallSite> original_directsound_loop_call_sites() noexcept {
    return loop_call_sites;
}
std::size_t select_original_sfx_voice(
    const std::array<std::uint32_t, original_sfx_voice_pool_capacity>& raw_status) noexcept {
    for (std::size_t index = 0; index < raw_status.size(); ++index) {
        if (raw_status[index] != directsound_status_playing) {
            return index;
        }
    }
    return 0;
}

} // namespace drone::audio
