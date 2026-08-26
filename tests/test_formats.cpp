#include <drone/formats/clv.hpp>
#include <drone/formats/demo.hpp>
#include <drone/formats/fly.hpp>
#include <drone/formats/jba.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

int main() {
    const auto base = fs::temp_directory_path() / "drone_format_tests";
    fs::remove_all(base); fs::create_directories(base);

    // Synthetic JBA fixture encoded exactly as the recovered original loader expects.
    {
        std::vector<unsigned char> file;
        file.reserve(drone::formats::JbaImage::file_bytes);
        for (int i = 0; i < 256; ++i) {
            file.push_back(static_cast<unsigned char>(i & 63));
            file.push_back(static_cast<unsigned char>((i + 1) & 63));
            file.push_back(static_cast<unsigned char>((i + 2) & 63));
        }
        std::vector<unsigned char> expected(drone::formats::JbaImage::pixel_count);
        for (std::size_t i = 0; i < expected.size(); ++i) expected[i] = static_cast<unsigned char>((i * 17 + 3) & 255);
        for (std::size_t lane = 0; lane < 10; ++lane)
            for (std::size_t dst = lane; dst < expected.size(); dst += 10) file.push_back(expected[dst]);
        const auto path = base / "fixture.jba";
        std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(file.data()), file.size());
        const auto image = drone::formats::load_fullscreen_jba(path);
        assert(image.pixels == expected);
        assert(image.palette[1].r == 4 && image.palette[1].g == 8 && image.palette[1].b == 12);
    }

    {
        const auto path = base / "fixture.clv";
        const unsigned char samples[] = {0, 255, 10, 20, 100, 100};
        std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(samples), sizeof(samples));
        const auto clv = drone::formats::load_clv(path);
        assert(clv.frames() == 3);
        const auto mono = drone::formats::downmix_clv_to_mono(clv);
        assert((mono == std::vector<std::uint8_t>{127, 15, 100}));
    }

    {
        const auto path = base / "fixture.fly";
        std::ofstream(path) << "2\n1\n2\n3\n-4\n5\n-6\n";
        const auto fly = drone::formats::load_fly(path);
        assert(fly.size() == 2 && fly[1].field0 == -4 && fly[1].field2 == -6);
    }

    {
        const auto path = base / "fixture.dat";
        std::ofstream out(path);
        for (int i = 0; i < 28; ++i) out << i << '\n';
        out.close();
        const auto demo = drone::formats::load_demo_dat(path);
        assert(demo.size() == 2 && demo[1][13] == 27);
    }

    fs::remove_all(base);
    std::cout << "all Drone format tests passed\n";
    return 0;
}
