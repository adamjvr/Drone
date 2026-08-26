#include <drone/formats/demo.hpp>

#include <fstream>
#include <stdexcept>

namespace drone::formats {
std::vector<DemoRecord> load_demo_dat(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("could not open demo DAT: " + path.string());
    std::vector<std::int32_t> values;
    long long value;
    while (in >> value) {
        if (value < INT32_MIN || value > INT32_MAX) throw std::runtime_error("demo value outside int32 range");
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
} // namespace drone::formats
