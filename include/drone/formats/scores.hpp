#pragma once

#include <drone/high_score.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace drone::formats {

class ScoresFormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Decode the Win32 runtime-created `scores` file. The original format obscures
// each logical value with large random filler runs. Filler bytes are ignored;
// the meaningful table is ten records of name + four non-negative integers.
HighScoreTable decode_legacy_scores(std::span<const std::uint8_t> bytes);

// Produce a structurally compatible legacy score file using deterministic
// filler. This is intended for round-trip/compatibility tooling; it does not
// attempt to reproduce the original CRT rand() byte stream.
std::vector<std::uint8_t> encode_legacy_scores(const HighScoreTable& table,
                                               std::uint32_t seed = 0x44524F4Eu);

} // namespace drone::formats
