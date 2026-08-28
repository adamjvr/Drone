#include <drone/fidelity/framebuffer_snapshot.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using drone::fidelity::FramebufferRect;
using drone::fidelity::FramebufferSnapshot;

namespace {

void usage() {
    std::cerr
        << "Drone indexed-framebuffer snapshot/comparison tool\n\n"
        << "Usage:\n"
        << "  drone_framecheck info <frame.drfb>\n"
        << "  drone_framecheck compare <reference.drfb> <candidate.drfb>\n"
        << "  drone_framecheck compare-region <reference.drfb> <candidate.drfb> <x> <y> <w> <h>\n"
        << "  drone_framecheck from-raw <indices.bin> <palette.rgb> <output.drfb>\n"
        << "  drone_framecheck to-ppm <frame.drfb> <output.ppm>\n";
}

std::vector<std::uint8_t> read_exact(const fs::path& path, std::size_t size) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("unable to open raw input: " + path.string());
    std::vector<std::uint8_t> bytes(size);
    in.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (static_cast<std::size_t>(in.gcount()) != size) {
        throw std::runtime_error("raw input has unexpected size: " + path.string());
    }
    char trailing{};
    if (in.read(&trailing, 1)) {
        throw std::runtime_error("raw input has unexpected trailing bytes: " + path.string());
    }
    return bytes;
}

void print_comparison(const drone::fidelity::FramebufferComparison& result) {
    std::cout << "pixel-mismatches=" << result.pixel_mismatch_count
              << " rendered-rgb-mismatches=" << result.rendered_rgb_mismatch_count
              << " palette-entry-mismatches=" << result.palette_entry_mismatch_count
              << " palette-channel-mismatches=" << result.palette_channel_mismatch_count;
    if (result.pixel_mismatch_bounds) {
        const auto& b = *result.pixel_mismatch_bounds;
        std::cout << " mismatch-bounds=" << b.x << ',' << b.y << ',' << b.width << ',' << b.height;
    } else {
        std::cout << " mismatch-bounds=none";
    }
    std::cout << " exact=" << (result.exact() ? "yes" : "no") << '\n';
}

} // namespace

int main(int argc, char** argv) try {
    if (argc < 3) {
        usage();
        return 2;
    }

    const std::string command = argv[1];
    if (command == "info") {
        if (argc != 3) { usage(); return 2; }
        const auto snapshot = drone::fidelity::load_framebuffer_snapshot(argv[2]);
        std::cout << "DRONEFB1 320x200 indexed8 pixels=" << snapshot.pixels.size()
                  << " palette=256xRGB8 file-bytes="
                  << drone::fidelity::framebuffer_snapshot_file_size << '\n';
        return 0;
    }

    if (command == "compare") {
        if (argc != 4) { usage(); return 2; }
        const auto reference = drone::fidelity::load_framebuffer_snapshot(argv[2]);
        const auto candidate = drone::fidelity::load_framebuffer_snapshot(argv[3]);
        const auto result = drone::fidelity::compare_framebuffer_snapshots(reference, candidate);
        print_comparison(result);
        return result.exact() ? 0 : 1;
    }

    if (command == "compare-region") {
        if (argc != 8) { usage(); return 2; }
        const auto reference = drone::fidelity::load_framebuffer_snapshot(argv[2]);
        const auto candidate = drone::fidelity::load_framebuffer_snapshot(argv[3]);
        FramebufferRect region{
            static_cast<std::size_t>(std::stoull(argv[4])),
            static_cast<std::size_t>(std::stoull(argv[5])),
            static_cast<std::size_t>(std::stoull(argv[6])),
            static_cast<std::size_t>(std::stoull(argv[7])),
        };
        const auto result = drone::fidelity::compare_framebuffer_snapshot_region(
            reference, candidate, region);
        print_comparison(result);
        return result.pixel_mismatch_count == 0 && result.rendered_rgb_mismatch_count == 0 ? 0 : 1;
    }

    if (command == "from-raw") {
        if (argc != 5) { usage(); return 2; }
        FramebufferSnapshot snapshot;
        snapshot.pixels = read_exact(argv[2], drone::fidelity::IndexedFramebuffer::pixel_count);
        const auto palette = read_exact(argv[3], drone::fidelity::framebuffer_snapshot_palette_bytes);
        for (std::size_t i = 0; i < snapshot.palette.size(); ++i) {
            snapshot.palette[i] = {
                palette[i * 3 + 0], palette[i * 3 + 1], palette[i * 3 + 2]};
        }
        drone::fidelity::write_framebuffer_snapshot(snapshot, argv[4]);
        std::cout << "wrote " << argv[4] << " (DRONEFB1 320x200 indexed8 + 256xRGB8)\n";
        return 0;
    }

    if (command == "to-ppm") {
        if (argc != 4) { usage(); return 2; }
        const auto snapshot = drone::fidelity::load_framebuffer_snapshot(argv[2]);
        drone::fidelity::write_framebuffer_snapshot_ppm(snapshot, argv[3]);
        std::cout << "wrote " << argv[3] << '\n';
        return 0;
    }

    usage();
    return 2;
} catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 2;
}
