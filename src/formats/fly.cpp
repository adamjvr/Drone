#include <drone/formats/fly.hpp>

#include <array>
#include <cctype>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace drone::formats {
namespace {

constexpr std::array<KnownFlyAsset, 10> known_assets{{
    {"frisbee1.fly", 937, 937},
    {"frisbee2.fly", 426, 426},
    {"leftdive.fly", 119, 119},
    {"leftdrop.fly", 200, 200},
    {"loop.fly", 380, 380},
    {"newcurly.fly", 232, 232},
    {"rightdiv.fly", 119, 118}, // canonical file is one record shorter than loader request
    {"ritedrop.fly", 200, 200},
    {"swarm.fly", 950, 949},    // canonical file is one record shorter than loader request
    {"swoop.fly", 190, 190},
}};

std::string lowercase_filename(std::string_view value) {
    std::string out(value);
    for (auto& ch : out) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return out;
}

FlyRecord read_record(std::istream& in, std::size_t index) {
    long long x = 0;
    long long y = 0;
    long long aux = 0;
    if (!(in >> x >> y >> aux)) {
        throw std::runtime_error("truncated FLY record data at record " + std::to_string(index));
    }
    if (x < std::numeric_limits<std::int16_t>::min() || x > std::numeric_limits<std::int16_t>::max() ||
        y < std::numeric_limits<std::int16_t>::min() || y > std::numeric_limits<std::int16_t>::max() ||
        aux < std::numeric_limits<std::int8_t>::min() || aux > std::numeric_limits<std::int8_t>::max()) {
        throw std::runtime_error("FLY field outside recovered storage range at record " + std::to_string(index));
    }
    return {
        static_cast<std::int16_t>(x),
        static_cast<std::int16_t>(y),
        static_cast<std::int8_t>(aux),
    };
}

void reject_trailing_numeric_data(std::istream& in, std::string_view description) {
    long long extra = 0;
    if (in >> extra) {
        throw std::runtime_error(std::string(description) + " contains extra numeric data");
    }
}

} // namespace

std::vector<FlyRecord> load_counted_fly(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("could not open FLY: " + path.string());

    long long count = 0;
    if (!(in >> count) || count < 0 || count > 1'000'000) {
        throw std::runtime_error("invalid counted FLY record count");
    }

    std::vector<FlyRecord> records;
    records.reserve(static_cast<std::size_t>(count));
    for (long long i = 0; i < count; ++i) {
        records.push_back(read_record(in, static_cast<std::size_t>(i)));
    }
    reject_trailing_numeric_data(in, "counted FLY");
    return records;
}

std::vector<FlyRecord> load_raw_fly(
    const std::filesystem::path& path,
    const std::size_t expected_records,
    const bool strict) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("could not open FLY: " + path.string());

    std::vector<FlyRecord> records;
    records.reserve(expected_records);
    for (std::size_t i = 0; i < expected_records; ++i) {
        in >> std::ws;
        if (in.peek() == std::char_traits<char>::eof()) {
            if (!strict) break;
            throw std::runtime_error("raw FLY ended before loader-requested record " + std::to_string(i));
        }
        records.push_back(read_record(in, i));
    }

    if (strict) reject_trailing_numeric_data(in, "raw FLY");
    return records;
}

std::optional<KnownFlyAsset> known_fly_asset(const std::string_view filename) {
    const auto needle = lowercase_filename(filename);
    for (const auto& asset : known_assets) {
        if (needle == asset.filename) return asset;
    }
    return std::nullopt;
}

std::vector<FlyRecord> load_fly(const std::filesystem::path& path) {
    return load_counted_fly(path);
}

} // namespace drone::formats
