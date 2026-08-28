#include <drone/formats/jba.hpp>

#include <fstream>
#include <stdexcept>

namespace drone::formats {
namespace {
std::vector<std::uint8_t> read_all(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("could not open JBA: " + path.string());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
}

JbaImage load_jba_320x200(const std::filesystem::path& path) {
    const auto bytes = read_all(path);
    if (bytes.size() != JbaImage::file_bytes) {
        throw std::runtime_error("not a 64,768-byte 320x200 JBA image/sheet: " + path.string());
    }

    JbaImage out;
    out.pixels.resize(JbaImage::pixel_count);

    // Recovered from Win32 0x4012B0 and independently matched in the DOS build.
    // On disk each VGA palette component is stored as a 6-bit value. The original
    // Win32 loader promotes it to an 8-bit DAC-style component by shifting left 2.
    for (std::size_t i = 0; i < 256; ++i) {
        const auto base = i * 3;
        out.palette[i] = {
            static_cast<std::uint8_t>(bytes[base + 0] << 2),
            static_cast<std::uint8_t>(bytes[base + 1] << 2),
            static_cast<std::uint8_t>(bytes[base + 2] << 2),
        };
    }

    std::size_t src = JbaImage::palette_bytes;
    for (std::size_t lane = 0; lane < 10; ++lane) {
        for (std::size_t dst = lane; dst < JbaImage::pixel_count; dst += 10) {
            out.pixels[dst] = bytes[src++];
        }
    }
    if (src != bytes.size()) throw std::runtime_error("internal JBA decode length mismatch");
    return out;
}

void write_ppm(const JbaImage& image, const std::filesystem::path& path) {
    if (image.pixels.size() != JbaImage::pixel_count) {
        throw std::runtime_error("JBA pixel buffer has wrong size");
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("could not create PPM: " + path.string());
    out << "P6\n" << JbaImage::width << " " << JbaImage::height << "\n255\n";
    for (const auto index : image.pixels) {
        const auto& c = image.palette[index];
        out.put(static_cast<char>(c.r));
        out.put(static_cast<char>(c.g));
        out.put(static_cast<char>(c.b));
    }
}

} // namespace drone::formats
