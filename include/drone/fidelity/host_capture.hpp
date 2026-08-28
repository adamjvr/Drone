#pragma once

#include <drone/fidelity/indexed_framebuffer.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace drone::fidelity {

struct FidelityHostCaptureLandmark {
    std::string label{"frame"};
    std::uint64_t sequence{};
};

[[nodiscard]] std::string sanitize_fidelity_capture_label(std::string_view label);

[[nodiscard]] std::filesystem::path fidelity_capture_landmark_path(
    const std::filesystem::path& directory,
    const FidelityHostCaptureLandmark& landmark);

// Writes a DRONEFB1 snapshot of the supplied const framebuffer. The capture
// path is observational: it does not mutate framebuffer pixels or palette.
void write_fidelity_host_capture(
    const IndexedFramebuffer& framebuffer,
    const std::filesystem::path& path);

[[nodiscard]] std::filesystem::path write_fidelity_host_landmark_capture(
    const IndexedFramebuffer& framebuffer,
    const std::filesystem::path& directory,
    const FidelityHostCaptureLandmark& landmark);

} // namespace drone::fidelity
