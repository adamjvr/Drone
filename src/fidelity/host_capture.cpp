#include <drone/fidelity/host_capture.hpp>

#include <drone/fidelity/framebuffer_snapshot.hpp>

#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace drone::fidelity {

std::string sanitize_fidelity_capture_label(std::string_view label) {
    std::string out;
    out.reserve(label.size());
    bool pending_dash = false;
    for (const unsigned char ch : label) {
        if (std::isalnum(ch) != 0) {
            if (pending_dash && !out.empty()) out.push_back('-');
            out.push_back(static_cast<char>(std::tolower(ch)));
            pending_dash = false;
        } else {
            pending_dash = true;
        }
    }
    if (out.empty()) return "frame";
    return out;
}

std::filesystem::path fidelity_capture_landmark_path(
    const std::filesystem::path& directory,
    const FidelityHostCaptureLandmark& landmark) {
    std::ostringstream name;
    name << std::setw(8) << std::setfill('0') << landmark.sequence
         << '-' << sanitize_fidelity_capture_label(landmark.label) << ".drfb";
    return directory / name.str();
}

void write_fidelity_host_capture(
    const IndexedFramebuffer& framebuffer,
    const std::filesystem::path& path) {
    if (path.empty()) throw std::invalid_argument("fidelity capture path may not be empty");
    const auto parent = path.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    write_framebuffer_snapshot(make_framebuffer_snapshot(framebuffer), path);
}

std::filesystem::path write_fidelity_host_landmark_capture(
    const IndexedFramebuffer& framebuffer,
    const std::filesystem::path& directory,
    const FidelityHostCaptureLandmark& landmark) {
    if (directory.empty()) throw std::invalid_argument("fidelity capture directory may not be empty");
    std::filesystem::create_directories(directory);
    const auto path = fidelity_capture_landmark_path(directory, landmark);
    write_fidelity_host_capture(framebuffer, path);
    return path;
}

} // namespace drone::fidelity
