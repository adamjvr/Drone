#include <drone/formats/clv.hpp>
#include <drone/formats/demo.hpp>
#include <drone/formats/fly.hpp>
#include <drone/formats/jba.hpp>
#include <drone/fidelity/sprite_sheet.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

static void usage() {
    std::cerr
        << "Drone reverse-engineering asset inspector\n\n"
        << "Usage:\n"
        << "  drone_inspect jba-info <file.jba> [preview.ppm]\n"
        << "  drone_inspect jba-grid-frame <file.jba> <width> <height> <cell-x> <cell-y> [preview.ppm]\n"
        << "  drone_inspect clv-info <file.clv> [downmix.wav]\n"
        << "  drone_inspect fly-info <file.fly> [loader-record-count]\n"
        << "  drone_inspect demo-info <file.dat>\n";
}

int main(int argc, char** argv) try {
    if (argc < 3) { usage(); return 2; }
    const std::string command = argv[1];
    const fs::path input = argv[2];

    if (command == "jba-info") {
        const auto image = drone::formats::load_jba_320x200(input);
        std::cout << "JBA 320x200 indexed8, palette=256xRGB6, pixels=" << image.pixels.size() << "\n";
        if (argc >= 4) {
            drone::formats::write_ppm(image, argv[3]);
            std::cout << "wrote " << argv[3] << "\n";
        }
    } else if (command == "jba-grid-frame") {
        if (argc < 7) { usage(); return 2; }
        const auto width = static_cast<std::size_t>(std::stoull(argv[3]));
        const auto height = static_cast<std::size_t>(std::stoull(argv[4]));
        const auto cell_x = static_cast<std::size_t>(std::stoull(argv[5]));
        const auto cell_y = static_cast<std::size_t>(std::stoull(argv[6]));
        const auto sheet = drone::formats::load_jba_320x200(input);
        const auto frame = drone::fidelity::extract_guttered_jba_frame(sheet, width, height, cell_x, cell_y);
        std::cout << "JBA sprite frame " << frame.width << 'x' << frame.height
                  << ", cell=(" << cell_x << ',' << cell_y << ")"
                  << ", source-origin=(" << (cell_x * (width + 1) + 1)
                  << ',' << (cell_y * (height + 1) + 1) << ")\n";
        if (argc >= 8) {
            drone::fidelity::write_sprite_ppm(frame, sheet.palette, argv[7]);
            std::cout << "wrote " << argv[7] << "\n";
        }
    } else if (command == "clv-info") {
        const auto audio = drone::formats::load_clv(input);
        std::cout << "CLV unsigned8 stereo, 22050 Hz, frames=" << audio.frames()
                  << ", seconds=" << (static_cast<double>(audio.frames()) / audio.sample_rate) << "\n";
        if (argc >= 4) {
            auto mono = drone::formats::downmix_clv_to_mono(audio);
            drone::formats::write_mono_u8_wav(mono, audio.sample_rate, argv[3]);
            std::cout << "wrote " << argv[3] << " using recovered floor-average stereo->mono rule\n";
        }
    } else if (command == "fly-info") {
        std::vector<drone::formats::FlyRecord> records;
        const auto filename = input.filename().string();
        std::size_t loader_count = 0;
        bool counted = false;
        if (filename == "Current.fly" || filename == "CURRENT.FLY" || filename == "current.fly") {
            records = drone::formats::load_counted_fly(input);
            counted = true;
        } else if (argc >= 4) {
            loader_count = static_cast<std::size_t>(std::stoull(argv[3]));
            records = drone::formats::load_raw_fly(input, loader_count, false);
        } else if (const auto known = drone::formats::known_fly_asset(filename)) {
            loader_count = known->loader_record_count;
            records = drone::formats::load_raw_fly(input, loader_count, false);
        } else {
            throw std::runtime_error("raw trajectory FLY requires an established loader record count");
        }
        std::cout << "FLY encoding=" << (counted ? "counted-current" : "raw-trajectory")
                  << ", records-read=" << records.size()
                  << ", storage=(int16 x,int16 y,int8 aux)";
        if (!counted) std::cout << ", original-loader-count=" << loader_count;
        std::cout << "\n";
        if (!counted && records.size() != loader_count) {
            std::cout << "NOTE: canonical file is shorter than the original loader request; no synthetic record was added\n";
        }
        const auto n = std::min<std::size_t>(records.size(), 8);
        for (std::size_t i = 0; i < n; ++i) {
            const auto& r = records[i];
            std::cout << i << ": x=" << r.x << ", y=" << r.y << ", aux=" << static_cast<int>(r.aux) << "\n";
        }
    } else if (command == "demo-info") {
        const auto records = drone::formats::load_demo_dat(input);
        const auto frames = drone::formats::load_demo_frames(input);
        std::size_t trajectory_events = 0;
        std::size_t bomb_events = 0;
        for (const auto& frame : frames) {
            trajectory_events += frame.has_trajectory_group_event() ? 1u : 0u;
            bomb_events += frame.bomb_spawned ? 1u : 0u;
        }
        std::cout << "demo records=" << records.size()
                  << ", fields=14 ASCII integers"
                  << ", trajectory-events=" << trajectory_events
                  << ", bomb-events=" << bomb_events << "\n";
        if (!records.empty()) {
            std::cout << "first raw:";
            for (auto v : records.front()) std::cout << ' ' << v;
            const auto& f = frames.front();
            std::cout << "\nfirst semantic: left=" << f.left
                      << " right=" << f.right
                      << " special-up=" << f.launch_special
                      << " special-down=" << f.load_cycle_special
                      << " shield=" << f.shield
                      << " rapid-missile=" << f.rapid_missile
                      << " trajectory-slot=" << f.trajectory_group_slot
                      << " trajectory-x-offset=" << f.trajectory_group_x_offset
                      << " trajectory-family=" << f.trajectory_path_family
                      << " bomb=" << f.bomb_spawned
                      << " bomb-xy=(" << f.bomb_x << ',' << f.bomb_y << ')'
                      << " drone-xy=(" << f.drone_x << ',' << f.drone_y << ")\n";
        }
    } else {
        usage(); return 2;
    }
    return 0;
} catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
}
