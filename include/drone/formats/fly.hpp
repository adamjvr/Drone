#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace drone::formats {

// Recovered on-disk/runtime storage for Drone trajectory data.
// x/y are established from Win32 consumers that copy the first two arrays
// directly into entity position fields. aux is recovered sprite-frame control
// for normal trajectory-owned entities (relative delta for <=1, absolute
// frame encoded as aux-2 for >1).
struct FlyRecord {
    std::int16_t x{};
    std::int16_t y{};
    std::int8_t aux{};
};

enum class FlyEncoding {
    counted_current,
    raw_trajectory,
};

struct KnownFlyAsset {
    std::string_view filename;
    std::size_t loader_record_count;
    std::size_t physical_record_count;
};

// CURRENT.FLY is a special working/composite file: N then N triples.
std::vector<FlyRecord> load_counted_fly(const std::filesystem::path& path);

// Runtime trajectory assets contain only triples; the original executable has
// a per-asset hard-coded read count rather than a count stored in the file.
// If the file is shorter than expected, strict=true reports the original-data
// mismatch instead of manufacturing an uninitialized final record.
std::vector<FlyRecord> load_raw_fly(
    const std::filesystem::path& path,
    std::size_t expected_records,
    bool strict = true);

// Convenience for known canonical trajectory filenames. Returns nullopt for a
// filename whose original loader count has not yet been established.
std::optional<KnownFlyAsset> known_fly_asset(std::string_view filename);

// Backward-compatible entry point: only accepts the counted CURRENT.FLY form.
// Callers handling gameplay trajectory files must choose an explicit loader
// count (or use known_fly_asset) so we do not guess semantics from file length.
std::vector<FlyRecord> load_fly(const std::filesystem::path& path);

} // namespace drone::formats
