#include <drone/formats/demo.hpp>

#include <fstream>
#include <limits>
#include <stdexcept>

namespace drone::formats {
namespace {

std::int32_t original_signed_byte(std::int32_t value) noexcept {
    const auto low = static_cast<std::uint32_t>(value) & 0xffu;
    return low < 0x80u ? static_cast<std::int32_t>(low)
                       : static_cast<std::int32_t>(low) - 0x100;
}

std::int32_t original_signed_word(std::int32_t value) noexcept {
    const auto low = static_cast<std::uint32_t>(value) & 0xffffu;
    return low < 0x8000u ? static_cast<std::int32_t>(low)
                         : static_cast<std::int32_t>(low) - 0x10000;
}

} // namespace

std::vector<DemoRecord> load_demo_dat(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("could not open demo DAT: " + path.string());
    std::vector<std::int32_t> values;
    long long value;
    while (in >> value) {
        if (value < std::numeric_limits<std::int32_t>::min() ||
            value > std::numeric_limits<std::int32_t>::max()) {
            throw std::runtime_error("demo value outside int32 range");
        }
        values.push_back(static_cast<std::int32_t>(value));
    }
    if (!in.eof()) throw std::runtime_error("non-integer data in demo DAT");
    if (values.size() % 14 != 0) throw std::runtime_error("demo DAT value count is not divisible by 14");
    std::vector<DemoRecord> records(values.size() / 14);
    for (std::size_t r = 0; r < records.size(); ++r) {
        for (std::size_t f = 0; f < 14; ++f) records[r][f] = values[r * 14 + f];
    }
    return records;
}

DemoFrame decode_demo_record(const DemoRecord& record) {
    // The canonical Win32 loader passes addresses inside signed byte/word
    // channel arrays to its integer fscanf-like routine. Gameplay later reads
    // only the low byte/word, so semantic decoding intentionally reproduces
    // that narrowing instead of rejecting textual values outside the nominal
    // channel width.
    const auto b1 = original_signed_byte(record[0]);
    const auto b2 = original_signed_byte(record[1]);
    const auto b3 = original_signed_byte(record[2]);
    const auto b4 = original_signed_byte(record[3]);
    const auto b5 = original_signed_byte(record[4]);
    const auto b6 = original_signed_byte(record[5]);
    const auto b7 = original_signed_byte(record[6]);
    const auto w8 = original_signed_word(record[7]);
    const auto b9 = original_signed_byte(record[8]);
    const auto b10 = original_signed_byte(record[9]);
    const auto w11 = original_signed_word(record[10]);
    const auto w12 = original_signed_word(record[11]);
    const auto w13 = original_signed_word(record[12]);
    const auto w14 = original_signed_word(record[13]);

    DemoFrame frame;
    frame.raw = record;
    frame.left = b1 != 0;
    frame.right = b2 != 0;
    frame.launch_special = b3 != 0;
    frame.load_cycle_special = b4 != 0;
    frame.shield = b5 != 0;
    frame.rapid_missile = b6 != 0;
    frame.trajectory_group_slot = b7;
    frame.trajectory_group_x_offset = w8;
    frame.trajectory_path_family = b9;
    frame.bomb_spawned = b10 != 0;
    frame.bomb_x = w11;
    frame.bomb_y = w12;
    frame.drone_x = w13;
    frame.drone_y = w14;
    return frame;
}

std::vector<DemoFrame> load_demo_frames(const std::filesystem::path& path) {
    const auto records = load_demo_dat(path);
    std::vector<DemoFrame> frames;
    frames.reserve(records.size());
    for (const auto& record : records) frames.push_back(decode_demo_record(record));
    return frames;
}

} // namespace drone::formats
