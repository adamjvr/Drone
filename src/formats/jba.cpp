#include <drone/formats/jba.hpp>

#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace drone::formats {
namespace {
std::vector<std::uint8_t> read_all(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("could not open JBA: " + path.string());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::uint16_t read_le16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 2 > bytes.size()) throw std::runtime_error("truncated small-JBA PCX header");
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(bytes[offset + 1] << 8);
}

template <typename Image>
void write_indexed_ppm(const Image& image, const std::filesystem::path& path,
                       std::size_t width, std::size_t height) {
    if (image.pixels.size() != width * height) {
        throw std::runtime_error("indexed image pixel buffer has wrong size");
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("could not create PPM: " + path.string());
    out << "P6\n" << width << " " << height << "\n255\n";
    for (const auto index : image.pixels) {
        const auto& c = image.palette[index];
        out.put(static_cast<char>(c.r));
        out.put(static_cast<char>(c.g));
        out.put(static_cast<char>(c.b));
    }
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

SmallJbaPcxImage load_small_jba_pcx128(const std::filesystem::path& path) {
    const auto bytes = read_all(path);
    constexpr auto minimum_size = 1u + SmallJbaPcxImage::pcx_header_bytes +
                                  SmallJbaPcxImage::palette_bytes;
    if (bytes.size() < minimum_size) {
        throw std::runtime_error("small-JBA PCX container is too short: " + path.string());
    }

    SmallJbaPcxImage out;
    out.preamble_length = bytes[0];
    const std::size_t pcx_offset = 1u + static_cast<std::size_t>(out.preamble_length);
    if (pcx_offset + SmallJbaPcxImage::pcx_header_bytes + SmallJbaPcxImage::palette_bytes > bytes.size()) {
        throw std::runtime_error("small-JBA preamble extends beyond container: " + path.string());
    }
    out.opaque_preamble.assign(bytes.begin() + 1, bytes.begin() + static_cast<std::ptrdiff_t>(pcx_offset));

    // All three canonical Windows-small-JBA members agree on this exact PCX
    // header contract. PaletteInfo is zero in the files and is intentionally
    // not used as a decoder requirement; the physical trailer establishes the
    // palette representation directly.
    if (bytes[pcx_offset + 0] != 0x0A || // ZSoft PCX manufacturer
        bytes[pcx_offset + 1] != 0x05 || // version 5
        bytes[pcx_offset + 2] != 0x01 || // RLE encoding
        bytes[pcx_offset + 3] != 0x08) { // 8 bits per pixel per plane
        throw std::runtime_error("small-JBA payload is not canonical 8-bit PCX: " + path.string());
    }

    const auto xmin = read_le16(bytes, pcx_offset + 4);
    const auto ymin = read_le16(bytes, pcx_offset + 6);
    const auto xmax = read_le16(bytes, pcx_offset + 8);
    const auto ymax = read_le16(bytes, pcx_offset + 10);
    const auto hres = read_le16(bytes, pcx_offset + 12);
    const auto vres = read_le16(bytes, pcx_offset + 14);
    const auto planes = bytes[pcx_offset + 65];
    const auto bytes_per_line = read_le16(bytes, pcx_offset + 66);

    if (xmin != 0 || ymin != 0 || xmax != 127 || ymax != 127 ||
        hres != 128 || vres != 128 || planes != 1 || bytes_per_line != 128) {
        throw std::runtime_error("small-JBA PCX geometry differs from canonical 128x128 family: " + path.string());
    }

    out.pixels.reserve(SmallJbaPcxImage::pixel_count);
    std::size_t src = pcx_offset + SmallJbaPcxImage::pcx_header_bytes;
    const std::size_t palette_start = bytes.size() - SmallJbaPcxImage::palette_bytes;

    while (out.pixels.size() < SmallJbaPcxImage::pixel_count) {
        if (src >= palette_start) {
            throw std::runtime_error("truncated small-JBA PCX RLE stream: " + path.string());
        }
        const std::uint8_t token = bytes[src++];
        std::size_t count = 1;
        std::uint8_t value = token;
        if ((token & 0xC0u) == 0xC0u) {
            count = token & 0x3Fu;
            if (count == 0 || src >= palette_start) {
                throw std::runtime_error("invalid small-JBA PCX RLE run: " + path.string());
            }
            value = bytes[src++];
        }
        if (count > SmallJbaPcxImage::pixel_count - out.pixels.size()) {
            throw std::runtime_error("small-JBA PCX RLE run exceeds 128x128 image: " + path.string());
        }
        out.pixels.insert(out.pixels.end(), count, value);
    }

    // Canonical files transition directly from RLE data to 768 RGB8 bytes.
    // Unlike conventional 256-color PCX, there is no 0x0C palette marker.
    if (src != palette_start) {
        throw std::runtime_error("small-JBA PCX has unexpected bytes between image and raw palette: " + path.string());
    }

    for (std::size_t i = 0; i < 256; ++i) {
        const auto base = palette_start + i * 3;
        out.palette[i] = {bytes[base], bytes[base + 1], bytes[base + 2]};
    }
    return out;
}

void write_ppm(const JbaImage& image, const std::filesystem::path& path) {
    write_indexed_ppm(image, path, JbaImage::width, JbaImage::height);
}

void write_ppm(const SmallJbaPcxImage& image, const std::filesystem::path& path) {
    write_indexed_ppm(image, path, SmallJbaPcxImage::width, SmallJbaPcxImage::height);
}

} // namespace drone::formats
