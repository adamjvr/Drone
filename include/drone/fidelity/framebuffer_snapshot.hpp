#pragma once

#include <drone/fidelity/indexed_framebuffer.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drone::fidelity {

inline constexpr std::array<char, 8> framebuffer_snapshot_magic{
    'D', 'R', 'O', 'N', 'E', 'F', 'B', '1'};
inline constexpr std::size_t framebuffer_snapshot_header_size = 24;
inline constexpr std::size_t framebuffer_snapshot_palette_bytes = 256 * 3;
inline constexpr std::size_t framebuffer_snapshot_file_size =
    framebuffer_snapshot_header_size + IndexedFramebuffer::pixel_count +
    framebuffer_snapshot_palette_bytes;

struct FramebufferSnapshot {
    std::vector<std::uint8_t> pixels;
    std::array<formats::Rgb8, 256> palette{};
};

struct FramebufferRect {
    std::size_t x{};
    std::size_t y{};
    std::size_t width{};
    std::size_t height{};

    [[nodiscard]] bool empty() const noexcept { return width == 0 || height == 0; }
};

struct FramebufferComparison {
    std::size_t pixel_mismatch_count{};
    std::size_t rendered_rgb_mismatch_count{};
    std::size_t palette_entry_mismatch_count{};
    std::size_t palette_channel_mismatch_count{};
    std::optional<FramebufferRect> pixel_mismatch_bounds;

    [[nodiscard]] bool exact() const noexcept {
        return pixel_mismatch_count == 0 && rendered_rgb_mismatch_count == 0 &&
            palette_channel_mismatch_count == 0;
    }
};

[[nodiscard]] FramebufferSnapshot make_framebuffer_snapshot(const IndexedFramebuffer& framebuffer);
[[nodiscard]] IndexedFramebuffer make_indexed_framebuffer(const FramebufferSnapshot& snapshot);

void write_framebuffer_snapshot(
    const FramebufferSnapshot& snapshot,
    const std::filesystem::path& path);
[[nodiscard]] FramebufferSnapshot load_framebuffer_snapshot(const std::filesystem::path& path);

[[nodiscard]] FramebufferComparison compare_framebuffer_snapshots(
    const FramebufferSnapshot& reference,
    const FramebufferSnapshot& candidate);

[[nodiscard]] FramebufferComparison compare_framebuffer_snapshot_region(
    const FramebufferSnapshot& reference,
    const FramebufferSnapshot& candidate,
    FramebufferRect region);

void write_framebuffer_snapshot_ppm(
    const FramebufferSnapshot& snapshot,
    const std::filesystem::path& path);

} // namespace drone::fidelity
