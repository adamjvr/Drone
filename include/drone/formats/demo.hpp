#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace drone::formats {

using DemoRecord = std::array<std::int32_t, 14>;
std::vector<DemoRecord> load_demo_dat(const std::filesystem::path& path);

} // namespace drone::formats
