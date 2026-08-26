#include <drone/formats/clv.hpp>

#include <array>
#include <fstream>
#include <stdexcept>

namespace drone::formats {
namespace {
void write_u16(std::ostream& out, std::uint16_t v) {
    out.put(static_cast<char>(v & 0xff));
    out.put(static_cast<char>((v >> 8) & 0xff));
}
void write_u32(std::ostream& out, std::uint32_t v) {
    for (int s = 0; s < 32; s += 8) out.put(static_cast<char>((v >> s) & 0xff));
}
}

ClvAudio load_clv(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("could not open CLV: " + path.string());
    ClvAudio result;
    result.interleaved_stereo_u8.assign(std::istreambuf_iterator<char>(in), {});
    if (result.interleaved_stereo_u8.size() % 2 != 0) {
        throw std::runtime_error("CLV has an odd byte count; expected stereo U8 frames");
    }
    return result;
}

std::vector<std::uint8_t> downmix_clv_to_mono(const ClvAudio& audio) {
    std::vector<std::uint8_t> mono;
    mono.reserve(audio.frames());
    for (std::size_t i = 0; i < audio.interleaved_stereo_u8.size(); i += 2) {
        const unsigned l = audio.interleaved_stereo_u8[i];
        const unsigned r = audio.interleaved_stereo_u8[i + 1];
        mono.push_back(static_cast<std::uint8_t>((l + r) / 2));
    }
    return mono;
}

void write_mono_u8_wav(const std::vector<std::uint8_t>& samples,
                       std::uint32_t sample_rate,
                       const std::filesystem::path& path) {
    if (samples.size() > 0xffffffffu - 36u) throw std::runtime_error("WAV too large");
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("could not create WAV: " + path.string());
    const auto data_size = static_cast<std::uint32_t>(samples.size());
    out.write("RIFF", 4); write_u32(out, 36u + data_size); out.write("WAVE", 4);
    out.write("fmt ", 4); write_u32(out, 16); write_u16(out, 1); write_u16(out, 1);
    write_u32(out, sample_rate); write_u32(out, sample_rate); write_u16(out, 1); write_u16(out, 8);
    out.write("data", 4); write_u32(out, data_size);
    out.write(reinterpret_cast<const char*>(samples.data()), static_cast<std::streamsize>(samples.size()));
}

} // namespace drone::formats
