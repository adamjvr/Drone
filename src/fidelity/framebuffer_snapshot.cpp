#include <drone/fidelity/framebuffer_snapshot.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace drone::fidelity {
namespace {

void require_snapshot_shape(const FramebufferSnapshot& snapshot) {
    if (snapshot.pixels.size() != IndexedFramebuffer::pixel_count) {
        throw std::runtime_error("framebuffer snapshot must contain exactly 64000 indexed pixels");
    }
}

void write_u16_le(std::ostream& out, std::uint16_t value) {
    const char bytes[2]{
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8u) & 0xffu),
    };
    out.write(bytes, sizeof(bytes));
}

void write_u32_le(std::ostream& out, std::uint32_t value) {
    const char bytes[4]{
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8u) & 0xffu),
        static_cast<char>((value >> 16u) & 0xffu),
        static_cast<char>((value >> 24u) & 0xffu),
    };
    out.write(bytes, sizeof(bytes));
}

std::uint16_t read_u16_le(std::istream& in) {
    std::array<unsigned char, 2> bytes{};
    in.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!in) throw std::runtime_error("truncated framebuffer snapshot header");
    return static_cast<std::uint16_t>(bytes[0]) |
        static_cast<std::uint16_t>(bytes[1] << 8u);
}

std::uint32_t read_u32_le(std::istream& in) {
    std::array<unsigned char, 4> bytes{};
    in.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!in) throw std::runtime_error("truncated framebuffer snapshot header");
    return static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[3]) << 24u);
}

void require_region(FramebufferRect region) {
    if (region.empty()) throw std::out_of_range("framebuffer comparison region may not be empty");
    if (region.x >= IndexedFramebuffer::width || region.y >= IndexedFramebuffer::height ||
        region.width > IndexedFramebuffer::width - region.x ||
        region.height > IndexedFramebuffer::height - region.y) {
        throw std::out_of_range("framebuffer comparison region exceeds 320x200 bounds");
    }
}

FramebufferComparison compare_impl(
    const FramebufferSnapshot& reference,
    const FramebufferSnapshot& candidate,
    FramebufferRect region,
    bool compare_palette) {
    require_snapshot_shape(reference);
    require_snapshot_shape(candidate);
    require_region(region);

    FramebufferComparison result{};
    std::size_t min_x = IndexedFramebuffer::width;
    std::size_t min_y = IndexedFramebuffer::height;
    std::size_t max_x = 0;
    std::size_t max_y = 0;

    for (std::size_t y = region.y; y < region.y + region.height; ++y) {
        for (std::size_t x = region.x; x < region.x + region.width; ++x) {
            const auto offset = y * IndexedFramebuffer::width + x;
            const auto reference_index = reference.pixels[offset];
            const auto candidate_index = candidate.pixels[offset];
            if (reference_index != candidate_index) {
                ++result.pixel_mismatch_count;
                min_x = std::min(min_x, x);
                min_y = std::min(min_y, y);
                max_x = std::max(max_x, x);
                max_y = std::max(max_y, y);
            }

            const auto& reference_rgb = reference.palette[reference_index];
            const auto& candidate_rgb = candidate.palette[candidate_index];
            if (reference_rgb.r != candidate_rgb.r ||
                reference_rgb.g != candidate_rgb.g ||
                reference_rgb.b != candidate_rgb.b) {
                ++result.rendered_rgb_mismatch_count;
            }
        }
    }

    if (result.pixel_mismatch_count != 0) {
        result.pixel_mismatch_bounds = FramebufferRect{
            .x = min_x,
            .y = min_y,
            .width = max_x - min_x + 1,
            .height = max_y - min_y + 1,
        };
    }

    if (compare_palette) {
        for (std::size_t i = 0; i < reference.palette.size(); ++i) {
            const auto& a = reference.palette[i];
            const auto& b = candidate.palette[i];
            std::size_t channel_mismatches = 0;
            channel_mismatches += a.r != b.r ? 1u : 0u;
            channel_mismatches += a.g != b.g ? 1u : 0u;
            channel_mismatches += a.b != b.b ? 1u : 0u;
            if (channel_mismatches != 0) ++result.palette_entry_mismatch_count;
            result.palette_channel_mismatch_count += channel_mismatches;
        }
    }

    return result;
}

} // namespace

FramebufferSnapshot make_framebuffer_snapshot(const IndexedFramebuffer& framebuffer) {
    if (framebuffer.pixels().size() != IndexedFramebuffer::pixel_count) {
        throw std::runtime_error("fidelity framebuffer pixel buffer has wrong size");
    }
    return FramebufferSnapshot{framebuffer.pixels(), framebuffer.palette()};
}

IndexedFramebuffer make_indexed_framebuffer(const FramebufferSnapshot& snapshot) {
    require_snapshot_shape(snapshot);
    IndexedFramebuffer framebuffer;
    framebuffer.pixels() = snapshot.pixels;
    framebuffer.palette() = snapshot.palette;
    return framebuffer;
}

void write_framebuffer_snapshot(
    const FramebufferSnapshot& snapshot,
    const std::filesystem::path& path) {
    require_snapshot_shape(snapshot);

    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("unable to create framebuffer snapshot: " + path.string());

    out.write(framebuffer_snapshot_magic.data(), framebuffer_snapshot_magic.size());
    write_u16_le(out, static_cast<std::uint16_t>(IndexedFramebuffer::width));
    write_u16_le(out, static_cast<std::uint16_t>(IndexedFramebuffer::height));
    write_u16_le(out, 256);
    write_u16_le(out, 0);
    write_u32_le(out, static_cast<std::uint32_t>(IndexedFramebuffer::pixel_count));
    write_u32_le(out, static_cast<std::uint32_t>(framebuffer_snapshot_palette_bytes));
    out.write(reinterpret_cast<const char*>(snapshot.pixels.data()), snapshot.pixels.size());
    for (const auto& color : snapshot.palette) {
        const char rgb[3]{
            static_cast<char>(color.r),
            static_cast<char>(color.g),
            static_cast<char>(color.b),
        };
        out.write(rgb, sizeof(rgb));
    }
    if (!out) throw std::runtime_error("failed while writing framebuffer snapshot: " + path.string());
}

FramebufferSnapshot load_framebuffer_snapshot(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("unable to open framebuffer snapshot: " + path.string());

    std::array<char, 8> magic{};
    in.read(magic.data(), magic.size());
    if (!in || magic != framebuffer_snapshot_magic) {
        throw std::runtime_error("invalid framebuffer snapshot magic/version");
    }

    const auto width = read_u16_le(in);
    const auto height = read_u16_le(in);
    const auto palette_entries = read_u16_le(in);
    const auto reserved = read_u16_le(in);
    const auto pixel_bytes = read_u32_le(in);
    const auto palette_bytes = read_u32_le(in);

    if (width != IndexedFramebuffer::width || height != IndexedFramebuffer::height ||
        palette_entries != 256 || reserved != 0 ||
        pixel_bytes != IndexedFramebuffer::pixel_count ||
        palette_bytes != framebuffer_snapshot_palette_bytes) {
        throw std::runtime_error("unsupported framebuffer snapshot geometry/layout");
    }

    FramebufferSnapshot snapshot;
    snapshot.pixels.resize(IndexedFramebuffer::pixel_count);
    in.read(reinterpret_cast<char*>(snapshot.pixels.data()), snapshot.pixels.size());
    if (!in) throw std::runtime_error("truncated framebuffer snapshot pixel plane");

    for (auto& color : snapshot.palette) {
        std::array<unsigned char, 3> rgb{};
        in.read(reinterpret_cast<char*>(rgb.data()), rgb.size());
        if (!in) throw std::runtime_error("truncated framebuffer snapshot palette");
        color = {rgb[0], rgb[1], rgb[2]};
    }

    char trailing{};
    if (in.read(&trailing, 1)) {
        throw std::runtime_error("framebuffer snapshot contains trailing bytes");
    }
    return snapshot;
}

FramebufferComparison compare_framebuffer_snapshots(
    const FramebufferSnapshot& reference,
    const FramebufferSnapshot& candidate) {
    return compare_impl(
        reference,
        candidate,
        FramebufferRect{0, 0, IndexedFramebuffer::width, IndexedFramebuffer::height},
        true);
}

FramebufferComparison compare_framebuffer_snapshot_region(
    const FramebufferSnapshot& reference,
    const FramebufferSnapshot& candidate,
    FramebufferRect region) {
    return compare_impl(reference, candidate, region, false);
}

void write_framebuffer_snapshot_ppm(
    const FramebufferSnapshot& snapshot,
    const std::filesystem::path& path) {
    require_snapshot_shape(snapshot);
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("unable to create framebuffer preview: " + path.string());
    out << "P6\n" << IndexedFramebuffer::width << ' ' << IndexedFramebuffer::height << "\n255\n";
    for (const auto index : snapshot.pixels) {
        const auto& color = snapshot.palette[index];
        const char rgb[3]{
            static_cast<char>(color.r),
            static_cast<char>(color.g),
            static_cast<char>(color.b),
        };
        out.write(rgb, sizeof(rgb));
    }
    if (!out) throw std::runtime_error("failed while writing framebuffer preview: " + path.string());
}

} // namespace drone::fidelity
