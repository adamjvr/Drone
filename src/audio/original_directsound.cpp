#include <drone/audio/original_directsound.hpp>

#include <array>

namespace drone::audio {


namespace {

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
