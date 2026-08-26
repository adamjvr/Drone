#include <drone/formats/clv.hpp>
#include <drone/formats/demo.hpp>
#include <drone/formats/fly.hpp>
#include <drone/formats/jba.hpp>

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
        << "  drone_inspect clv-info <file.clv> [downmix.wav]\n"
        << "  drone_inspect fly-info <file.fly>\n"
        << "  drone_inspect demo-info <file.dat>\n";
}

int main(int argc, char** argv) try {
    if (argc < 3) { usage(); return 2; }
    const std::string command = argv[1];
    const fs::path input = argv[2];

    if (command == "jba-info") {
        const auto image = drone::formats::load_fullscreen_jba(input);
        std::cout << "JBA 320x200 indexed8, palette=256xRGB6, pixels=" << image.pixels.size() << "\n";
        if (argc >= 4) {
            drone::formats::write_ppm(image, argv[3]);
            std::cout << "wrote " << argv[3] << "\n";
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
        const auto records = drone::formats::load_fly(input);
        std::cout << "FLY records=" << records.size() << " storage=(int16,int16,int8)\n";
        const auto n = std::min<std::size_t>(records.size(), 8);
        for (std::size_t i = 0; i < n; ++i) {
            const auto& r = records[i];
            std::cout << i << ": " << r.field0 << ", " << r.field1 << ", " << static_cast<int>(r.field2) << "\n";
        }
    } else if (command == "demo-info") {
        const auto records = drone::formats::load_demo_dat(input);
        std::cout << "demo records=" << records.size() << ", fields=14 x int32 textual values\n";
        if (!records.empty()) {
            std::cout << "first:";
            for (auto v : records.front()) std::cout << ' ' << v;
            std::cout << "\n";
        }
    } else {
        usage(); return 2;
    }
    return 0;
} catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
}
