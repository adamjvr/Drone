#include <drone/formats/fly.hpp>

#include <fstream>
#include <limits>
#include <stdexcept>

namespace drone::formats {
std::vector<FlyRecord> load_fly(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("could not open FLY: " + path.string());
    long long count = 0;
    if (!(in >> count) || count < 0 || count > 1'000'000) {
        throw std::runtime_error("invalid FLY record count");
    }
    std::vector<FlyRecord> records;
    records.reserve(static_cast<std::size_t>(count));
    for (long long i = 0; i < count; ++i) {
        long long a, b, c;
        if (!(in >> a >> b >> c)) throw std::runtime_error("truncated FLY record data");
        if (a < std::numeric_limits<std::int16_t>::min() || a > std::numeric_limits<std::int16_t>::max() ||
            b < std::numeric_limits<std::int16_t>::min() || b > std::numeric_limits<std::int16_t>::max() ||
            c < std::numeric_limits<std::int8_t>::min() || c > std::numeric_limits<std::int8_t>::max()) {
            throw std::runtime_error("FLY field outside recovered storage range");
        }
        records.push_back({static_cast<std::int16_t>(a), static_cast<std::int16_t>(b), static_cast<std::int8_t>(c)});
    }
    long long extra;
    if (in >> extra) throw std::runtime_error("FLY contains extra numeric data after declared records");
    return records;
}
} // namespace drone::formats
