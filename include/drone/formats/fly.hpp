#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace drone::formats {

struct FlyRecord {
    std::int16_t field0{};
    std::int16_t field1{};
    std::int8_t field2{};
};

std::vector<FlyRecord> load_fly(const std::filesystem::path& path);

} // namespace drone::formats
