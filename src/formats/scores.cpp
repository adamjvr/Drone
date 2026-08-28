#include <drone/formats/scores.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <string_view>

namespace drone::formats {
namespace {

class Cursor {
public:
    explicit Cursor(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

    std::uint8_t get() {
        if (pos_ >= bytes_.size()) {
            throw ScoresFormatError("unexpected end of legacy scores file");
        }
        return bytes_[pos_++];
    }

    void skip(const std::size_t count) {
        if (count > bytes_.size() - pos_) {
            throw ScoresFormatError("legacy scores filler exceeds file size");
        }
        pos_ += count;
    }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t pos_ = 0;
};

int digit(const std::uint8_t c) {
    if (c < '0' || c > '9') {
        throw ScoresFormatError("expected decimal digit in legacy scores file");
    }
    return static_cast<int>(c - '0');
}

std::size_t read_three_digit_padding(Cursor& cursor) {
    return static_cast<std::size_t>(digit(cursor.get()) * 100 +
                                    digit(cursor.get()) * 10 +
                                    digit(cursor.get()));
}

void skip_three_noise_bytes(Cursor& cursor) {
    cursor.skip(3);
}

std::string decode_name(Cursor& cursor) {
    skip_three_noise_bytes(cursor);
    const auto padding = read_three_digit_padding(cursor);
    skip_three_noise_bytes(cursor);

    std::string result;
    result.reserve(high_score_interactive_name_max_chars);
    for (;;) {
        cursor.skip(padding);
        const auto c = cursor.get();
        if (c == 0) {
            break;
        }
        if (result.size() >= high_score_name_storage_bytes - 1) {
            throw ScoresFormatError("legacy high-score name exceeds 29-byte storage slot");
        }
        result.push_back(static_cast<char>(c));
    }
    return result;
}

std::int16_t decode_number(Cursor& cursor) {
    skip_three_noise_bytes(cursor);
    const auto padding = read_three_digit_padding(cursor);
    skip_three_noise_bytes(cursor);

    const int count = digit(cursor.get());
    if (count < 1 || count > 4) {
        throw ScoresFormatError("legacy score numeric field has unsupported digit count");
    }

    int value = 0;
    for (int i = 0; i < count; ++i) {
        cursor.skip(padding);
        value = value * 10 + digit(cursor.get());
    }
    if (value > std::numeric_limits<std::int16_t>::max()) {
        throw ScoresFormatError("legacy score numeric field exceeds signed 16-bit range");
    }
    return static_cast<std::int16_t>(value);
}

struct FillerRng {
    std::uint32_t state;

    std::uint32_t next() {
        // Deterministic local LCG. Compatibility relies only on skipped filler
        // structure, not on matching the original Microsoft CRT rand stream.
        state = state * 1664525u + 1013904223u;
        return state;
    }

    char digit_char() { return static_cast<char>('0' + (next() % 10u)); }
    char upper_char() { return static_cast<char>('A' + (next() % 26u)); }
    std::size_t padding() { return 300u + (next() % 400u); }
};

void append_decimal(std::vector<std::uint8_t>& out, const std::size_t value) {
    const auto text = std::to_string(value);
    out.insert(out.end(), text.begin(), text.end());
}

void append_noise_digits(std::vector<std::uint8_t>& out, FillerRng& rng, const std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        out.push_back(static_cast<std::uint8_t>(rng.digit_char()));
    }
}

void append_noise_letters(std::vector<std::uint8_t>& out, FillerRng& rng, const std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        out.push_back(static_cast<std::uint8_t>(rng.upper_char()));
    }
}

void encode_name(std::vector<std::uint8_t>& out, FillerRng& rng, std::string_view name) {
    if (name.size() >= high_score_name_storage_bytes) {
        throw ScoresFormatError("high-score name does not fit legacy 30-byte slot");
    }

    append_noise_digits(out, rng, 3);
    const auto padding = rng.padding();
    append_decimal(out, padding); // always 300..699: exactly three characters
    append_noise_digits(out, rng, 3);

    for (const char c : name) {
        append_noise_letters(out, rng, padding);
        out.push_back(static_cast<std::uint8_t>(c));
    }
    append_noise_letters(out, rng, padding);
    out.push_back(0);
}

void encode_number(std::vector<std::uint8_t>& out, FillerRng& rng, const std::int16_t value) {
    if (value < 0) {
        throw ScoresFormatError("legacy high-score writer only supports established non-negative fields");
    }

    const auto text = std::to_string(value);
    if (text.empty() || text.size() > 4) {
        throw ScoresFormatError("legacy high-score numeric field must contain 1..4 digits");
    }

    append_noise_digits(out, rng, 3);
    const auto padding = rng.padding();
    append_decimal(out, padding);
    append_noise_digits(out, rng, 3);
    append_decimal(out, text.size());

    for (const char c : text) {
        append_noise_digits(out, rng, padding);
        out.push_back(static_cast<std::uint8_t>(c));
    }
}

} // namespace

HighScoreTable decode_legacy_scores(const std::span<const std::uint8_t> bytes) {
    Cursor cursor(bytes);
    HighScoreTable table{};
    for (auto& entry : table) {
        entry.name = decode_name(cursor);
        entry.drones_disarmed = decode_number(cursor);
        entry.score = decode_number(cursor);
        entry.mothership_destroyed = decode_number(cursor);
        entry.percentage_hit = decode_number(cursor);
    }
    return table;
}

std::vector<std::uint8_t> encode_legacy_scores(const HighScoreTable& table,
                                               const std::uint32_t seed) {
    FillerRng rng{seed};
    std::vector<std::uint8_t> out;
    out.reserve(128 * 1024);

    for (const auto& entry : table) {
        encode_name(out, rng, entry.name);
        encode_number(out, rng, entry.drones_disarmed);
        encode_number(out, rng, entry.score);
        encode_number(out, rng, entry.mothership_destroyed);
        encode_number(out, rng, entry.percentage_hit);
    }
    return out;
}

} // namespace drone::formats
