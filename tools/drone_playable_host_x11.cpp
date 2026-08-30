#include <drone/audio/audio_event.hpp>
#include <drone/audio/portable_backend.hpp>
#include <drone/audio/presentation_audio.hpp>
#include <drone/audio/sample_mixer.hpp>
#include <drone/fidelity/font2.hpp>
#include <drone/fidelity/hud_presentation.hpp>
#include <drone/fidelity/indexed_framebuffer.hpp>
#include <drone/fidelity/scaled_overlay_presentation.hpp>
#include <drone/fidelity/sprite_blit.hpp>
#include <drone/fidelity/sprite_sheet.hpp>
#include <drone/fidelity/world_viewport.hpp>
#include <drone/formats/clv.hpp>
#include <drone/formats/demo.hpp>
#include <drone/formats/fly.hpp>
#include <drone/formats/jba.hpp>
#include <drone/gameplay/game_session.hpp>
#include <drone/gameplay/demo_replay.hpp>
#include <drone/gameplay/debris_effects.hpp>
#include <drone/gameplay/gemini_boss.hpp>
#include <drone/gameplay/lid_top_boss.hpp>
#include <drone/gameplay/rapid_missile.hpp>
#include <drone/gameplay/shield.hpp>
#include <drone/gameplay/trajectory_templates.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <png.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <memory>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace drone;

namespace {

constexpr int kLogicalW = 320;
constexpr int kLogicalH = 200;
constexpr int kMinScale = 1;
constexpr int kMaxScale = 8;
constexpr double kTickHz = 70.0863;
constexpr auto kTickDuration = std::chrono::duration<double>(1.0 / kTickHz);
constexpr std::uint32_t kStartupWeaponHelpTicks = static_cast<std::uint32_t>(kTickHz * 5.0);

struct SpriteBank {
    fidelity::IndexedSpriteFrame frame0{};
    std::vector<fidelity::IndexedSpriteFrame> frames{};
};

std::string upper_ascii(std::string value) {
    for (char& c : value) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}

fidelity::IndexedSpriteFrame extract_frame(
    const formats::JbaImage& sheet,
    std::size_t width,
    std::size_t height,
    std::size_t index) {
    const auto cols = std::max<std::size_t>(1, formats::JbaImage::width / (width + 1));
    return fidelity::extract_guttered_jba_frame(sheet, width, height, index % cols, index / cols);
}

SpriteBank extract_bank(const formats::JbaImage& sheet,
                        std::size_t width,
                        std::size_t height,
                        std::size_t count) {
    SpriteBank bank;
    bank.frames.reserve(count);
    for (std::size_t i = 0; i < count; ++i) bank.frames.push_back(extract_frame(sheet, width, height, i));
    if (!bank.frames.empty()) bank.frame0 = bank.frames.front();
    return bank;
}

SpriteBank extract_bank_rows(const formats::JbaImage& sheet,
                             std::size_t width,
                             std::size_t height,
                             std::size_t count,
                             const std::array<std::uint8_t, 4>& row_counts) {
    // The original trajectory-sheet loaders do not infer a maximum number of
    // columns from 320/(width+1). They issue explicit (cell_x, cell_y) calls.
    // Several sheets intentionally use only the left portion of each row, so
    // treating every geometrically possible cell as a frame inserts blank
    // animation slots and makes live enemies visually disappear.
    SpriteBank bank;
    bank.frames.reserve(count);
    std::size_t frame_slot = 0;
    for (std::size_t row = 0; row < row_counts.size() && frame_slot < count; ++row) {
        for (std::size_t col = 0; col < row_counts[row] && frame_slot < count; ++col) {
            bank.frames.push_back(fidelity::extract_guttered_jba_frame(sheet, width, height, col, row));
            ++frame_slot;
        }
    }
    if (bank.frames.size() != count) {
        throw std::runtime_error("trajectory sprite layout did not provide requested frame count");
    }
    if (!bank.frames.empty()) bank.frame0 = bank.frames.front();
    return bank;
}

struct AssetStore {
    fs::path root;
    std::unordered_map<std::string, formats::JbaImage> jba_cache;

    explicit AssetStore(fs::path path) : root(std::move(path)) {}

    fs::path resolve(std::string_view requested) const {
        const auto name = upper_ascii(std::string(requested));
        const auto direct = root / name;
        if (fs::exists(direct)) return direct;

        // The DOS/shareware corpus does not carry the registered Windows red
        // Stinger projectile sheet.  During evidence-driven development, use
        // the separately preserved Windows sight asset without mutating either
        // canonical reference tree.  Self-contained builds place REDPROBE.JBA
        // directly in their runtime asset directory, so the direct path above
        // remains authoritative.
        if (name == "REDPROBE.JBA") {
            const auto supplemental = root.parent_path() / "windows" / "Sights" / "Redprobe.jba";
            if (fs::exists(supplemental)) return supplemental;
        }

        return direct;
    }

    const formats::JbaImage& jba(std::string name) {
        name = upper_ascii(std::move(name));
        auto it = jba_cache.find(name);
        if (it != jba_cache.end()) return it->second;
        auto [inserted, ok] = jba_cache.emplace(name, formats::load_jba_320x200(resolve(name)));
        (void)ok;
        return inserted->second;
    }

    bool exists(std::string_view name) const {
        return fs::exists(resolve(name));
    }
};

struct RgbaImage {
    int width{};
    int height{};
    std::vector<std::uint8_t> pixels{}; // RGBA8

    bool valid() const noexcept {
        return width > 0 && height > 0 &&
               pixels.size() == static_cast<std::size_t>(width) * height * 4u;
    }
};

RgbaImage load_png_rgba(const fs::path& path) {
    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&image, path.c_str())) {
        throw std::runtime_error("PNG read failed: " + path.string());
    }
    image.format = PNG_FORMAT_RGBA;
    RgbaImage out;
    out.width = static_cast<int>(image.width);
    out.height = static_cast<int>(image.height);
    out.pixels.resize(PNG_IMAGE_SIZE(image));
    if (!png_image_finish_read(&image, nullptr, out.pixels.data(), 0, nullptr)) {
        const std::string reason = image.message[0] ? image.message : "unknown libpng error";
        png_image_free(&image);
        throw std::runtime_error("PNG decode failed: " + path.string() + ": " + reason);
    }
    png_image_free(&image);
    return out;
}

RgbaImage resize_rgba_bilinear(const RgbaImage& src, int dst_w, int dst_h) {
    if (!src.valid() || dst_w <= 0 || dst_h <= 0) return {};
    if (src.width == dst_w && src.height == dst_h) return src;

    RgbaImage out;
    out.width = dst_w;
    out.height = dst_h;
    out.pixels.resize(static_cast<std::size_t>(dst_w) * dst_h * 4u);

    const double sx = static_cast<double>(src.width) / dst_w;
    const double sy = static_cast<double>(src.height) / dst_h;
    for (int y = 0; y < dst_h; ++y) {
        const double fy = std::max(0.0, (y + 0.5) * sy - 0.5);
        const int y0 = std::clamp(static_cast<int>(std::floor(fy)), 0, src.height - 1);
        const int y1 = std::min(y0 + 1, src.height - 1);
        const double wy = fy - std::floor(fy);
        for (int x = 0; x < dst_w; ++x) {
            const double fx = std::max(0.0, (x + 0.5) * sx - 0.5);
            const int x0 = std::clamp(static_cast<int>(std::floor(fx)), 0, src.width - 1);
            const int x1 = std::min(x0 + 1, src.width - 1);
            const double wx = fx - std::floor(fx);
            const std::size_t i00 = (static_cast<std::size_t>(y0) * src.width + x0) * 4u;
            const std::size_t i10 = (static_cast<std::size_t>(y0) * src.width + x1) * 4u;
            const std::size_t i01 = (static_cast<std::size_t>(y1) * src.width + x0) * 4u;
            const std::size_t i11 = (static_cast<std::size_t>(y1) * src.width + x1) * 4u;
            const std::size_t od = (static_cast<std::size_t>(y) * dst_w + x) * 4u;
            for (int c = 0; c < 4; ++c) {
                const double a = src.pixels[i00 + c] * (1.0 - wx) + src.pixels[i10 + c] * wx;
                const double b = src.pixels[i01 + c] * (1.0 - wx) + src.pixels[i11 + c] * wx;
                out.pixels[od + c] = static_cast<std::uint8_t>(
                    std::clamp(std::lround(a * (1.0 - wy) + b * wy), 0l, 255l));
            }
        }
    }
    return out;
}

enum class HdFilterMode : std::uint8_t {
    Smooth,
    Sharp,
};

std::string_view hd_filter_name(HdFilterMode mode) noexcept {
    return mode == HdFilterMode::Sharp ? "SHARP" : "SMOOTH";
}

RgbaImage resize_rgba_nearest(const RgbaImage& src, int dst_w, int dst_h) {
    if (!src.valid() || dst_w <= 0 || dst_h <= 0) return {};
    if (src.width == dst_w && src.height == dst_h) return src;
    RgbaImage out;
    out.width = dst_w;
    out.height = dst_h;
    out.pixels.resize(static_cast<std::size_t>(dst_w) * dst_h * 4u);
    for (int y = 0; y < dst_h; ++y) {
        const int sy = std::clamp((y * src.height) / dst_h, 0, src.height - 1);
        for (int x = 0; x < dst_w; ++x) {
            const int sx = std::clamp((x * src.width) / dst_w, 0, src.width - 1);
            const auto si = (static_cast<std::size_t>(sy) * src.width + sx) * 4u;
            const auto di = (static_cast<std::size_t>(y) * dst_w + x) * 4u;
            std::copy_n(src.pixels.begin() + static_cast<std::ptrdiff_t>(si), 4,
                        out.pixels.begin() + static_cast<std::ptrdiff_t>(di));
        }
    }
    return out;
}

std::string strip_extension_upper(std::string value) {
    value = upper_ascii(std::move(value));
    const auto dot = value.find_last_of('.');
    if (dot != std::string::npos) value.resize(dot);
    return value;
}

struct HdAssetStore {
    fs::path root;
    bool available{false};
    std::size_t png_file_count{0};
    int cache_scale{0};
    HdFilterMode filter{HdFilterMode::Smooth};
    std::unordered_map<std::string, fs::path> windows_fullscreen_index{};
    std::unordered_map<std::string, fs::path> dos_fullscreen_index{};
    std::unordered_map<std::string, std::shared_ptr<RgbaImage>> resized_cache{};

    static void index_decoded_tree(const fs::path& tree,
                                   std::unordered_map<std::string, fs::path>& out) {
        if (!fs::is_directory(tree)) return;
        for (const auto& entry : fs::recursive_directory_iterator(tree)) {
            if (!entry.is_regular_file()) continue;
            if (upper_ascii(entry.path().extension().string()) != ".PNG") continue;
            const auto stem = strip_extension_upper(entry.path().filename().string());
            // First occurrence wins. The upscale corpus keeps canonical JBA
            // basenames unique within each platform tree.
            out.emplace(stem, entry.path());
        }
    }

    explicit HdAssetStore(fs::path path) : root(std::move(path)) {
        const bool trees_present = fs::is_directory(root / "decoded") &&
                                   fs::is_directory(root / "sprite_frames");
        if (!trees_present) return;

        index_decoded_tree(root / "decoded" / "windows", windows_fullscreen_index);
        index_decoded_tree(root / "decoded" / "dos", dos_fullscreen_index);
        for (const auto& tree : {root / "decoded", root / "sprite_frames"}) {
            if (!fs::is_directory(tree)) continue;
            for (const auto& entry : fs::recursive_directory_iterator(tree)) {
                if (entry.is_regular_file() && upper_ascii(entry.path().extension().string()) == ".PNG")
                    ++png_file_count;
            }
        }

        // A directory existing is not enough. Require the anchor assets that
        // prove the generated 12x corpus is actually usable by this presenter.
        available = !resolve_fullscreen("TITLESH.JBA").empty() &&
                    !resolve_fullscreen("RIVERTOP.JBA").empty() &&
                    !resolve_sprite("recovered", "SHIP", 0).empty();
    }

    std::string diagnostic_summary() const {
        return "root=" + root.string() +
               " pngs=" + std::to_string(png_file_count) +
               " win_sheets=" + std::to_string(windows_fullscreen_index.size()) +
               " dos_sheets=" + std::to_string(dos_fullscreen_index.size());
    }

    bool self_test(std::ostream& out) {
        if (!available) {
            out << "HD_SELFTEST FAIL " << diagnostic_summary() << " missing anchor assets\n";
            return false;
        }
        try {
            const auto title_path = resolve_fullscreen("TITLESH.JBA");
            const auto river_path = resolve_fullscreen("RIVERTOP.JBA");
            const auto ship_path = resolve_sprite("recovered", "SHIP", 0);
            const auto title = load_png_rgba(title_path);
            const auto river = load_png_rgba(river_path);
            const auto ship = load_png_rgba(ship_path);
            if (!title.valid() || !river.valid() || !ship.valid()) {
                out << "HD_SELFTEST FAIL " << diagnostic_summary() << " decoded invalid image\n";
                return false;
            }
            out << "HD_SELFTEST OK " << diagnostic_summary()
                << " TITLESH=" << title.width << "x" << title.height
                << " RIVERTOP=" << river.width << "x" << river.height
                << " SHIP_00=" << ship.width << "x" << ship.height << "\n";
            return true;
        } catch (const std::exception& e) {
            out << "HD_SELFTEST FAIL " << diagnostic_summary() << " " << e.what() << "\n";
            return false;
        }
    }

    void set_scale(int scale) {
        if (cache_scale == scale) return;
        cache_scale = scale;
        resized_cache.clear();
    }

    void set_filter(HdFilterMode next) {
        if (filter == next) return;
        filter = next;
        resized_cache.clear();
    }

    fs::path resolve_fullscreen(std::string_view jba_name) const {
        const auto stem = strip_extension_upper(std::string(jba_name));
        // The playable host reconstructs the Win32 release, so prefer the
        // Windows upscale when the DOS and Windows visual corpora differ.
        if (const auto it = windows_fullscreen_index.find(stem); it != windows_fullscreen_index.end())
            return it->second;
        if (const auto it = dos_fullscreen_index.find(stem); it != dos_fullscreen_index.end())
            return it->second;
        return {};
    }

    fs::path resolve_sprite(std::string_view category,
                            std::string_view family,
                            std::size_t frame) const {
        const auto stem = strip_extension_upper(std::string(family));
        char suffix[32]{};
        std::snprintf(suffix, sizeof(suffix), "_%02zu.png", frame);
        const auto path = root / "sprite_frames" / std::string(category) /
                          stem / (stem + suffix);
        if (fs::exists(path)) return path;
        return {};
    }

    bool has_sprite(std::string_view category,
                    std::string_view family,
                    std::size_t frame) const {
        return !resolve_sprite(category, family, frame).empty();
    }

    const RgbaImage* resized_path(const fs::path& path, int width, int height) {
        if (path.empty() || width <= 0 || height <= 0) return nullptr;
        const std::string key = path.string() + "#" + std::to_string(width) + "x" + std::to_string(height);
        if (auto it = resized_cache.find(key); it != resized_cache.end()) return it->second.get();
        try {
            auto decoded = load_png_rgba(path);
            auto image = std::make_shared<RgbaImage>(
                filter == HdFilterMode::Sharp
                    ? resize_rgba_nearest(decoded, width, height)
                    : resize_rgba_bilinear(decoded, width, height));
            auto [it, inserted] = resized_cache.emplace(key, std::move(image));
            (void)inserted;
            return it->second.get();
        } catch (const std::exception& e) {
            std::cerr << "HD art warning: " << e.what() << '\n';
            return nullptr;
        }
    }

    const RgbaImage* fullscreen(std::string_view jba_name, int scale) {
        return resized_path(resolve_fullscreen(jba_name), kLogicalW * scale, kLogicalH * scale);
    }

    const RgbaImage* sprite(std::string_view category,
                            std::string_view family,
                            std::size_t frame,
                            int width,
                            int height) {
        return resized_path(resolve_sprite(category, family, frame), width, height);
    }
};

enum class HdBackgroundKind : std::uint8_t {
    Disabled,
    Fullscreen,
    World
};

struct HdSpriteDraw {
    std::string category;
    std::string family;
    std::size_t frame{};
    int x{};
    int y{};
    int logical_width{};
    int logical_height{};
};

struct HdFramePlan {
    HdBackgroundKind background{HdBackgroundKind::Disabled};
    std::string fullscreen_asset{};
    std::array<std::string, 3> world_assets{};
    int world_scroll_row{};
    std::array<std::uint8_t, fidelity::logical_viewport_bytes> base_pixels{};
    bool base_valid{false};
    bool dim_background{false};
    std::vector<HdSpriteDraw> sprites{};

    void reset() {
        background = HdBackgroundKind::Disabled;
        fullscreen_asset.clear();
        world_assets = {};
        world_scroll_row = 0;
        base_valid = false;
        dim_background = false;
        sprites.clear();
    }

    void capture_base(const fidelity::IndexedFramebuffer& fb) {
        std::copy_n(fb.pixels().begin(), base_pixels.size(), base_pixels.begin());
        base_valid = true;
    }

    void begin_fullscreen(std::string asset, const fidelity::IndexedFramebuffer& fb) {
        background = HdBackgroundKind::Fullscreen;
        fullscreen_asset = std::move(asset);
        capture_base(fb);
    }

    void begin_world(const std::array<std::string, 3>& pages,
                     int scroll_row,
                     const fidelity::IndexedFramebuffer& fb) {
        background = HdBackgroundKind::World;
        world_assets = pages;
        world_scroll_row = scroll_row;
        capture_base(fb);
    }

    void add_sprite(std::string category,
                    std::string family,
                    std::size_t frame,
                    int x,
                    int y,
                    int logical_width,
                    int logical_height) {
        if (logical_width <= 0 || logical_height <= 0) return;
        sprites.push_back({std::move(category), std::move(family), frame,
                           x, y, logical_width, logical_height});
    }
};

void blit_asset_sprite(fidelity::IndexedFramebuffer& fb,
                       const fidelity::IndexedSpriteFrame& frame,
                       int x,
                       int y,
                       HdFramePlan* hd,
                       std::string_view category,
                       std::string_view family,
                       std::size_t frame_index,
                       std::optional<std::pair<int, int>> destination_size = std::nullopt) {
    fidelity::blit_transparent_original(fb, frame, x, y);
    if (!hd) return;
    const int w = destination_size ? destination_size->first : static_cast<int>(frame.width);
    const int h = destination_size ? destination_size->second : static_cast<int>(frame.height);
    hd->add_sprite(std::string(category), strip_extension_upper(std::string(family)),
                   frame_index, x, y, w, h);
}

struct WorldImage {
    std::array<formats::Rgb8, 256> palette{};
    std::vector<std::uint8_t> pixels;
    std::array<std::string, 3> source_pages{};
};

WorldImage load_world_stack(AssetStore& assets, const std::array<const char*, 3>& names) {
    WorldImage world;
    world.pixels.resize(fidelity::scenery_world_bytes);
    for (std::size_t page = 0; page < names.size(); ++page) {
        const auto& image = assets.jba(names[page]);
        world.source_pages[page] = upper_ascii(names[page]);
        if (page == 0) world.palette = image.palette;
        std::copy(image.pixels.begin(), image.pixels.end(),
                  world.pixels.begin() + static_cast<std::ptrdiff_t>(page * 320 * 200));
    }
    return world;
}

WorldImage load_initial_river_world(AssetStore& assets) {
    return load_world_stack(assets, {"RIVERTOP.JBA", "RIVERMID.JBA", "RIVERBOT.JBA"});
}

WorldImage load_desert_world(AssetStore& assets) {
    return load_world_stack(assets, {"DESERTOP.JBA", "DESERMID.JBA", "DESERBOT.JBA"});
}

void apply_scenery_transition(WorldImage& world,
                              AssetStore& assets,
                              const gameplay::SceneryTransitionPlan scenery) {
    switch (scenery) {
    case gameplay::SceneryTransitionPlan::DesertStack:
        world = load_desert_world(assets);
        break;
    case gameplay::SceneryTransitionPlan::RiverStack:
        world = load_initial_river_world(assets);
        break;
    case gameplay::SceneryTransitionPlan::SharewareTerminationDesertBottomOnly: {
        // The canonical shareware stop loads only deserbot.jba before entering
        // Results.  Preserve the visible terminal page without manufacturing a
        // nonexistent third-level stack.
        const auto& image = assets.jba("DESERBOT.JBA");
        world.palette = image.palette;
        world.source_pages = {"DESERBOT.JBA", "DESERBOT.JBA", "DESERBOT.JBA"};
        for (int page = 0; page < 3; ++page) {
            std::copy(image.pixels.begin(), image.pixels.end(),
                      world.pixels.begin() + static_cast<std::ptrdiff_t>(page * 320 * 200));
        }
        break;
    }
    default:
        // Registered-only scenery is not present in the supplied shareware data.
        break;
    }
}

void render_fullscreen_image(fidelity::IndexedFramebuffer& fb, const formats::JbaImage& image) {
    fb.palette() = image.palette;
    std::copy_n(image.pixels.begin(), fidelity::logical_viewport_bytes, fb.pixels().begin());
}

void render_world(fidelity::IndexedFramebuffer& fb, const WorldImage& world, int scroll_row) {
    fb.palette() = world.palette;
    if (!fidelity::compose_scrolling_world_viewport(world.pixels, fb.pixels(), scroll_row)) {
        throw std::runtime_error("invalid scrolling-world viewport state");
    }
}

std::vector<formats::FlyRecord> make_generated_path(bool left_to_right) {
    // Exact startup loops recovered at Win32 0x004087EB..0x0040897C.
    std::vector<formats::FlyRecord> out;
    out.reserve(left_to_right ? 410 : 430);
    std::int32_t x_fixed = left_to_right ? -5 * 65536 : 322 * 65536;
    std::int32_t y_fixed = 50 << 16;
    std::int32_t y_velocity = 0;
    std::int32_t y_accel = 0x0c80;
    for (;;) {
        out.push_back({static_cast<std::int16_t>(x_fixed >> 16),
                       static_cast<std::int16_t>(y_fixed >> 16),
                       static_cast<std::int8_t>(-1)});
        x_fixed += left_to_right ? 0xce20 : -0xce20;
        y_fixed += y_velocity;
        if (y_velocity > 0x20000 || y_velocity < -0x20000) y_accel = -y_accel;
        y_velocity += y_accel;
        const int x = x_fixed >> 16;
        if (left_to_right ? (x >= 321) : (x <= -20)) break;
    }
    return out;
}

struct PathStorage {
    std::array<std::vector<formats::FlyRecord>, gameplay::canonical_trajectory_path_family_count> data{};
    gameplay::TrajectoryPathCatalogView view{};
};

PathStorage load_paths(const fs::path& root) {
    using gameplay::TrajectoryPathFamily;
    PathStorage p;
    const auto load_known = [&](TrajectoryPathFamily family, const char* name) {
        const auto known = formats::known_fly_asset(name);
        if (!known) throw std::runtime_error(std::string("missing FLY metadata: ") + name);
        auto& dst = p.data[static_cast<std::size_t>(family)];
        dst = formats::load_raw_fly(root / upper_ascii(name), known->loader_record_count, false);
    };
    load_known(TrajectoryPathFamily::Loop, "loop.fly");
    load_known(TrajectoryPathFamily::LeftDive, "leftdive.fly");
    load_known(TrajectoryPathFamily::Swarm, "swarm.fly");
    load_known(TrajectoryPathFamily::Swoop, "swoop.fly");
    load_known(TrajectoryPathFamily::NewCurly, "newcurly.fly");
    load_known(TrajectoryPathFamily::Frisbee1, "frisbee1.fly");
    load_known(TrajectoryPathFamily::Frisbee2, "frisbee2.fly");
    load_known(TrajectoryPathFamily::LeftDrop, "leftdrop.fly");
    load_known(TrajectoryPathFamily::RightDive, "rightdiv.fly");
    load_known(TrajectoryPathFamily::RightDrop, "ritedrop.fly");
    p.data[static_cast<std::size_t>(TrajectoryPathFamily::Generated402)] = make_generated_path(true);
    p.data[static_cast<std::size_t>(TrajectoryPathFamily::Generated422)] = make_generated_path(false);
    for (std::size_t i = 0; i < p.data.size(); ++i) p.view.families[i] = p.data[i];
    return p;
}

constexpr std::array<const char*, gameplay::canonical_trajectory_group_count> kGroupSheets{{
    "BLADE.JBA", "ARROW.JBA", "BLADE.JBA", "BAT.JBA", "HYDRA.JBA",
    "BLADE.JBA", "SADDLE.JBA", "FRISBEE.JBA", "FRISBEE.JBA", "HYDRA.JBA",
    "BAT.JBA", "ARROW.JBA", "SLOOP.JBA", "SLOOP.JBA", "FLAKE.JBA",
    "FLAKE.JBA", "SKATE.JBA"
}};

// Exact cell-row widths recovered from the Win32 loader around
// 0x0040A8CD..0x0040B13F.  These are extraction-call counts, not an inferred
// packing rule.  Hydra is deliberately asymmetric: 5 + 5 + 6 frames.
constexpr std::array<std::array<std::uint8_t, 4>, gameplay::canonical_trajectory_group_count>
    kGroupSheetRowCounts{{
        {{4, 4, 4, 3}}, // Blade
        {{4, 4, 4, 4}}, // Arrow
        {{4, 4, 4, 3}}, // Blade
        {{8, 8, 0, 0}}, // Bat
        {{5, 5, 6, 0}}, // Hydra
        {{4, 4, 4, 3}}, // Blade
        {{8, 8, 8, 8}}, // Saddle
        {{8, 8, 8, 8}}, // Frisbee
        {{8, 8, 8, 8}}, // Frisbee
        {{5, 5, 6, 0}}, // Hydra
        {{8, 8, 0, 0}}, // Bat
        {{4, 4, 4, 4}}, // Arrow
        {{8, 8, 8, 8}}, // Sloop
        {{8, 8, 8, 8}}, // Sloop
        {{8, 8, 0, 0}}, // Flake
        {{8, 8, 0, 0}}, // Flake
        {{8, 8, 8, 8}}, // Skate
    }};

struct TrajectorySprites {
    std::array<SpriteBank, gameplay::canonical_trajectory_group_count> banks{};
    gameplay::TrajectorySpriteMaskCatalogView masks{};
};

TrajectorySprites load_trajectory_sprites(AssetStore& assets) {
    TrajectorySprites result;
    const auto& templates = gameplay::canonical_trajectory_group_templates();
    for (std::size_t g = 0; g < templates.size(); ++g) {
        const auto& t = templates[g];
        result.banks[g] = extract_bank_rows(
            assets.jba(kGroupSheets[g]), t.sprite_width, t.sprite_height, t.frame_count, kGroupSheetRowCounts[g]);
        for (std::size_t f = 0; f < result.banks[g].frames.size(); ++f) {
            const auto& pixels = result.banks[g].frames[f].pixels;
            if (std::none_of(pixels.begin(), pixels.end(), [](std::uint8_t p) { return p != 0; })) {
                throw std::runtime_error("trajectory sprite bank contains an unexpected blank frame");
            }
        }
        for (std::size_t f = 0; f < result.banks[g].frames.size() && f < gameplay::canonical_trajectory_collision_frame_slots; ++f) {
            result.masks.frames[g][f] = result.banks[g].frames[f].pixels;
        }
    }
    return result;
}

struct BossSprites {
    SpriteBank lid;
    SpriteBank top;
    std::array<fidelity::IndexedSpriteFrame, gameplay::gemini_body_frame_count> gemini_body{};
    fidelity::IndexedSpriteFrame gemini_head{};
    gameplay::LidTopBossSpriteMaskView lid_mask{};
    gameplay::GeminiBossSpriteMaskView gemini_masks{};
};

BossSprites load_boss_sprites(AssetStore& assets) {
    BossSprites b;
    b.lid = extract_bank(assets.jba("LID.JBA"), 36, 40, 9);
    b.top = extract_bank(assets.jba("TOP.JBA"), 68, 56, 1);
    const auto g1 = extract_bank(assets.jba("GEMINI1.JBA"), 56, 41, 15);
    const auto g2 = extract_bank(assets.jba("GEMINI2.JBA"), 56, 41, 15);
    for (std::size_t i = 0; i < 15; ++i) {
        b.gemini_body[i] = g1.frames[i];
        b.gemini_body[i + 15] = g2.frames[i];
    }
    b.gemini_head = extract_frame(assets.jba("GEMHEAD.JBA"), 43, 34, 0);
    b.lid_mask.top_frame = b.top.frames[0].pixels;
    for (std::size_t i = 0; i < b.gemini_body.size(); ++i) b.gemini_masks.body_frames[i] = b.gemini_body[i].pixels;
    b.gemini_masks.head_frame = b.gemini_head.pixels;
    return b;
}

struct FontCache {
    std::array<fidelity::IndexedSpriteFrame, fidelity::font2_glyph_count> glyphs{};
};

FontCache load_font(AssetStore& assets) {
    FontCache f;
    const auto& sheet = assets.jba("FONT2.JBA");
    for (std::size_t i = 0; i < f.glyphs.size(); ++i) f.glyphs[i] = fidelity::extract_font2_glyph(sheet, i);
    return f;
}

void draw_text(fidelity::IndexedFramebuffer& fb, const FontCache& font,
               int x, int y, std::string_view text, std::uint8_t color) {
    for (unsigned char c : text) {
        const auto idx = fidelity::font2_glyph_index(c);
        if (idx) {
            const auto& glyph = font.glyphs[*idx];
            for (std::size_t gy = 0; gy < glyph.height; ++gy) {
                for (std::size_t gx = 0; gx < glyph.width; ++gx) {
                    if (glyph.pixels[gy * glyph.width + gx] == 0) continue;
                    const int dx = x + static_cast<int>(gx);
                    const int dy = y + static_cast<int>(gy);
                    if (dx >= 0 && dx < 320 && dy >= 0 && dy < 200) fb.pixels()[dy * 320 + dx] = color;
                }
            }
        }
        x += static_cast<int>(fidelity::font2_advance_x);
    }
}

void draw_hline(fidelity::IndexedFramebuffer& fb, int x, int y, int width, std::uint8_t color) {
    if (y < 0 || y >= 200 || width <= 0) return;
    const int x0 = std::max(0, x);
    const int x1 = std::min(320, x + width);
    if (x1 <= x0) return;
    std::fill(fb.pixels().begin() + y * 320 + x0, fb.pixels().begin() + y * 320 + x1, color);
}

void fill_rect(fidelity::IndexedFramebuffer& fb, int x, int y, int width, int height, std::uint8_t color) {
    if (width <= 0 || height <= 0) return;
    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(320, x + width);
    const int y1 = std::min(200, y + height);
    for (int row = y0; row < y1; ++row) {
        std::fill(fb.pixels().begin() + row * 320 + x0,
                  fb.pixels().begin() + row * 320 + x1, color);
    }
}


void blit_scaled_transparent(fidelity::IndexedFramebuffer& fb,
                             const fidelity::IndexedSpriteFrame& src,
                             const fidelity::ScaledOverlayDestination& dst) {
    const int dw = dst.right - dst.left;
    const int dh = dst.bottom - dst.top;
    if (dw <= 0 || dh <= 0 || src.width == 0 || src.height == 0) return;
    for (int y = std::max(0, dst.top); y < std::min(200, dst.bottom); ++y) {
        const auto sy = std::min<std::size_t>(src.height - 1,
            static_cast<std::size_t>((static_cast<std::int64_t>(y - dst.top) * src.height) / dh));
        for (int x = std::max(0, dst.left); x < std::min(320, dst.right); ++x) {
            const auto sx = std::min<std::size_t>(src.width - 1,
                static_cast<std::size_t>((static_cast<std::int64_t>(x - dst.left) * src.width) / dw));
            const auto p = src.pixels[sy * src.width + sx];
            if (p != 0) fb.pixels()[y * 320 + x] = p;
        }
    }
}

struct Explosion {
    int x{};
    int y{};
    int frame{};
    int frame_count{27};
    int age{};
    bool active{true};
};

struct ObjectiveDebris {
    fidelity::ScaledOverlayGeometry geom{};
    int vx{};
    int vy{};
    int frame{};
    int frame_count{};
    std::size_t bank{};
    bool active{};
};

struct EffectRuntime {
    std::vector<Explosion> explosions;
    std::array<ObjectiveDebris, fidelity::objective_scaled_debris_sprite_count> objective{};
    std::array<gameplay::DebrisSpriteState, gameplay::canonical_debris_sprite_pool_size> junk1{};
    std::array<gameplay::DebrisSpriteState, gameplay::canonical_debris_sprite_pool_size> junk2{};
    std::array<gameplay::DebrisSpriteState, gameplay::canonical_debris_sprite_pool_size> wheel{};
    bool objective_active{};
    std::uint32_t presentation_rng = 0x44524f4eU; // deterministic host-only stream

    std::uint32_t next_rng() noexcept {
        presentation_rng = presentation_rng * 1664525U + 1013904223U;
        return presentation_rng;
    }

    template <std::size_t N>
    void spawn_debris_in(std::array<gameplay::DebrisSpriteState, N>& pool,
                         int center_x,
                         int center_y,
                         int width,
                         int height,
                         std::uint8_t frames) {
        for (auto& d : pool) {
            if (d.active) continue;
            const auto r0 = next_rng();
            const auto r1 = next_rng();
            d.x = center_x - width / 2;
            d.y = center_y - height / 2;
            d.velocity_x = static_cast<int>((r0 >> 8) % 9U) - 4;
            d.velocity_y = -static_cast<int>((r1 >> 8) % 6U) - 1;
            d.sprite_width = static_cast<std::int16_t>(width);
            d.sprite_height = static_cast<std::int16_t>(height);
            d.current_frame = static_cast<std::uint8_t>(r0 % frames);
            d.frame_count = frames;
            d.frame_step = (r1 & 1U) ? 1 : -1;
            d.active = true;
            return;
        }
    }

    void spawn_debris_triplet(int center_x, int center_y) {
        spawn_debris_in(junk1, center_x, center_y, 7, 7, 16);
        spawn_debris_in(junk2, center_x, center_y, 12, 9, 16);
        spawn_debris_in(wheel, center_x, center_y, 5, 5, 14);
    }

    void spawn_explosion(int center_x, int center_y, bool with_debris = false) {
        if (explosions.size() > 96) explosions.erase(explosions.begin(), explosions.begin() + 32);
        explosions.push_back({center_x - 21, center_y - 19, 0, 27, 0, true});
        if (with_debris) spawn_debris_triplet(center_x, center_y);
    }
};

void render_player_shield_effect(fidelity::IndexedFramebuffer& fb,
                                 const gameplay::PlayerMotionState& player,
                                 EffectRuntime& effects) {
    // Win32 0x0041E6D0. The original first perturbs the existing pixels inside
    // the 22x22 player sprite (3-pixel inset), then builds a noisy symmetric
    // cloud around the sprite center. The executable consumes two fixed-point
    // quarter-circle lookup tables; 18 samples at 5-degree spacing reproduce
    // that recovered geometry while keeping this host-side renderer independent
    // of the still-unmapped runtime table initialization.
    constexpr int sprite_w = 22;
    constexpr int sprite_h = 22;
    constexpr int inset = 3;
    constexpr int inner_radius = 13;
    constexpr int outer_radius = 18;
    constexpr double pi = 3.14159265358979323846;

    const auto rnd = [&effects](std::uint32_t modulus) -> std::uint32_t {
        return modulus == 0 ? 0 : effects.next_rng() % modulus;
    };
    const auto perturb = [&](int x, int y) {
        if (x <= 0 || x >= 320 || y <= 0 || y >= 200) return;
        auto& px = fb.pixels()[static_cast<std::size_t>(y) * 320 + x];
        px = static_cast<std::uint8_t>(px + static_cast<std::uint8_t>(3u - rnd(4)));
    };
    const auto spark = [&](int x, int y) {
        if (x <= 0 || x >= 320 || y <= 0 || y >= 199) return;
        fb.pixels()[static_cast<std::size_t>(y) * 320 + x] =
            static_cast<std::uint8_t>(0x29u + rnd(7));
    };
    const auto cloud_cluster = [&](int inner_x, int inner_y, int outer_x, int outer_y) {
        spark(outer_x, outer_y);
        for (int i = 0; i < 8; ++i) {
            perturb(inner_x + static_cast<int>(rnd(10)) - 5,
                    inner_y + static_cast<int>(rnd(10)) - 5);
        }
    };

    for (int y = player.y + inset; y < player.y + sprite_h - inset; ++y) {
        for (int x = player.x + inset; x < player.x + sprite_w - inset; ++x) {
            if (x < 0 || x >= 320 || y < 0 || y >= 200) continue;
            auto& px = fb.pixels()[static_cast<std::size_t>(y) * 320 + x];
            px = static_cast<std::uint8_t>(px + static_cast<std::uint8_t>(3u - rnd(4)));
        }
    }

    const int cx = player.x + sprite_w / 2;
    const int cy = player.y + sprite_h / 2;
    for (int i = 0; i < 18; ++i) {
        const double angle = static_cast<double>(i * 5) * pi / 180.0;
        const double c = std::cos(angle);
        const double si = std::sin(angle);
        const int ix = cx + static_cast<int>(std::lround(inner_radius * c));
        const int iy = cy + static_cast<int>(std::lround(inner_radius * si));
        const int ox = cx + 2 + static_cast<int>(std::lround(outer_radius * c)) - static_cast<int>(rnd(5));
        const int oy = cy + 2 + static_cast<int>(std::lround(outer_radius * si)) - static_cast<int>(rnd(5));

        cloud_cluster(ix, iy, ox, oy);
        cloud_cluster(2 * cx - ix, iy, 2 * cx - ox, oy);
        cloud_cluster(2 * cx - ix, 2 * cy - iy, 2 * cx - ox, 2 * cy - oy);
        cloud_cluster(ix, 2 * cy - iy, ox, 2 * cy - oy);
    }
}


// Minimal libpulse-simple declarations so the playable host does not require
// development headers; the runtime library is optional and loaded dynamically.
struct pa_simple;
struct pa_sample_spec { int format; std::uint32_t rate; std::uint8_t channels; };
using pa_simple_new_fn = pa_simple*(*)(const char*, const char*, int, const char*, const char*, const pa_sample_spec*, const void*, const void*, int*);
using pa_simple_write_fn = int(*)(pa_simple*, const void*, std::size_t, int*);
using pa_simple_free_fn = void(*)(pa_simple*);

class AudioHost {
public:
    explicit AudioHost(AssetStore& assets) : assets_(assets) {}
    ~AudioHost() { stop(); }

    void start() {
        load_samples();
        running_ = true;
        thread_ = std::thread([this]{ thread_main(); });
    }

    void stop() {
        if (!running_.exchange(false)) return;
        cv_.notify_all();
        if (thread_.joinable()) thread_.join();
    }

    void push(std::span<const audio::AudioEvent> events) {
        std::lock_guard lock(mutex_);
        for (const auto& e : events) events_.push_back(e);
        cv_.notify_one();
    }

    void push(const audio::AudioEvent& event) {
        std::lock_guard lock(mutex_);
        events_.push_back(event);
        cv_.notify_one();
    }

private:
    void load_samples() {
        for (std::size_t i = 0; i < audio::portable_audio_cue_count; ++i) {
            const auto cue = static_cast<audio::AudioCue>(i);
            const auto& def = audio::audio_cue_definition(cue);
            std::string stem(def.original_asset);
            const auto dot = stem.find_last_of('.');
            if (dot != std::string::npos) stem.resize(dot);
            const auto filename = upper_ascii(stem + ".CLV");
            if (!assets_.exists(filename)) continue;
            try {
                const auto clv = formats::load_clv(assets_.root / filename);
                audio::PortablePcmSample sample;
                sample.sample_rate_hz = formats::ClvAudio::sample_rate;
                sample.channels = 2;
                sample.interleaved_samples.reserve(clv.interleaved_stereo_u8.size());
                for (const auto v : clv.interleaved_stereo_u8) {
                    sample.interleaved_samples.push_back(static_cast<std::int16_t>((static_cast<int>(v) - 128) << 8));
                }
                const int volume = def.original_volume_0_to_100 >= 0 ? def.original_volume_0_to_100 : 100;
                if (const auto gain = audio::portable_win32_game_volume_gain_q30(volume)) {
                    sample.default_left_gain_q30 = *gain;
                    sample.default_right_gain_q30 = *gain;
                }
                sample.default_playback_rate_hz = def.original_frequency_hz;
                (void)mixer_.set_sample(cue, std::move(sample));
            } catch (...) {
                // Individual missing/bad audio must not prevent gameplay.
            }
        }
    }

    void apply_events(std::deque<audio::AudioEvent>& local) {
        while (!local.empty()) {
            const auto event = local.front(); local.pop_front();
            if (const auto command = audio::lower_audio_event_for_original_backend(audio::OriginalAudioBackend::Win32DirectSound, event)) {
                (void)mixer_.execute(*command);
            }
        }
    }

    void thread_main() {
        void* pulse = dlopen("libpulse-simple.so.0", RTLD_NOW | RTLD_LOCAL);
        pa_simple* stream = nullptr;
        pa_simple_write_fn write_fn = nullptr;
        pa_simple_free_fn free_fn = nullptr;
        if (pulse) {
            const auto new_fn = reinterpret_cast<pa_simple_new_fn>(dlsym(pulse, "pa_simple_new"));
            write_fn = reinterpret_cast<pa_simple_write_fn>(dlsym(pulse, "pa_simple_write"));
            free_fn = reinterpret_cast<pa_simple_free_fn>(dlsym(pulse, "pa_simple_free"));
            if (new_fn && write_fn && free_fn) {
                int error = 0;
                const pa_sample_spec spec{3, 22050, 2}; // PA_SAMPLE_S16LE, 22.05 kHz stereo
                stream = new_fn(nullptr, "Drone", 1, nullptr, "Game audio", &spec, nullptr, nullptr, &error);
            }
        }

        constexpr std::size_t frames = 256;
        std::array<std::int16_t, frames * 2> buffer{};
        while (running_) {
            std::deque<audio::AudioEvent> local;
            {
                std::lock_guard lock(mutex_);
                local.swap(events_);
            }
            apply_events(local);
            (void)mixer_.render_stereo_i16(buffer, 22050);
            if (stream && write_fn) {
                int error = 0;
                if (write_fn(stream, buffer.data(), buffer.size() * sizeof(buffer[0]), &error) < 0) {
                    if (free_fn) free_fn(stream);
                    stream = nullptr;
                }
            } else {
                std::this_thread::sleep_for(std::chrono::duration<double>(static_cast<double>(frames) / 22050.0));
            }
        }
        if (stream && free_fn) free_fn(stream);
        if (pulse) dlclose(pulse);
    }

    AssetStore& assets_;
    audio::PortableAudioSampleMixer mixer_{audio::OriginalAudioBackend::Win32DirectSound};
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<audio::AudioEvent> events_;
};

struct KeySnapshot {
    std::array<char, 32> keymap{};
    Display* display{};

    explicit KeySnapshot(Display* d) : display(d) { XQueryKeymap(display, keymap.data()); }
    bool code_down(unsigned code) const {
        return code < 256 && (keymap[code >> 3] & (1 << (code & 7))) != 0;
    }
    bool down(KeySym sym) const {
        const KeyCode code = XKeysymToKeycode(display, sym);
        return code != 0 && code_down(code);
    }
};

std::optional<KeySym> first_new_keysym(Display* display,
                                       const KeySnapshot& current,
                                       const KeySnapshot& previous) {
    for (unsigned code = 8; code < 256; ++code) {
        if (!current.code_down(code) || previous.code_down(code)) continue;
        const KeySym sym = XkbKeycodeToKeysym(display, static_cast<KeyCode>(code), 0, 0);
        if (sym != NoSymbol) return sym;
    }
    return std::nullopt;
}

struct EdgeKeys {
    bool pause{};
    bool quit{};
    bool resume{};
};

struct PlayAssets {
    SpriteBank ship;
    SpriteBank missile;
    SpriteBank bomb;
    SpriteBank probe;
    SpriteBank redprobe;
    SpriteBank stinger_display;
    SpriteBank drone;
    SpriteBank explode;
    SpriteBank target;
    std::array<SpriteBank, 3> outcome_markers;
    SpriteBank outcome_cursor;
    SpriteBank junk1;
    SpriteBank junk2;
    SpriteBank wheel;
    std::array<SpriteBank, fidelity::objective_scaled_debris_sprite_count> objective_debris;
};

PlayAssets load_play_assets(AssetStore& a) {
    PlayAssets p;
    p.ship = extract_bank(a.jba("SHIP.JBA"), 22, 22, 15);
    p.missile = extract_bank(a.jba("MISSILE.JBA"), 1, 9, 3);
    p.bomb = extract_bank(a.jba("BOMB.JBA"), 1, 9, 3);
    p.probe = extract_bank(a.jba("PROBE.JBA"), 3, 8, 1);
    if (a.exists("REDPROBE.JBA")) p.redprobe = extract_bank(a.jba("REDPROBE.JBA"), 3, 8, 1);
    else p.redprobe = p.probe;
    p.stinger_display = extract_bank(a.jba("STINGER.JBA"), 61, 53, 6);
    p.drone = extract_bank(a.jba("DRONE.JBA"), 15, 38, 1);
    p.explode = extract_bank(a.jba("EXPLODE1.JBA"), 42, 38, 27);
    p.target = extract_bank(a.jba("TARGET.JBA"), 17, 13, 1);
    if (a.exists("MINIPRG.JBA")) p.outcome_markers[0] = extract_bank(a.jba("MINIPRG.JBA"), 11, 16, 1);
    if (a.exists("MINIPRB.JBA")) p.outcome_markers[1] = extract_bank(a.jba("MINIPRB.JBA"), 11, 16, 1);
    if (a.exists("MINIPRR.JBA")) p.outcome_markers[2] = extract_bank(a.jba("MINIPRR.JBA"), 11, 16, 1);
    if (a.exists("SQUARE.JBA")) p.outcome_cursor = extract_bank(a.jba("SQUARE.JBA"), 13, 18, 1);
    // Frame-loader loops establish 16-frame junk1/junk2 banks and a 14-frame
    // wheel bank (Win32 0x00409ECC..0x0040A05B).
    p.junk1 = extract_bank(a.jba("JUNK1.JBA"), 7, 7, 16);
    p.junk2 = extract_bank(a.jba("JUNK2.JBA"), 12, 9, 16);
    p.wheel = extract_bank(a.jba("WHEEL.JBA"), 5, 5, 14);
    const auto& desc = fidelity::objective_scaled_debris_descriptors();
    for (std::size_t i = 0; i < desc.size(); ++i) {
        p.objective_debris[i] = extract_bank(a.jba(upper_ascii(std::string(desc[i].asset))), desc[i].source_width, desc[i].source_height, desc[i].frame_count);
    }
    return p;
}

void update_effects(EffectRuntime& effects, const gameplay::GameSession& session,
                    const gameplay::GameSessionTickResult& tick) {
    // The simulation now exports exact destroyed trajectory actor identities and
    // pre-retirement geometry. Do not infer kill locations from generic
    // active->inactive transitions: natural path exits can occur in the same
    // tick and were previously stealing another actor's explosion.
    for (std::size_t i = 0; i < tick.trajectory_destroyed_actor_event_count; ++i) {
        const auto& destroyed = tick.trajectory_destroyed_actor_events[i];
        effects.spawn_explosion(
            destroyed.x + destroyed.sprite_width / 2,
            destroyed.y + destroyed.sprite_height / 2,
            true);
    }

    for (const auto& request : tick.drone_detonation_explosions) {
        if (!tick.drone_detonation_effect_tick) break;
        int x = request.x;
        int y = request.y;
        if (request.kind == gameplay::DroneDetonationExplosionKind::RadialRing) {
            constexpr double pi = 3.14159265358979323846;
            const double rad = static_cast<double>(request.angle_degrees) * pi / 180.0;
            x = request.center_x + static_cast<int>(std::lround(std::cos(rad) * request.radius)) + request.jitter_x;
            y = request.center_y + static_cast<int>(std::lround(std::sin(rad) * request.radius)) + request.jitter_y;
        }
        effects.spawn_explosion(x, y);
    }
    if (tick.drone_detonation_started) {
        effects.objective_active = true;
        const auto& desc = fidelity::objective_scaled_debris_descriptors();
        for (std::size_t i = 0; i < desc.size(); ++i) {
            auto& d = effects.objective[i];
            d.geom = {session.encounter.drone.detonation_center_x,
                      session.encounter.drone.detonation_center_y,
                      desc[i].source_width, desc[i].source_height};
            d.vx = desc[i].initial_velocity_x;
            d.vy = desc[i].initial_velocity_y;
            d.frame = 0;
            d.frame_count = desc[i].frame_count;
            d.bank = i;
            d.active = true;
        }
    }
    for (auto& e : effects.explosions) {
        if (!e.active) continue;
        ++e.age;
        if (tick.animation_tick) {
            ++e.frame;
            if (e.frame >= e.frame_count) e.active = false;
        }
    }
    effects.explosions.erase(std::remove_if(effects.explosions.begin(), effects.explosions.end(), [](const Explosion& e){ return !e.active; }), effects.explosions.end());

    // The original shares one updater across the three parallel 15-slot sprite
    // debris pools. Preserve that lifecycle; only the producer RNG remains a
    // host-side presentation approximation until its exact call-site stream is
    // folded into GameSession.
    const auto advance_debris_bank = [&](auto& bank) {
        for (auto& d : bank) {
            if (!d.active) continue;
            (void)gameplay::advance_debris_sprite(d, static_cast<std::uint8_t>(effects.next_rng() & 0x7fU));
        }
    };
    advance_debris_bank(effects.junk1);
    advance_debris_bank(effects.junk2);
    advance_debris_bank(effects.wheel);

    if (effects.objective_active) {
        bool any = false;
        for (auto& d : effects.objective) {
            if (!d.active) continue;
            d.geom.x += d.vx;
            d.geom.y += d.vy;
            d.frame = (d.frame + 1) % std::max(1, d.frame_count);
            fidelity::advance_objective_scaled_debris_growth(d.geom, static_cast<std::uint8_t>(tick.gameplay_substep_phase));
            d.active = fidelity::objective_scaled_debris_visible(d.geom);
            any |= d.active;
        }
        effects.objective_active = any;
    }
}

void render_debris_sprites(fidelity::IndexedFramebuffer& fb,
                           const EffectRuntime& effects,
                           const PlayAssets& sprites,
                           HdFramePlan* hd) {
    // Original compositor interleaves junk1[i] -> junk2[i] -> wheel[i].
    for (std::size_t i = 0; i < gameplay::canonical_debris_sprite_pool_size; ++i) {
        const auto draw = [&](const gameplay::DebrisSpriteState& d,
                              const SpriteBank& bank,
                              std::string_view family) {
            if (!d.active || bank.frames.empty()) return;
            const auto frame = static_cast<std::size_t>(d.current_frame) % bank.frames.size();
            blit_asset_sprite(fb, bank.frames[frame], d.x, d.y, hd,
                              "runtime_known", family, frame);
        };
        draw(effects.junk1[i], sprites.junk1, "JUNK1");
        draw(effects.junk2[i], sprites.junk2, "JUNK2");
        draw(effects.wheel[i], sprites.wheel, "WHEEL");
    }
}

struct GameplayControlLegend {
    std::string main_fire;
    std::string shield;
    std::string special_select;
    std::string special_launch;
    std::string resume_cancel;
};

enum class DroneFailureCause : std::uint8_t {
    Unknown,
    RapidMissile,
    Stinger,
    HoverTimeout,
};

struct ObjectiveAssistState {
    bool fire_release_required = false;
    std::uint32_t probe_lost_ticks_remaining = 0;
    std::uint32_t safety_notice_ticks_remaining = 0;
    std::uint32_t blocked_stinger_ticks_remaining = 0;
};

std::string_view drone_failure_cause_text(const DroneFailureCause cause) noexcept {
    switch (cause) {
    case DroneFailureCause::RapidMissile: return "YOUR RAPID MISSILE HIT THE DRONE";
    case DroneFailureCause::Stinger: return "YOUR RED STINGER HIT THE DRONE";
    case DroneFailureCause::HoverTimeout: return "THE DRONE TIMED OUT BEFORE DISARM";
    case DroneFailureCause::Unknown: break;
    }
    return {};
}

void render_game(fidelity::IndexedFramebuffer& fb,
                 HdFramePlan* hd,
                 const WorldImage& world,
                 const gameplay::GameSession& session,
                 const gameplay::GameSessionTickResult& tick,
                 const TrajectorySprites& trajectories,
                 const BossSprites& bosses,
                 const PlayAssets& sprites,
                 const FontCache& font,
                 EffectRuntime& effects,
                 fidelity::SpecialWeaponHudTimers& hud_timers,
                 std::int32_t drone_outcome_cursor_y,
                 bool paused,
                 bool quit_confirm,
                 bool debug_hud,
                 const GameplayControlLegend& control_legend,
                 bool weapon_help_visible,
                 bool objective_safety_enabled,
                 const ObjectiveAssistState& objective_assist) {
    render_world(fb, world, session.encounter.world_scroll_row);
    if (hd) hd->begin_world(world.source_pages, session.encounter.world_scroll_row, fb);

    // Major Win32 world-presentation ordering: boss/objective/special layers,
    // unscaled explosions + debris, trajectories, rapid missiles, bombs,
    // player, then the dedicated player-death singleton.
    const auto& boss = session.encounter.boss;
    if (boss.family == gameplay::BossFamily::LidTop) {
        const auto& b = boss.lid_top;
        if (b.top_activity != gameplay::boss_activity_inactive)
            blit_asset_sprite(fb, bosses.top.frames[0], b.root_x, b.root_y, hd,
                              "recovered", "TOP", 0);
        if (b.lid_activity != gameplay::boss_activity_inactive && b.lid_frame < bosses.lid.frames.size())
            blit_asset_sprite(fb, bosses.lid.frames[b.lid_frame], b.root_x + 16, b.root_y + 8, hd,
                              "recovered", "LID", b.lid_frame);
    } else if (boss.family == gameplay::BossFamily::Gemini) {
        const auto& b = boss.gemini;
        const auto render_side = [&](const gameplay::GeminiBossSideLifecycleState& side) {
            if (side.body_activity != gameplay::boss_activity_inactive && side.body_frame < bosses.gemini_body.size()) {
                const bool second = side.body_frame >= 15;
                blit_asset_sprite(fb, bosses.gemini_body[side.body_frame], side.body_x, side.body_y, hd,
                                  "recovered", second ? "GEMINI2" : "GEMINI1",
                                  second ? side.body_frame - 15 : side.body_frame);
            }
            if (side.head_activity != gameplay::boss_activity_inactive)
                blit_asset_sprite(fb, bosses.gemini_head, side.head_x, side.head_y, hd,
                                  "recovered", "GEMHEAD", 0);
        };
        render_side(b.side_a); render_side(b.side_b);
    }

    if (session.encounter.drone.activity != 0) {
        blit_asset_sprite(fb, sprites.drone.frames[0], session.encounter.drone.x, session.encounter.drone.y, hd,
                          "recovered", "DRONE", 0);
    }

    const auto& special = session.encounter.special_weapon;
    if (special.activity != gameplay::SpecialWeaponActivity::Inactive) {
        const auto& bank = special.kind == gameplay::SpecialWeaponKind::Stinger ? sprites.redprobe : sprites.probe;
        blit_asset_sprite(fb, bank.frames[0], special.x, special.y, hd,
                          "recovered",
                          special.kind == gameplay::SpecialWeaponKind::Stinger ? "REDPROBE" : "PROBE", 0);
    }
    if (session.encounter.stinger_display.active && session.encounter.stinger_display.current_frame < sprites.stinger_display.frames.size()) {
        blit_asset_sprite(fb, sprites.stinger_display.frames[session.encounter.stinger_display.current_frame],
                          session.encounter.stinger_display.x, session.encounter.stinger_display.y, hd,
                          "runtime_known", "STINGER", session.encounter.stinger_display.current_frame);
    }

    for (const auto& e : effects.explosions) {
        if (e.active && e.frame >= 0 && static_cast<std::size_t>(e.frame) < sprites.explode.frames.size())
            blit_asset_sprite(fb, sprites.explode.frames[static_cast<std::size_t>(e.frame)], e.x, e.y, hd,
                              "runtime_known", "EXPLODE1", static_cast<std::size_t>(e.frame));
    }
    render_debris_sprites(fb, effects, sprites, hd);

    for (std::size_t g = 0; g < session.encounter.trajectories.groups.size(); ++g) {
        const auto& group = session.encounter.trajectories.groups[g];
        if (group.lifecycle.mode == gameplay::TrajectoryGroupMode::Inactive) continue;
        for (const auto& actor : group.actors) {
            if (actor.activity == gameplay::TrajectoryEntityActivity::Inactive) continue;
            if (actor.current_frame < trajectories.banks[g].frames.size()) {
                blit_asset_sprite(fb, trajectories.banks[g].frames[actor.current_frame], actor.x, actor.y, hd,
                                  "trajectory", kGroupSheets[g], actor.current_frame);
            }
        }
    }

    for (const auto& missile : session.encounter.rapid_missiles.missiles) {
        if (missile.active && missile.frame < sprites.missile.frames.size())
            blit_asset_sprite(fb, sprites.missile.frames[missile.frame], missile.x, missile.y, hd,
                              "recovered", "MISSILE", missile.frame);
    }
    for (const auto& bomb : session.encounter.enemy_bombs.bombs) {
        if (bomb.active && bomb.frame < sprites.bomb.frames.size())
            blit_asset_sprite(fb, sprites.bomb.frames[bomb.frame], bomb.x, bomb.y, hd,
                              "recovered", "BOMB", bomb.frame);
    }

    if (session.campaign.player_lifecycle.player_active) {
        const auto frame = std::clamp(session.encounter.player.frame, 0, static_cast<int>(sprites.ship.frames.size() - 1));
        blit_asset_sprite(fb, sprites.ship.frames[static_cast<std::size_t>(frame)],
                          session.encounter.player.x, session.encounter.player.y, hd,
                          "recovered", "SHIP", static_cast<std::size_t>(frame));
        if (session.encounter.shield.active) {
            render_player_shield_effect(fb, session.encounter.player, effects);
        }
    }
    if (gameplay::player_death_effect_visible(session.encounter.player_death_effect) &&
        session.encounter.player_death_effect.frame >= 0 &&
        static_cast<std::size_t>(session.encounter.player_death_effect.frame) < sprites.explode.frames.size()) {
        blit_asset_sprite(fb,
            sprites.explode.frames[static_cast<std::size_t>(session.encounter.player_death_effect.frame)],
            session.encounter.player_death_effect.x, session.encounter.player_death_effect.y, hd,
            "runtime_known", "EXPLODE1",
            static_cast<std::size_t>(session.encounter.player_death_effect.frame));
    }

    // Objective debris uses the original scaled-overlay presentation path and
    // remains later than the ordinary unscaled explosion/debris batch.
    for (const auto& d : effects.objective) {
        if (!d.active || d.bank >= sprites.objective_debris.size()) continue;
        const auto& bank = sprites.objective_debris[d.bank];
        if (d.frame < 0 || static_cast<std::size_t>(d.frame) >= bank.frames.size()) continue;
        const auto destination = fidelity::objective_scaled_debris_destination(d.geom);
        blit_scaled_transparent(fb, bank.frames[static_cast<std::size_t>(d.frame)], destination);
        if (hd) {
            const auto& desc = fidelity::objective_scaled_debris_descriptors()[d.bank];
            hd->add_sprite("runtime_known", strip_extension_upper(std::string(desc.asset)),
                           static_cast<std::size_t>(d.frame), destination.left, destination.top,
                           destination.right - destination.left, destination.bottom - destination.top);
        }
    }

    std::array<std::uint8_t, fidelity::drone_outcome_marker_count> raw_outcomes{};
    for (std::size_t i = 0; i < raw_outcomes.size(); ++i) {
        raw_outcomes[i] = static_cast<std::uint8_t>(session.campaign.mission.outcomes[i]);
    }
    for (const auto& marker : fidelity::plan_drone_outcome_markers(raw_outcomes)) {
        if (!marker.visible || marker.frame_index >= sprites.outcome_markers.size()) continue;
        const auto& bank = sprites.outcome_markers[marker.frame_index];
        if (!bank.frames.empty()) {
            static constexpr std::array<const char*, 3> names{{"MINIPRG", "MINIPRB", "MINIPRR"}};
            blit_asset_sprite(fb, bank.frames[0], marker.x, marker.y, hd,
                              "runtime_known", names[marker.frame_index], 0);
        }
    }
    const auto cursor = fidelity::plan_drone_outcome_cursor(
        static_cast<std::uint8_t>(session.campaign.mission.processed_count), drone_outcome_cursor_y);
    if (cursor.visible && !sprites.outcome_cursor.frames.empty()) {
        blit_asset_sprite(fb, sprites.outcome_cursor.frames[0], cursor.x, cursor.y, hd,
                          "runtime_known", "SQUARE", 0);
    }

    for (const auto& row : fidelity::plan_shield_meter_rows(gameplay::displayed_shield_units(session.encounter.shield))) {
        if (row.width > 0) draw_hline(fb, row.x, row.y, row.width, row.palette_index);
    }

    const auto score = std::max(0, session.campaign.score.total);
    const auto score_place = fidelity::score_text_placement(score);
    draw_text(fb, font, score_place.x, score_place.y, std::to_string(score), score_place.palette_index);
    const auto lives_place = fidelity::lives_text_placement();
    draw_text(fb, font, lives_place.x, lives_place.y, std::to_string(std::max(0, session.campaign.player_lifecycle.lives)), lives_place.palette_index);

    const auto hud = fidelity::plan_special_weapon_status({
        static_cast<std::uint8_t>(special.activity),
        static_cast<std::int16_t>(special.probe_decode.phase1_elapsed),
        static_cast<std::int16_t>(special.probe_decode.phase1_threshold),
        static_cast<std::int16_t>(special.probe_decode.phase2_elapsed),
        static_cast<std::int16_t>(special.probe_decode.phase2_threshold)}, hud_timers);
    hud_timers = hud.next_timers;
    if (hud.visible) draw_text(fb, font, hud.placement.x, hud.placement.y, hud.text, hud.placement.palette_index);

    // Host usability aid only; gameplay semantics in GameSession remain the
    // recovered original.  The safety layer can be toggled with F5.  Its job is
    // to make the original two-step Probe workflow legible and prevent accidental
    // self-sabotage while the Probe is seeking/decoding, without changing the
    // underlying DRONE damage rules used by tests and parity work.
    const auto& drone = session.encounter.drone;
    const bool probe_attached =
        special.activity == gameplay::SpecialWeaponActivity::ProbeAttachedDecoding;
    const bool drone_objective_visible =
        drone.activity == gameplay::canonical_drone_active_activity &&
        !drone.disarm_completed &&
        drone.y >= -39 && drone.y <= gameplay::canonical_drone_hover_y;

    if (objective_assist.blocked_stinger_ticks_remaining > 0 &&
        !paused && !quit_confirm && !session.runtime.demo_playback_mode) {
        fill_rect(fb, 38, 3, 244, 19, 0x00);
        draw_text(fb, font, 56, 5, "RED STINGER BLOCKED FOR DRONE SAFETY", 0xF7);
        draw_text(fb, font, 50, 13, control_legend.special_select + " CYCLE TO BLUE PROBE", 28);
    } else if (objective_assist.safety_notice_ticks_remaining > 0 &&
               !paused && !quit_confirm && !session.runtime.demo_playback_mode) {
        fill_rect(fb, 50, 3, 220, 19, 0x00);
        draw_text(fb, font, 73, 5, std::string("OBJECTIVE SAFETY ") +
            (objective_safety_enabled ? "ENABLED" : "DISABLED"), 0xF7);
        draw_text(fb, font, 78, 13, "F5 TO TOGGLE ANY TIME", 28);
    } else if (probe_attached && !paused && !quit_confirm &&
        !session.runtime.demo_playback_mode) {
        fill_rect(fb, 30, 3, 260, 28, 0x00);
        const bool phase2 = special.probe_decode.status == gameplay::ProbeDecodeStatus::Phase2Disarming;
        const std::uint32_t elapsed = phase2 ? special.probe_decode.phase2_elapsed : special.probe_decode.phase1_elapsed;
        const std::uint32_t threshold = std::max<std::uint32_t>(1, phase2 ? special.probe_decode.phase2_threshold : special.probe_decode.phase1_threshold);
        const int percent = std::clamp(static_cast<int>((elapsed * 100u) / threshold), 0, 100);
        const std::string phase = phase2 ? "DISARMING " : "DECODING ";
        draw_text(fb, font, 53, 5, "BLUE PROBE ATTACHED - " + phase + std::to_string(percent) + "%", 0xF7);
        draw_text(fb, font, 64, 13, objective_safety_enabled
            ? "OBJECTIVE SAFETY ON - FIRE LOCKED"
            : "DO NOT FIRE AT THE DRONE", 28);
        fill_rect(fb, 52, 23, 216, 3, 0xF6);
        fill_rect(fb, 52, 23, (216 * percent) / 100, 3, 0xF7);
    } else if (objective_assist.probe_lost_ticks_remaining > 0 &&
               !paused && !quit_confirm && !session.runtime.demo_playback_mode) {
        fill_rect(fb, 42, 3, 236, 19, 0x00);
        draw_text(fb, font, 65, 5, "PROBE KNOCKED OFF - LAUNCH ANOTHER", 0xF7);
        draw_text(fb, font, 54, 13, control_legend.special_select + " LOAD/CYCLE   " +
            control_legend.special_launch + " LAUNCH", 28);
    } else if (drone_objective_visible && !paused && !quit_confirm &&
        !session.runtime.demo_playback_mode) {
        fill_rect(fb, 34, 3, 252, 27, 0x00);
        if (special.activity == gameplay::SpecialWeaponActivity::LaunchedHoming) {
            if (special.kind == gameplay::SpecialWeaponKind::Probe) {
                draw_text(fb, font, 55, 5, "BLUE PROBE IN FLIGHT - SEEKING DRONE", 0xF7);
                draw_text(fb, font, 64, 13, objective_safety_enabled
                    ? "OBJECTIVE SAFETY ON - FIRE LOCKED"
                    : "KEEP FIRE CLEAR OF THE DRONE", 28);
            } else {
                draw_text(fb, font, 46, 5, "RED STINGER IN FLIGHT - WAIT FOR RELOAD", 0xF7);
                draw_text(fb, font, 49, 13, "NEXT: LOAD BLUE PROBE, THEN " + control_legend.special_launch, 28);
            }
        } else if (special.activity == gameplay::SpecialWeaponActivity::LoadedTracking) {
            if (special.kind == gameplay::SpecialWeaponKind::Probe) {
                draw_text(fb, font, 67, 5, "BLUE PROBE READY - " + control_legend.special_launch + " LAUNCH", 0xF7);
                draw_text(fb, font, 68, 13, "THIS IS THE DRONE DISARM WEAPON", 28);
            } else {
                draw_text(fb, font, 48, 5, "RED STINGER SELECTED - DO NOT LAUNCH", 0xF7);
                draw_text(fb, font, 50, 13, control_legend.special_select + " CYCLE TO BLUE PROBE", 28);
            }
        } else {
            const std::string selected = special.kind == gameplay::SpecialWeaponKind::Probe
                ? "BLUE PROBE SELECTED" : "RED STINGER SELECTED";
            draw_text(fb, font, 68, 5, "DRONE OBJECTIVE - " + selected, 0xF7);
            draw_text(fb, font, 48, 13, control_legend.special_select + " LOAD/CYCLE   " +
                control_legend.special_launch + " LAUNCH", 28);
        }
        draw_text(fb, font, 76, 21, std::string("F5 OBJECTIVE SAFETY ") +
            (objective_safety_enabled ? "ON" : "OFF"), 0xF6);
    } else if (weapon_help_visible && !paused && !quit_confirm &&
               !session.runtime.demo_playback_mode) {
        fill_rect(fb, 58, 3, 204, 27, 0x00);
        draw_text(fb, font, 86, 5, "BLUE PROBE DISARMS DRONE", 28);
        draw_text(fb, font, 73, 13, control_legend.special_select + " LOAD/CYCLE PROBE/STINGER", 28);
        draw_text(fb, font, 79, 21, control_legend.special_launch + " LAUNCH   " + control_legend.main_fire + " FIRE", 28);
    }

    if ((special.activity == gameplay::SpecialWeaponActivity::LoadedTracking ||
         special.activity == gameplay::SpecialWeaponActivity::LaunchedHoming) &&
        !paused && !quit_confirm && !session.runtime.demo_playback_mode) {
        const bool launched = special.activity == gameplay::SpecialWeaponActivity::LaunchedHoming;
        const std::string prompt = special.kind == gameplay::SpecialWeaponKind::Probe
            ? (launched ? "BLUE PROBE - SEEKING DRONE"
                        : "BLUE PROBE READY - " + control_legend.special_launch + " LAUNCH")
            : (launched ? "RED STINGER - SEEKING ENEMY"
                        : "RED STINGER READY - " + control_legend.special_launch + " LAUNCH");
        fill_rect(fb, 62, 149, 196, 11, 0x00);
        draw_text(fb, font, 72, 151, prompt, 28);
    }

    if ((special.activity == gameplay::SpecialWeaponActivity::LoadedTracking ||
         special.activity == gameplay::SpecialWeaponActivity::LaunchedHoming) && !sprites.target.frames.empty()) {
        int tx = session.encounter.drone.x;
        int ty = session.encounter.drone.y;
        int tw = gameplay::canonical_drone_sprite_width;
        int th = gameplay::canonical_drone_sprite_height;
        if (special.kind == gameplay::SpecialWeaponKind::Stinger) {
            // GameSession now carries the actual retained target geometry,
            // including ordinary trajectory actors selected by the original
            // updater. Zero-sized dummy/stale targets use the historical host
            // fallback rather than fabricating a gameplay target.
            const auto& target = tick.stinger_target_geometry;
            if (target.width > 0 && target.height > 0) {
                tx = target.x; ty = target.y; tw = target.width; th = target.height;
            } else {
                tx = tick.stinger_target_desired_x - 8; ty = 1; tw = 16; th = 16;
            }
        }
        const auto pos = fidelity::special_target_reticle_placement({tx, ty, static_cast<std::int16_t>(tw), static_cast<std::int16_t>(th)});
        blit_asset_sprite(fb, sprites.target.frames[0], pos.x, pos.y, hd,
                          "runtime_known", "TARGET", 0);
    }

    if (paused || quit_confirm) {
        if (hd) hd->dim_background = true;
        // Win32 pause overlay darkens each palette component by 40 with floor 0.
        for (auto& c : fb.palette()) {
            c.r = static_cast<std::uint8_t>(c.r > 40 ? c.r - 40 : 0);
            c.g = static_cast<std::uint8_t>(c.g > 40 ? c.g - 40 : 0);
            c.b = static_cast<std::uint8_t>(c.b > 40 ? c.b - 40 : 0);
        }
        if (paused) {
            // Preserve the original pause state while making the reconstructed
            // host controls discoverable. These strings use the ACTIVE bindings,
            // including user remaps from the menu.
            draw_text(fb, font, 116, 62, "GAME PAUSED", 28);
            draw_text(fb, font, 74, 76, control_legend.main_fire + "  FIRE MISSILES", 28);
            draw_text(fb, font, 74, 86, control_legend.special_select + "  SELECT PROBE/STINGER", 28);
            draw_text(fb, font, 74, 96, control_legend.special_launch + "  LAUNCH SELECTED", 28);
            draw_text(fb, font, 74, 106, control_legend.shield + "  SHIELD", 28);
            draw_text(fb, font, 74, 116, std::string("F5  OBJECTIVE SAFETY ") +
                (objective_safety_enabled ? "ON" : "OFF"), 28);
            draw_text(fb, font, 74, 130, control_legend.resume_cancel + "  RESUMES", 28);
        } else {
            // Win32 0x0040C734..0x0040C77C. Confirmation is Y, not Q.
            draw_text(fb, font, 120, 80, "QUIT GAME?", 28);
            draw_text(fb, font, 116, 90, "<Y> TO QUIT", 28);
            draw_text(fb, font, 116, 100, "<R> RESUMES", 28);
        }
    }

    if (debug_hud) {
        std::size_t active_actors = 0;
        for (const auto& group : session.encounter.trajectories.groups) {
            for (const auto& actor : group.actors) {
                if (actor.activity != gameplay::TrajectoryEntityActivity::Inactive) ++active_actors;
            }
        }
        draw_text(fb, font, 4, 4, "DEV TICK " + std::to_string(session.total_gameplay_updates), 57);
        draw_text(fb, font, 4, 12, "GRP " + std::to_string(session.encounter.trajectories.active_group_count) +
                                  " ACT " + std::to_string(active_actors), 57);
        draw_text(fb, font, 4, 20, "NEW " + std::to_string(tick.trajectory_actors_activated) +
                                  " ESC " + std::to_string(tick.trajectory_actors_escaped) +
                                  " KILL " + std::to_string(tick.trajectory_actors_destroyed), 57);
        draw_text(fb, font, 4, 28, "ALIEN " + std::to_string(session.encounter.encounter_alien_ships_hit) +
                                  "/" + std::to_string(session.encounter.encounter_alien_ships_total), 57);
        draw_text(fb, font, 4, 36, "BRK " + std::to_string(tick.trajectory_groups_entered_breakaway) +
                                  " RNG " + std::to_string(tick.trajectory_breakaway_random_draws_consumed), 57);
    }
}

struct MissionInterstitialUi {
    static constexpr int surveillance_width = 160;
    static constexpr int surveillance_height = 100;
    static constexpr int confirm_lock_presentations = 58;

    bool active = false;
    std::string outcome_asset;
    std::string mission_asset;
    std::string failure_detail;
    std::array<std::uint8_t, surveillance_width * surveillance_height> surveillance{};
    bool surveillance_valid = false;
    std::int32_t alien_ships_hit = 0;
    std::int32_t alien_ships_total = 0;
    std::int32_t score = 0;
    int confirm_lock_remaining = confirm_lock_presentations;
};

std::string mission_outcome_asset(const gameplay::MissionInterstitialPlan& plan) {
    const char* prefix = plan.tone == gameplay::MissionInterstitialTone::Good ? "GOOD" : "BAD";
    return std::string(prefix) + std::to_string(std::max<int>(1, plan.result_ordinal)) + ".JBA";
}

std::string mission_briefing_asset(const gameplay::MissionInterstitialPlan& plan) {
    switch (plan.briefing) {
    case gameplay::MissionBriefingCard::Mission1: return "MISSION1.JBA";
    case gameplay::MissionBriefingCard::Mission2: return "MISSION2.JBA";
    case gameplay::MissionBriefingCard::Mission3: return "MISSION3.JBA";
    case gameplay::MissionBriefingCard::Mission4: return "MISSION4.JBA";
    case gameplay::MissionBriefingCard::Mission5: return "MISSION5.JBA";
    case gameplay::MissionBriefingCard::Mission6Yes: return "MISS6YES.JBA";
    case gameplay::MissionBriefingCard::Mission6No: return "MISS6NO.JBA";
    }
    return "MISSION1.JBA";
}


void capture_mission_surveillance(
    const fidelity::IndexedFramebuffer& fb,
    std::array<std::uint8_t, MissionInterstitialUi::surveillance_width *
                                  MissionInterstitialUi::surveillance_height>& out) {
    // Win32 0x0040BB72..0x0040BBDF stores a 160x100 surveillance buffer by
    // sampling every second source pixel and every second source row from the
    // 320x200 software framebuffer. The buffer is later copied verbatim into
    // the mission card at x=14/y=81.
    const auto& pixels = fb.pixels();
    for (int y = 0; y < MissionInterstitialUi::surveillance_height; ++y) {
        for (int x = 0; x < MissionInterstitialUi::surveillance_width; ++x) {
            out[static_cast<std::size_t>(y) * MissionInterstitialUi::surveillance_width + x] =
                pixels[static_cast<std::size_t>(y * 2) * 320 + x * 2];
        }
    }
}

void render_mission_interstitial(
    fidelity::IndexedFramebuffer& fb,
    HdFramePlan* hd,
    AssetStore& assets,
    const FontCache& font,
    const MissionInterstitialUi& ui) {
    if (!assets.exists(ui.mission_asset) || !assets.exists(ui.outcome_asset)) return;

    // The original 0x0041D690 path does NOT show GOODn/BADn as a fullscreen
    // page. It captures only the first 280x37 pixels from that JBA into a
    // transparent sprite, loads MISSIONn/MISS6*, and blits the outcome sprite
    // at x=17/y=27. Pixel index 0 is transparent.
    const auto& mission = assets.jba(ui.mission_asset);
    const auto& outcome = assets.jba(ui.outcome_asset);
    render_fullscreen_image(fb, mission);
    if (hd) hd->begin_fullscreen(ui.mission_asset, fb);

    constexpr int banner_w = 280;
    constexpr int banner_h = 37;
    constexpr int banner_x = 17;
    constexpr int banner_y = 27;
    for (int y = 0; y < banner_h; ++y) {
        for (int x = 0; x < banner_w; ++x) {
            const auto px = outcome.pixels[static_cast<std::size_t>(y) * 320 + x];
            if (px == 0) continue;
            fb.pixels()[static_cast<std::size_t>(banner_y + y) * 320 + banner_x + x] = px;
        }
    }

    // 0x0041D8E1..0x0041D918 copies the saved 160x100 surveillance image
    // into the framed aperture at x=14/y=81.
    if (ui.surveillance_valid) {
        constexpr int photo_x = 14;
        constexpr int photo_y = 81;
        for (int y = 0; y < MissionInterstitialUi::surveillance_height; ++y) {
            const auto* src = ui.surveillance.data() +
                static_cast<std::size_t>(y) * MissionInterstitialUi::surveillance_width;
            auto* dst = fb.pixels().data() +
                static_cast<std::size_t>(photo_y + y) * 320 + photo_x;
            std::copy_n(src, MissionInterstitialUi::surveillance_width, dst);
        }
    }

    // Exact dynamic-stat positions from 0x0041D91A..0x0041DA62. The original
    // bitmap-text call uses palette index 0x1C for these five values.
    const auto hit = std::max<std::int32_t>(0, ui.alien_ships_hit);
    const auto total = std::max<std::int32_t>(0, ui.alien_ships_total);
    const auto missed = std::max<std::int32_t>(0, total - hit);
    const auto percentage = total > 0
        ? static_cast<std::int32_t>((static_cast<std::int64_t>(hit) * 100) / total)
        : 0;
    draw_text(fb, font, 272, 98, std::to_string(hit), 0x1C);
    draw_text(fb, font, 272, 112, std::to_string(missed), 0x1C);
    draw_text(fb, font, 272, 126, std::to_string(total), 0x1C);
    draw_text(fb, font, 272, 140, std::to_string(percentage) + "%", 0x1C);
    draw_text(fb, font, 272, 154, std::to_string(ui.score), 0x1C);

    if (!ui.failure_detail.empty()) {
        // Host diagnostic only; keep it above the original ENTER footer.
        fill_rect(fb, 34, 65, 252, 10, 0x00);
        draw_text(fb, font, 42, 66, ui.failure_detail, 0xF7);
    }
}

std::string results_art_asset(const gameplay::Win32PostGamePlan& plan, const AssetStore& assets) {
    const auto index = static_cast<int>(plan.outcome_summary.disarm_art_index);
    if (index == 2 && assets.exists("DISARM2S.JBA")) return "DISARM2S.JBA";
    const auto normal = std::string("DISARM") + std::to_string(index) + ".JBA";
    if (assets.exists(normal)) return normal;
    return assets.exists("GAMEOVER.JBA") ? "GAMEOVER.JBA" : "TITLESH.JBA";
}

void render_post_game(fidelity::IndexedFramebuffer& fb,
                      HdFramePlan* hd,
                      AssetStore& assets,
                      const gameplay::GameSession& session,
                      const FontCache& font,
                      int ordering_page) {
    if (!session.post_game.plan) return;
    const auto phase = session.post_game.phase;
    if (phase == gameplay::PostGameModalPhase::ResultsConfirmLock ||
        phase == gameplay::PostGameModalPhase::ResultsAwaitConfirmation) {
        const auto result_asset = results_art_asset(*session.post_game.plan, assets);
        render_fullscreen_image(fb, assets.jba(result_asset));
        if (hd) hd->begin_fullscreen(result_asset, fb);
        const auto& st = session.post_game.plan->statistics;
        // The original result art carries its own labels.  These values are the
        // six recovered dynamic statistics; keep the small overlay compact until
        // their exact pixel coordinates are recovered.
        draw_text(fb, font, 4, 158, "HIT " + std::to_string(st.alien_ships_hit), 57);
        draw_text(fb, font, 4, 166, "MISSED " + std::to_string(st.alien_ships_missed), 57);
        draw_text(fb, font, 4, 174, "TOTAL " + std::to_string(st.alien_ships_total), 57);
        draw_text(fb, font, 4, 182, "SCORE " + std::to_string(st.score), 57);
        if (phase == gameplay::PostGameModalPhase::ResultsAwaitConfirmation)
            draw_text(fb, font, 214, 190, "ENTER", 57);
        return;
    }
    if (phase == gameplay::PostGameModalPhase::OrderingInformation) {
        const auto page = std::clamp(ordering_page, 1, 5);
        const auto name = std::string("ORDER") + std::to_string(page) + ".JBA";
        if (assets.exists(name)) {
            render_fullscreen_image(fb, assets.jba(name));
            if (hd) hd->begin_fullscreen(name, fb);
        }
        draw_text(fb, font, 250, 190, "ENTER", 57);
        return;
    }
    if (phase == gameplay::PostGameModalPhase::HighScoreTable) {
        std::fill(fb.pixels().begin(), fb.pixels().end(), 0);
        draw_text(fb, font, 112, 48, "HIGH SCORES", 57);
        int y = 68;
        for (const auto& entry : session.high_scores) {
            draw_text(fb, font, 56, y, entry.name, 57);
            draw_text(fb, font, 220, y, std::to_string(entry.score), 57);
            y += 10;
        }
        draw_text(fb, font, 238, 190, "ENTER", 57);
        return;
    }
    if (phase == gameplay::PostGameModalPhase::CompletionCredits) {
        std::fill(fb.pixels().begin(), fb.pixels().end(), 0);
        draw_text(fb, font, 96, 94, "DRONE COMPLETE", 57);
        draw_text(fb, font, 238, 190, "ENTER", 57);
    }
}


enum class FrontEndMode : std::uint8_t {
    MainMenu,
    Difficulty,
    Instructions,
    Ordering,
    HighScores,
    ConfigureJoystick,
    VideoSettings,
    Gameplay,
};

enum class HostControlAction : std::uint8_t {
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    RapidFire,
    Shield,
    SpecialLoad,
    SpecialLaunch,
    Pause,
    QuitPrompt,
    ConfirmQuit,
    ResumeCancel,
    Count,
};

struct HostControlDefinition {
    std::string_view config_name;
    std::string_view label;
    KeySym default_primary;
    KeySym default_secondary;
};

constexpr std::array<HostControlDefinition, static_cast<std::size_t>(HostControlAction::Count)>
kHostControlDefinitions{{
    {"MOVE_LEFT",      "MOVE LEFT",          XK_Left,      NoSymbol},
    {"MOVE_RIGHT",     "MOVE RIGHT",         XK_Right,     NoSymbol},
    {"MOVE_UP",        "MOVE UP",            XK_a,         NoSymbol},
    {"MOVE_DOWN",      "MOVE DOWN",          XK_z,         NoSymbol},
    {"RAPID_FIRE",     "MAIN FIRE / MISSILES", XK_Control_L, XK_Control_R},
    {"SHIELD",         "SHIELD",               XK_space,     NoSymbol},
    {"SPECIAL_LOAD",   "SELECT PROBE/STINGER", XK_Down,      NoSymbol},
    {"SPECIAL_LAUNCH", "LAUNCH PROBE/STINGER", XK_Up,        NoSymbol},
    {"PAUSE",          "PAUSE",              XK_p,         NoSymbol},
    {"QUIT_PROMPT",    "QUIT PROMPT",        XK_q,         XK_Escape},
    {"CONFIRM_QUIT",   "CONFIRM QUIT",       XK_y,         NoSymbol},
    {"RESUME_CANCEL",  "RESUME/CANCEL",      XK_r,         NoSymbol},
}};

constexpr std::size_t host_control_index(HostControlAction action) noexcept {
    return static_cast<std::size_t>(action);
}

std::string friendly_keysym(KeySym sym) {
    switch (sym) {
    case NoSymbol: return "NONE";
    case XK_Left: return "LEFT";
    case XK_Right: return "RIGHT";
    case XK_Up: return "UP";
    case XK_Down: return "DOWN";
    case XK_Control_L: return "LCTRL";
    case XK_Control_R: return "RCTRL";
    case XK_Shift_L: return "LSHIFT";
    case XK_Shift_R: return "RSHIFT";
    case XK_Alt_L: return "LALT";
    case XK_Alt_R: return "RALT";
    case XK_Super_L: return "LSUPER";
    case XK_Super_R: return "RSUPER";
    case XK_space: return "SPACE";
    case XK_Escape: return "ESC";
    case XK_Return: return "ENTER";
    case XK_KP_Enter: return "KP ENTER";
    case XK_BackSpace: return "BACKSPACE";
    case XK_Tab: return "TAB";
    default:
        break;
    }
    if (const char* raw = XKeysymToString(sym)) return upper_ascii(raw);
    return "KEY";
}

std::string config_keysym_name(KeySym sym) {
    if (sym == NoSymbol) return {};
    if (const char* raw = XKeysymToString(sym)) return raw;
    return {};
}

struct ControlBindings {
    std::array<std::pair<KeySym, KeySym>, static_cast<std::size_t>(HostControlAction::Count)> values{};
    fs::path config_path{};
    std::string status{};

    explicit ControlBindings(fs::path path) : config_path(std::move(path)) {
        restore_defaults(false);
        load();
    }

    bool down(const KeySnapshot& keys, HostControlAction action) const {
        const auto& binding = values[host_control_index(action)];
        return (binding.first != NoSymbol && keys.down(binding.first)) ||
               (binding.second != NoSymbol && keys.down(binding.second));
    }

    std::string display(HostControlAction action) const {
        const auto& binding = values[host_control_index(action)];
        auto text = friendly_keysym(binding.first);
        if (binding.second != NoSymbol) text += "/" + friendly_keysym(binding.second);
        return text;
    }

    void restore_defaults(bool persist = true) {
        for (std::size_t i = 0; i < values.size(); ++i) {
            values[i] = {kHostControlDefinitions[i].default_primary,
                         kHostControlDefinitions[i].default_secondary};
        }
        if (persist) status = save() ? "ALL DEFAULTS RESTORED" : "DEFAULTS ACTIVE - SAVE FAILED";
    }

    void restore_default(HostControlAction action) {
        const auto i = host_control_index(action);
        values[i] = {kHostControlDefinitions[i].default_primary,
                     kHostControlDefinitions[i].default_secondary};
        status = save() ? "DEFAULT RESTORED" : "DEFAULT ACTIVE - SAVE FAILED";
    }

    bool assign(HostControlAction action, KeySym sym) {
        if (sym == NoSymbol) {
            status = "UNSUPPORTED KEY";
            return false;
        }
        const auto selected = host_control_index(action);
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i == selected) continue;
            if (values[i].first == sym || values[i].second == sym) {
                status = "KEY USED BY " + std::string(kHostControlDefinitions[i].label);
                return false;
            }
        }
        values[selected] = {sym, NoSymbol};
        status = save() ? "BINDING SAVED" : "BINDING ACTIVE - SAVE FAILED";
        return true;
    }

    void load() {
        std::ifstream in(config_path);
        if (!in) return;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            const auto equals = line.find('=');
            if (equals == std::string::npos) continue;
            const std::string key = line.substr(0, equals);
            const std::string encoded = line.substr(equals + 1);
            for (std::size_t i = 0; i < kHostControlDefinitions.size(); ++i) {
                if (key != kHostControlDefinitions[i].config_name) continue;
                const auto split = encoded.find('|');
                const std::string first_name = encoded.substr(0, split);
                const std::string second_name = split == std::string::npos ? std::string{} : encoded.substr(split + 1);
                const KeySym first = first_name.empty() ? NoSymbol : XStringToKeysym(first_name.c_str());
                const KeySym second = second_name.empty() ? NoSymbol : XStringToKeysym(second_name.c_str());
                if (first != NoSymbol) values[i] = {first, second};
                break;
            }
        }
    }

    bool save() const {
        std::ofstream out(config_path, std::ios::trunc);
        if (!out) return false;
        out << "# Drone reconstructed Linux host controls v1\n";
        out << "# Delete this file or use D in the control editor to restore defaults.\n";
        for (std::size_t i = 0; i < values.size(); ++i) {
            out << kHostControlDefinitions[i].config_name << '='
                << config_keysym_name(values[i].first);
            if (values[i].second != NoSymbol) out << '|' << config_keysym_name(values[i].second);
            out << '\n';
        }
        return static_cast<bool>(out);
    }
};

struct VideoPreferences {
    fs::path config_path{};
    bool prefer_hd{true};
    int scale_mode{0}; // 0 = AUTO, otherwise exact integer scale.
    HdFilterMode filter{HdFilterMode::Smooth};
    std::string status{};

    explicit VideoPreferences(fs::path path) : config_path(std::move(path)) { load(); }

    void restore_defaults(bool persist = true) {
        prefer_hd = true;
        scale_mode = 0;
        filter = HdFilterMode::Smooth;
        if (persist) status = save() ? "VIDEO DEFAULTS RESTORED" : "DEFAULTS ACTIVE - SAVE FAILED";
    }

    void load() {
        std::ifstream in(config_path);
        if (!in) return;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            const auto equals = line.find('=');
            if (equals == std::string::npos) continue;
            const auto key = upper_ascii(line.substr(0, equals));
            const auto value = upper_ascii(line.substr(equals + 1));
            if (key == "ART_MODE") {
                prefer_hd = value != "CLASSIC";
            } else if (key == "SCALE") {
                if (value == "AUTO") scale_mode = 0;
                else {
                    try { scale_mode = std::clamp(std::stoi(value), kMinScale, kMaxScale); } catch (...) {}
                }
            } else if (key == "HD_FILTER") {
                filter = value == "SHARP" ? HdFilterMode::Sharp : HdFilterMode::Smooth;
            }
        }
    }

    bool save() const {
        std::ofstream out(config_path, std::ios::trunc);
        if (!out) return false;
        out << "# Drone reconstructed Linux video preferences v1\n";
        out << "ART_MODE=" << (prefer_hd ? "HD" : "CLASSIC") << '\n';
        out << "SCALE=" << (scale_mode == 0 ? std::string("AUTO") : std::to_string(scale_mode)) << '\n';
        out << "HD_FILTER=" << hd_filter_name(filter) << '\n';
        return static_cast<bool>(out);
    }
};

struct DemoPlaybackRuntime {
    // Win32 0x0042B97C initializes to -1 and the attract/demo launch path
    // pre-increments/wraps it through four shareware demos.
    int selector = -1;
    std::string filename{};
    std::vector<formats::DemoFrame> frames{};
    gameplay::DemoReplayTimeline timeline{};

    [[nodiscard]] bool active() const noexcept { return !frames.empty(); }

    void clear() {
        filename.clear();
        frames.clear();
        timeline.reset();
    }
};

std::optional<gameplay::TrajectoryPathFamily> demo_trajectory_path_family(const std::int32_t value) {
    // Win32 0x0040D0CC..0x0040D118 jump table, cross-checked against
    // trajectory-pointer globals in the reverse ledger.
    switch (value) {
    case 0: return gameplay::TrajectoryPathFamily::LeftDive;
    case 1: return gameplay::TrajectoryPathFamily::RightDive;
    case 2: return gameplay::TrajectoryPathFamily::LeftDrop;
    case 3: return gameplay::TrajectoryPathFamily::RightDrop;
    default: return std::nullopt;
    }
}

void begin_next_original_demo(
    DemoPlaybackRuntime& replay,
    AssetStore& assets,
    gameplay::GameSession& session,
    WorldImage& world) {

    // Exact Win32 selector order from 0x0041A08C..0x0041A3B5.  The first
    // launch turns the executable's initial -1 byte into selector 0.
    replay.selector = (replay.selector + 1) % 4;
    struct DemoChoice { const char* filename; bool desert; };
    static constexpr std::array<DemoChoice, 4> choices{{
        {"DEMOA4.DAT", false},
        {"DEMOB3.DAT", false},
        {"DEMOA2.DAT", true},
        {"DEMOB1.DAT", true},
    }};
    const auto& choice = choices[static_cast<std::size_t>(replay.selector)];
    const auto path = assets.resolve(choice.filename);
    if (!fs::exists(path)) {
        throw std::runtime_error(std::string("missing original demo replay: ") + choice.filename);
    }

    replay.filename = choice.filename;
    replay.frames = formats::load_demo_frames(path);
    replay.timeline.reset();

    session.runtime.demo_playback_mode = true;
    gameplay::reset_game_session(session, gameplay::GameplaySessionResetScope::FullCampaign);
    world = choice.desert
        ? load_world_stack(assets, {"DESERTOP.JBA", "DESERMID.JBA", "DESERBOT.JBA"})
        : load_initial_river_world(assets);
}

void apply_demo_replay_checkpoints(
    gameplay::GameSession& session,
    const gameplay::DemoGameplayFrame& frame,
    const gameplay::TrajectoryPathCatalogView& paths) {

    if (frame.trajectory.spawn && frame.trajectory.group_slot >= 0 &&
        frame.trajectory.group_slot < static_cast<std::int32_t>(gameplay::canonical_trajectory_group_count)) {
        const auto group_index = static_cast<std::size_t>(frame.trajectory.group_slot);
        auto& group = session.encounter.trajectories.groups[group_index];
        if (frame.trajectory.path_family) {
            if (const auto family = demo_trajectory_path_family(*frame.trajectory.path_family)) {
                group.path_family = *family;
            }
        }
        if (gameplay::activate_transient_trajectory_group(
                session.encounter.trajectories, group_index, paths,
                static_cast<std::int16_t>(frame.trajectory.group_x_offset), 0)) {
            // Win32 0x0040D248..0x0040D255 increments the encounter-local
            // alien total alongside activation ownership.
            ++session.encounter.encounter_alien_ships_total;
        }
    }

    if (frame.bomb.spawn) {
        (void)gameplay::spawn_replay_enemy_bomb(
            session.encounter.enemy_bombs, frame.bomb.x, frame.bomb.y);
    }

    // Channels 13/14 are explicit deterministic Drone checkpoints.  Applying
    // them before the clean gameplay step gives collision/targeting systems
    // the recorded position instead of allowing live RNG/motion to diverge.
    session.encounter.drone.x = frame.drone.x;
    session.encounter.drone.y = frame.drone.y;
}

constexpr std::array<std::string_view, 8> kMainMenuLabels{{
    "START GAME",
    "INSTRUCTIONS",
    "ORDERING INFORMATION",
    "HIGH SCORES",
    "CONFIGURE CONTROLS",
    "VIDEO SETTINGS",
    "PLAY DEMO",
    "EXIT DRONE",
}};

struct MenuTextPlacement {
    int x;
    int y;
};

// The first five and final two entries retain the recovered front-end vocabulary.
// VIDEO SETTINGS is an explicit host/remaster extension so display preferences are
// discoverable without relying on F-key shortcuts.
constexpr std::array<MenuTextPlacement, 8> kMainMenuPlacements{{
    {125, 67},
    {117, 77},
    {85, 87},
    {121, 97},
    {96, 107},
    {108, 117},
    {128, 127},
    {124, 137},
}};

constexpr std::array<std::string_view, 3> kDifficultyLabels{{
    "BEGINNER", "INTERMEDIATE", "ADVANCED"
}};
constexpr std::array<MenuTextPlacement, 3> kDifficultyPlacements{{
    {125, 94}, {111, 104}, {125, 114}
}};

void render_main_menu(fidelity::IndexedFramebuffer& fb,
                      HdFramePlan* hd,
                      AssetStore& assets,
                      const FontCache& font,
                      int selection) {
    render_fullscreen_image(fb, assets.jba("TITLESH.JBA"));
    if (hd) hd->begin_fullscreen("TITLESH.JBA", fb);
    selection = std::clamp(selection, 0, static_cast<int>(kMainMenuLabels.size()) - 1);
    for (std::size_t i = 0; i < kMainMenuLabels.size(); ++i) {
        draw_text(fb, font, kMainMenuPlacements[i].x, kMainMenuPlacements[i].y,
                  kMainMenuLabels[i], static_cast<std::uint8_t>(i == static_cast<std::size_t>(selection) ? 0xF7 : 0xF6));
    }
}

void blit_frontend_modal(fidelity::IndexedFramebuffer& fb,
                         const formats::JbaImage& image,
                         int dst_x = 65,
                         int dst_y = 61) {
    // CHOOSE.JBA / CONFIGUR.JBA are 320x200 containers whose actual modal art
    // occupies the top-left 192x87. Win32 slides this extracted panel down to
    // its resting position x=65, y=61 (run_skill_selector 0x0041A828..0x0041A867).
    constexpr int modal_w = 192;
    constexpr int modal_h = 87;
    for (int y = 0; y < modal_h; ++y) {
        for (int x = 0; x < modal_w; ++x) {
            const auto px = image.pixels[static_cast<std::size_t>(y) * 320 + x];
            if (px == 0) continue;
            const int dx = dst_x + x;
            const int dy = dst_y + y;
            if (dx >= 0 && dx < 320 && dy >= 0 && dy < 200) {
                fb.pixels()[static_cast<std::size_t>(dy) * 320 + dx] = px;
            }
        }
    }
}

void render_difficulty(fidelity::IndexedFramebuffer& fb,
                       HdFramePlan* hd,
                       AssetStore& assets,
                       const FontCache& font,
                       gameplay::DifficultyLevel difficulty) {
    render_fullscreen_image(fb, assets.jba("TITLESH.JBA"));
    if (hd) hd->begin_fullscreen("TITLESH.JBA", fb);
    blit_frontend_modal(fb, assets.jba("CHOOSE.JBA"));
    const int selected = std::clamp(static_cast<int>(difficulty) - 1, 0, 2);
    for (std::size_t i = 0; i < kDifficultyLabels.size(); ++i) {
        draw_text(fb, font, kDifficultyPlacements[i].x, kDifficultyPlacements[i].y,
                  kDifficultyLabels[i], static_cast<std::uint8_t>(i == static_cast<std::size_t>(selected) ? 0xF7 : 0xF6));
    }
}

void render_controls_editor(fidelity::IndexedFramebuffer& fb,
                            HdFramePlan* hd,
                            AssetStore& assets,
                            const FontCache& font,
                            const ControlBindings& controls,
                            int selection,
                            bool waiting_for_key,
                            int display_scale) {
    render_fullscreen_image(fb, assets.jba("TITLESH.JBA"));
    if (hd) hd->begin_fullscreen("TITLESH.JBA", fb);

    // This is a host-extension of the original CONFIGURE JOYSTICK path.  Keep
    // the original 320x200/paletted presentation vocabulary, but make the
    // reconstructed Linux input path visible and actually editable.
    fill_rect(fb, 6, 24, 308, 172, 0xF6);
    fill_rect(fb, 8, 26, 304, 168, 0x00);
    draw_text(fb, font, 93, 30, "CONFIGURE CONTROLS", 0xF7);
    draw_hline(fb, 14, 41, 292, 0xF6);

    selection = std::clamp(selection, 0, static_cast<int>(kHostControlDefinitions.size()) - 1);
    int y = 43;
    for (std::size_t i = 0; i < kHostControlDefinitions.size(); ++i, y += 8) {
        const bool selected = i == static_cast<std::size_t>(selection);
        const auto color = static_cast<std::uint8_t>(selected ? 0xF7 : 0xF6);
        if (selected) draw_text(fb, font, 12, y, ">", color);
        draw_text(fb, font, 22, y, kHostControlDefinitions[i].label, color);
        const auto binding_text = selected && waiting_for_key
            ? std::string("<PRESS KEY>")
            : controls.display(static_cast<HostControlAction>(i));
        draw_text(fb, font, 190, y, binding_text, color);
    }

    draw_hline(fb, 14, 138, 292, 0xF6);
    draw_text(fb, font, 16, 143, "BLUE PROBE DISARMS DRONES", 0xF7);
    draw_text(fb, font, 16, 151, "RED STINGER MISSILE ATTACKS ENEMIES", 0xF7);
    draw_text(fb, font, 16, 160,
              "SCALE " + std::to_string(display_scale) + "X   VIDEO SETTINGS ON MAIN MENU", 0xF6);
    draw_text(fb, font, 16, 169,
              waiting_for_key
                  ? (controls.status.empty() ? "PRESS NEW KEY - ESC CANCEL" : controls.status)
                  : (controls.status.empty() ? "AMMO UNLIMITED - DOWN LOADS/CYCLES, UP LAUNCHES" : controls.status),
              waiting_for_key ? 0xF7 : 0xF6);
    draw_text(fb, font, 16, 178, "ENTER REBIND   BACKSPACE DEFAULT", 0xF6);
    draw_text(fb, font, 16, 187, "D ALL DEFAULTS   ESC BACK", 0xF6);
}

struct VideoSettingsView {
    bool hd_available{};
    bool hd_enabled{};
    int active_scale{kMinScale};
    int maximum_scale{kMinScale};
    std::size_t hd_asset_count{};
    bool last_background_hd{};
    std::size_t last_sprite_hits{};
    std::size_t last_sprite_misses{};
};

void render_video_settings(fidelity::IndexedFramebuffer& fb,
                           HdFramePlan* hd,
                           AssetStore& assets,
                           const FontCache& font,
                           const VideoPreferences& video,
                           const VideoSettingsView& view,
                           int selection) {
    render_fullscreen_image(fb, assets.jba("TITLESH.JBA"));
    if (hd) hd->begin_fullscreen("TITLESH.JBA", fb);
    fill_rect(fb, 24, 38, 272, 136, 0xF6);
    fill_rect(fb, 26, 40, 268, 132, 0x00);
    draw_text(fb, font, 104, 47, "VIDEO SETTINGS", 0xF7);
    draw_hline(fb, 34, 58, 252, 0xF6);

    const auto row = [&](int index, int y, std::string_view label, const std::string& value) {
        const bool selected = selection == index;
        const auto color = static_cast<std::uint8_t>(selected ? 0xF7 : 0xF6);
        if (selected) draw_text(fb, font, 34, y, ">", color);
        draw_text(fb, font, 44, y, label, color);
        draw_text(fb, font, 182, y, value, color);
    };

    row(0, 68, "ART MODE",
        view.hd_enabled ? "HD 12X" : "CLASSIC");
    const std::string scale_value = video.scale_mode == 0
        ? "AUTO " + std::to_string(view.active_scale) + "X"
        : std::to_string(view.active_scale) + "X";
    row(1, 82, "SCALE", scale_value);
    row(2, 96, "FILTER", std::string(hd_filter_name(video.filter)));
    row(3, 110, "DEFAULTS", "ENTER");

    draw_hline(fb, 34, 124, 252, 0xF6);
    draw_text(fb, font, 36, 130,
              view.hd_available
                  ? ("HD CACHE " + std::to_string(view.hd_asset_count) + " PNGS")
                  : "HD CACHE NOT AVAILABLE",
              view.hd_available ? 0xF7 : 0xF6);
    draw_text(fb, font, 36, 139,
              "FRAME BG " + std::string(view.last_background_hd ? "HD" : "CLASSIC") +
                  " SPR " + std::to_string(view.last_sprite_hits) +
                  "/" + std::to_string(view.last_sprite_hits + view.last_sprite_misses), 0xF6);
    if (!video.status.empty()) draw_text(fb, font, 36, 148, video.status, 0xF7);
    else draw_text(fb, font, 36, 148, "LEFT/RIGHT CHANGE", 0xF6);
    draw_text(fb, font, 36, 159, "ENTER SELECT   ESC BACK", 0xF6);
}

void render_frontend(fidelity::IndexedFramebuffer& fb,
                     HdFramePlan* hd,
                     AssetStore& assets,
                     const FontCache& font,
                     const gameplay::GameSession& session,
                     const ControlBindings& controls,
                     FrontEndMode mode,
                     int main_menu_selection,
                     int instructions_page,
                     int ordering_page,
                     int control_selection,
                     bool control_waiting_for_key,
                     int display_scale,
                     const VideoPreferences& video,
                     const VideoSettingsView& video_view,
                     int video_selection) {
    switch (mode) {
    case FrontEndMode::MainMenu:
        render_main_menu(fb, hd, assets, font, main_menu_selection);
        return;
    case FrontEndMode::Difficulty:
        render_difficulty(fb, hd, assets, font, session.runtime.difficulty);
        return;
    case FrontEndMode::Instructions: {
        const int page = std::clamp(instructions_page, 1, 9);
        const auto name = std::string("INSTR0") + std::to_string(page) + ".JBA";
        render_fullscreen_image(fb, assets.jba(name));
        if (hd) hd->begin_fullscreen(name, fb);
        return;
    }
    case FrontEndMode::Ordering: {
        const int page = std::clamp(ordering_page, 1, 5);
        const auto name = std::string("ORDER") + std::to_string(page) + ".JBA";
        render_fullscreen_image(fb, assets.jba(name));
        if (hd) hd->begin_fullscreen(name, fb);
        return;
    }
    case FrontEndMode::HighScores: {
        const std::string high_score_asset = assets.exists("TOPFLYER.JBA") ? "TOPFLYER.JBA" : "TITLESH.JBA";
        render_fullscreen_image(fb, assets.jba(high_score_asset));
        if (hd) hd->begin_fullscreen(high_score_asset, fb);
        {
            // TOPFLYER.JBA already contains the title, rank numerals and Escape
            // footer. Fill only the ten dynamic entry rows.
            int y = 64;
            for (auto it = session.high_scores.rbegin(); it != session.high_scores.rend(); ++it) {
                if (!it->name.empty()) draw_text(fb, font, 28, y, it->name, 0xF7);
                if (it->score != 0) draw_text(fb, font, 230, y, std::to_string(it->score), 0xF7);
                y += 12;
            }
        }
        return;
    }
    case FrontEndMode::ConfigureJoystick:
        render_controls_editor(fb, hd, assets, font, controls, control_selection,
                               control_waiting_for_key, display_scale);
        return;
    case FrontEndMode::VideoSettings:
        render_video_settings(fb, hd, assets, font, video, video_view, video_selection);
        return;
    case FrontEndMode::Gameplay:
        return;
    }
}

struct X11Presenter {
    Display* display{};
    int screen{};
    Window window{};
    GC gc{};
    XImage* image{};
    std::vector<std::uint32_t> pixels;
    Atom wm_delete{};
    HdAssetStore* hd_assets{};
    bool hd_enabled{false};
    bool last_hd_background{false};
    std::size_t last_hd_sprite_hits{0};
    std::size_t last_hd_sprite_misses{0};
    int scale{kMinScale};
    int window_w{kLogicalW};
    int window_h{kLogicalH};

    static int choose_auto_scale(Display* display, int screen) noexcept {
        // Keep enough room for normal desktop decorations while choosing the
        // largest exact integer multiple of the original 320x200 framebuffer.
        const int usable_w = std::max(kLogicalW, DisplayWidth(display, screen) - 64);
        const int usable_h = std::max(kLogicalH, DisplayHeight(display, screen) - 64);
        int chosen = kMinScale;
        for (int candidate = kMinScale; candidate <= kMaxScale; ++candidate) {
            if (kLogicalW * candidate > usable_w || kLogicalH * candidate > usable_h) break;
            chosen = candidate;
        }
        return chosen;
    }

    int maximum_fitting_scale() const noexcept {
        return choose_auto_scale(display, screen);
    }

    explicit X11Presenter(std::optional<int> requested_scale = std::nullopt,
                          HdAssetStore* hd_store = nullptr,
                          bool enable_hd = false)
        : hd_assets(hd_store),
          hd_enabled(enable_hd && hd_store && hd_store->available) {
        display = XOpenDisplay(nullptr);
        if (!display) throw std::runtime_error("XOpenDisplay failed");
        screen = DefaultScreen(display);
        const int fit_max = choose_auto_scale(display, screen);
        scale = std::clamp(requested_scale.value_or(fit_max), kMinScale, fit_max);
        if (hd_assets) hd_assets->set_scale(scale);
        window_w = kLogicalW * scale;
        window_h = kLogicalH * scale;
        window = XCreateSimpleWindow(display, RootWindow(display, screen), 50, 50, window_w, window_h, 0,
                                     BlackPixel(display, screen), BlackPixel(display, screen));
        XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask);
        wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display, window, &wm_delete, 1);
        gc = XCreateGC(display, window, 0, nullptr);
        rebuild_image();
        apply_window_hints();
        update_title();
        XMapWindow(display, window);
    }

    ~X11Presenter() {
        destroy_image();
        if (gc) XFreeGC(display, gc);
        if (window) XDestroyWindow(display, window);
        if (display) XCloseDisplay(display);
    }

    void destroy_image() noexcept {
        if (!image) return;
        image->data = nullptr;
        XDestroyImage(image);
        image = nullptr;
    }

    void rebuild_image() {
        destroy_image();
        window_w = kLogicalW * scale;
        window_h = kLogicalH * scale;
        pixels.assign(static_cast<std::size_t>(window_w) * window_h, 0);
        image = XCreateImage(display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap, 0,
                             reinterpret_cast<char*>(pixels.data()), window_w, window_h, 32, 0);
        if (!image) throw std::runtime_error("XCreateImage failed");
    }

    void apply_window_hints() {
        XSizeHints hints{};
        hints.flags = PMinSize | PMaxSize | PBaseSize;
        hints.min_width = hints.max_width = hints.base_width = window_w;
        hints.min_height = hints.max_height = hints.base_height = window_h;
        XSetWMNormalHints(display, window, &hints);
    }

    void update_title() {
        std::string mode = " CLASSIC";
        if (hd_enabled && hd_assets)
            mode = " HD:" + std::to_string(hd_assets->png_file_count);
        const std::string title = "Drone — Reconstructed Playable Host [" +
                                  std::to_string(scale) + "x" + mode + "]";
        XStoreName(display, window, title.c_str());
    }

    bool set_hd_enabled(bool enabled) {
        if (enabled && (!hd_assets || !hd_assets->available)) return false;
        if (hd_enabled == enabled) return false;
        hd_enabled = enabled;
        if (hd_assets) hd_assets->set_scale(scale);
        update_title();
        XClearWindow(display, window);
        XFlush(display);
        return true;
    }

    bool toggle_hd() {
        return set_hd_enabled(!hd_enabled);
    }

    bool set_hd_filter(HdFilterMode filter) {
        if (!hd_assets) return false;
        if (hd_assets->filter == filter) return false;
        hd_assets->set_filter(filter);
        XClearWindow(display, window);
        XFlush(display);
        return true;
    }

    bool set_scale(int requested) {
        // Do not let F3 create a fixed-size window larger than the current
        // desktop. The previous host allowed 8x (2560x1600) even on a 1080p/
        // ~1200p desktop, which clipped mission cards and made successful
        // transitions appear corrupt.
        const int next = std::clamp(requested, kMinScale, maximum_fitting_scale());
        if (next == scale) return false;
        scale = next;
        if (hd_assets) hd_assets->set_scale(scale);
        rebuild_image();
        XResizeWindow(display, window, static_cast<unsigned>(window_w), static_cast<unsigned>(window_h));
        apply_window_hints();
        update_title();
        XClearWindow(display, window);
        XFlush(display);
        return true;
    }

    bool pump_close() {
        while (XPending(display)) {
            XEvent e{}; XNextEvent(display, &e);
            if (e.type == ClientMessage && static_cast<Atom>(e.xclient.data.l[0]) == wm_delete) return true;
        }
        return false;
    }

    static std::uint32_t pack_rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                  bool dim = false) noexcept {
        if (dim) {
            r = static_cast<std::uint8_t>(r > 40 ? r - 40 : 0);
            g = static_cast<std::uint8_t>(g > 40 ? g - 40 : 0);
            b = static_cast<std::uint8_t>(b > 40 ? b - 40 : 0);
        }
        return (static_cast<std::uint32_t>(r) << 16) |
               (static_cast<std::uint32_t>(g) << 8) | b;
    }

    void render_classic_full(const fidelity::IndexedFramebuffer& fb) {
        const auto& palette = fb.palette();
        const auto& src = fb.pixels();
        for (int y = 0; y < kLogicalH; ++y) {
            for (int x = 0; x < kLogicalW; ++x) {
                const auto c = palette[src[static_cast<std::size_t>(y) * kLogicalW + x]];
                const std::uint32_t rgb = pack_rgb(c.r, c.g, c.b);
                const int oy = y * scale;
                const int ox = x * scale;
                for (int sy = 0; sy < scale; ++sy)
                    std::fill_n(pixels.data() + static_cast<std::size_t>(oy + sy) * window_w + ox, scale, rgb);
            }
        }
    }

    bool render_hd_background(const HdFramePlan& plan) {
        if (!hd_assets || !hd_assets->available) return false;
        if (plan.background == HdBackgroundKind::Fullscreen) {
            const auto* image = hd_assets->fullscreen(plan.fullscreen_asset, scale);
            if (!image || image->width != window_w || image->height != window_h) return false;
            for (int y = 0; y < window_h; ++y) {
                for (int x = 0; x < window_w; ++x) {
                    const auto i = (static_cast<std::size_t>(y) * window_w + x) * 4u;
                    pixels[static_cast<std::size_t>(y) * window_w + x] =
                        pack_rgb(image->pixels[i], image->pixels[i + 1], image->pixels[i + 2],
                                 plan.dim_background);
                }
            }
            return true;
        }
        if (plan.background == HdBackgroundKind::World) {
            std::array<const RgbaImage*, 3> pages{};
            for (std::size_t i = 0; i < pages.size(); ++i) {
                pages[i] = hd_assets->fullscreen(plan.world_assets[i], scale);
                if (!pages[i] || pages[i]->width != window_w || pages[i]->height != window_h) return false;
            }
            const int page_h = kLogicalH * scale;
            const int world_h = page_h * 3;
            int source_y = (plan.world_scroll_row * scale) % world_h;
            if (source_y < 0) source_y += world_h;
            for (int y = 0; y < window_h; ++y) {
                const int wy = (source_y + y) % world_h;
                const int page = wy / page_h;
                const int py = wy % page_h;
                const auto* image = pages[static_cast<std::size_t>(page)];
                for (int x = 0; x < window_w; ++x) {
                    const auto i = (static_cast<std::size_t>(py) * window_w + x) * 4u;
                    pixels[static_cast<std::size_t>(y) * window_w + x] =
                        pack_rgb(image->pixels[i], image->pixels[i + 1], image->pixels[i + 2],
                                 plan.dim_background);
                }
            }
            return true;
        }
        return false;
    }

    void overlay_classic_deltas(const fidelity::IndexedFramebuffer& fb,
                                const HdFramePlan& plan) {
        if (!plan.base_valid) return;
        const auto& palette = fb.palette();
        const auto& src = fb.pixels();
        const auto covered_by_hd_sprite = [&](int px, int py) {
            if (!hd_assets) return false;
            for (const auto& sprite : plan.sprites) {
                // Only suppress the classic pixels when a real HD replacement
                // exists. The previous compositor suppressed planned sprites
                // even when their PNG mapping was absent, creating invisible
                // actors instead of a correct classic fallback.
                if (!hd_assets->has_sprite(sprite.category, sprite.family, sprite.frame)) continue;
                if (px >= sprite.x && py >= sprite.y &&
                    px < sprite.x + sprite.logical_width &&
                    py < sprite.y + sprite.logical_height) return true;
            }
            return false;
        };
        for (int y = 0; y < kLogicalH; ++y) {
            for (int x = 0; x < kLogicalW; ++x) {
                const auto li = static_cast<std::size_t>(y) * kLogicalW + x;
                if (src[li] == plan.base_pixels[li]) continue;
                // Asset-backed sprites are replaced by their transparent HD
                // counterparts below. Do not leave the old chunky indexed
                // sprite underneath their alpha edges.
                if (covered_by_hd_sprite(x, y)) continue;
                const auto c = palette[src[li]];
                const std::uint32_t rgb = pack_rgb(c.r, c.g, c.b);
                const int oy = y * scale;
                const int ox = x * scale;
                for (int sy = 0; sy < scale; ++sy)
                    std::fill_n(pixels.data() + static_cast<std::size_t>(oy + sy) * window_w + ox, scale, rgb);
            }
        }
    }

    bool blit_hd_sprite(const HdSpriteDraw& cmd, bool dim) {
        if (!hd_assets) return false;
        const int dst_w = cmd.logical_width * scale;
        const int dst_h = cmd.logical_height * scale;
        const auto* image = hd_assets->sprite(cmd.category, cmd.family, cmd.frame, dst_w, dst_h);
        if (!image) return false;

        const int origin_x = cmd.x * scale;
        const int origin_y = cmd.y * scale;
        for (int sy = 0; sy < image->height; ++sy) {
            const int dy = origin_y + sy;
            if (dy < 0 || dy >= window_h) continue;
            for (int sx = 0; sx < image->width; ++sx) {
                const int dx = origin_x + sx;
                if (dx < 0 || dx >= window_w) continue;
                const auto si = (static_cast<std::size_t>(sy) * image->width + sx) * 4u;
                const std::uint8_t alpha = image->pixels[si + 3];
                if (alpha == 0) continue;
                std::uint8_t r = image->pixels[si];
                std::uint8_t g = image->pixels[si + 1];
                std::uint8_t b = image->pixels[si + 2];
                if (dim) {
                    r = static_cast<std::uint8_t>(r > 40 ? r - 40 : 0);
                    g = static_cast<std::uint8_t>(g > 40 ? g - 40 : 0);
                    b = static_cast<std::uint8_t>(b > 40 ? b - 40 : 0);
                }
                auto& dst = pixels[static_cast<std::size_t>(dy) * window_w + dx];
                if (alpha == 255) {
                    dst = pack_rgb(r, g, b);
                } else {
                    const std::uint8_t dr = static_cast<std::uint8_t>((dst >> 16) & 0xffu);
                    const std::uint8_t dg = static_cast<std::uint8_t>((dst >> 8) & 0xffu);
                    const std::uint8_t db = static_cast<std::uint8_t>(dst & 0xffu);
                    const unsigned inv = 255u - alpha;
                    const auto rr = static_cast<std::uint8_t>((r * alpha + dr * inv + 127u) / 255u);
                    const auto gg = static_cast<std::uint8_t>((g * alpha + dg * inv + 127u) / 255u);
                    const auto bb = static_cast<std::uint8_t>((b * alpha + db * inv + 127u) / 255u);
                    dst = pack_rgb(rr, gg, bb);
                }
            }
        }
        return true;
    }

    void present(const fidelity::IndexedFramebuffer& fb, const HdFramePlan* plan = nullptr) {
        last_hd_background = false;
        last_hd_sprite_hits = 0;
        last_hd_sprite_misses = 0;
        const bool use_hd = hd_enabled && plan && plan->base_valid && render_hd_background(*plan);
        last_hd_background = use_hd;
        if (!use_hd) {
            render_classic_full(fb);
        } else {
            overlay_classic_deltas(fb, *plan);
            for (const auto& sprite : plan->sprites) {
                if (blit_hd_sprite(sprite, plan->dim_background)) ++last_hd_sprite_hits;
                else ++last_hd_sprite_misses;
            }
        }
        XPutImage(display, window, gc, image, 0, 0, 0, 0, window_w, window_h);
        XFlush(display);
    }
};

} // namespace

int main(int argc, char** argv) {
    try {
        fs::path asset_root = "assets";
        bool asset_root_set = false;
        fs::path hd_root{};
        bool hd_root_set = false;
        bool prefer_hd = true;
        bool require_hd = false;
        bool hd_self_test = false;
        bool scale_cli_override = false;
        bool art_cli_override = false;
        bool filter_cli_override = false;
        HdFilterMode requested_filter = HdFilterMode::Smooth;
        std::optional<int> requested_scale{};

        const auto parse_scale = [&](std::string_view value) -> bool {
            if (value == "auto") {
                requested_scale.reset();
                return true;
            }
            try {
                const int parsed = std::stoi(std::string(value));
                if (parsed < kMinScale || parsed > kMaxScale) return false;
                requested_scale = parsed;
                return true;
            } catch (...) {
                return false;
            }
        };

        if (const char* env_scale = std::getenv("DRONE_SCALE")) {
            scale_cli_override = true;
            if (!parse_scale(env_scale)) {
                std::cerr << "DRONE_SCALE must be auto or an integer from "
                          << kMinScale << " to " << kMaxScale << "\n";
                return 2;
            }
        }

        for (int i = 1; i < argc; ++i) {
            const std::string_view arg = argv[i];
            if (arg == "--scale") {
                scale_cli_override = true;
                if (i + 1 >= argc || !parse_scale(argv[++i])) {
                    std::cerr << "--scale requires auto or an integer from "
                              << kMinScale << " to " << kMaxScale << "\n";
                    return 2;
                }
            } else if (arg.rfind("--scale=", 0) == 0) {
                scale_cli_override = true;
                if (!parse_scale(arg.substr(8))) {
                    std::cerr << "--scale requires auto or an integer from "
                              << kMinScale << " to " << kMaxScale << "\n";
                    return 2;
                }
            } else if (arg == "--hd-art") {
                prefer_hd = true;
                art_cli_override = true;
            } else if (arg == "--classic-art") {
                prefer_hd = false;
                art_cli_override = true;
            } else if (arg == "--require-hd") {
                prefer_hd = true;
                art_cli_override = true;
                require_hd = true;
            } else if (arg == "--hd-self-test") {
                prefer_hd = true;
                art_cli_override = true;
                require_hd = true;
                hd_self_test = true;
            } else if (arg == "--hd-filter") {
                if (i + 1 >= argc) {
                    std::cerr << "--hd-filter requires smooth or sharp\n";
                    return 2;
                }
                const auto mode = upper_ascii(argv[++i]);
                if (mode != "SMOOTH" && mode != "SHARP") {
                    std::cerr << "--hd-filter requires smooth or sharp\n";
                    return 2;
                }
                requested_filter = mode == "SHARP" ? HdFilterMode::Sharp : HdFilterMode::Smooth;
                filter_cli_override = true;
            } else if (arg == "--hd-root") {
                if (i + 1 >= argc) {
                    std::cerr << "--hd-root requires a directory\n";
                    return 2;
                }
                hd_root = fs::path(argv[++i]);
                hd_root_set = true;
            } else if (arg.rfind("--hd-root=", 0) == 0) {
                hd_root = fs::path(std::string(arg.substr(10)));
                hd_root_set = true;
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "usage: " << argv[0]
                          << " [asset-directory] [--scale auto|1..8] [--hd-art|--classic-art|--require-hd]"
                             " [--hd-root DIR] [--hd-self-test] [--hd-filter smooth|sharp]\n"
                          << "DRONE_SCALE may also set the startup integer scale.\n"
                          << "HD art defaults to <asset-directory>/../assets_hd when present; F6 toggles it.\n"
                          << "Video preferences persist in drone-video.cfg and are editable from the main menu.\n";
                return 0;
            } else if (!asset_root_set) {
                asset_root = fs::path(arg);
                asset_root_set = true;
            } else {
                std::cerr << "unexpected argument: " << arg << "\n";
                return 2;
            }
        }

        if (!fs::exists(asset_root / "SHIP.JBA")) {
            std::cerr << "usage: " << argv[0]
                      << " [asset-directory] [--scale auto|1..8] [--hd-art|--classic-art|--require-hd] [--hd-root DIR] [--hd-self-test] [--hd-filter smooth|sharp]\n"
                      << "asset directory must contain SHIP.JBA and the original Drone data files\n";
            return 2;
        }

        AssetStore assets(asset_root);
        fs::path controls_root = asset_root.has_parent_path() ? asset_root.parent_path() : fs::path{};
        if (controls_root.empty()) controls_root = fs::current_path();
        VideoPreferences video(controls_root / "drone-video.cfg");
        if (!scale_cli_override) {
            requested_scale = video.scale_mode == 0 ? std::optional<int>{} : std::optional<int>{video.scale_mode};
        }
        if (!art_cli_override) prefer_hd = video.prefer_hd;
        if (!filter_cli_override) requested_filter = video.filter;
        else video.filter = requested_filter;

        if (!hd_root_set) {
            const fs::path parent = asset_root.has_parent_path() ? asset_root.parent_path() : fs::current_path();
            hd_root = parent / "assets_hd";
        }
        HdAssetStore hd_assets(hd_root);
        hd_assets.set_filter(requested_filter);
        if (hd_self_test) return hd_assets.self_test(std::cout) ? 0 : 3;
        if (prefer_hd && !hd_assets.available) {
            std::cerr << "HD art not usable: " << hd_assets.diagnostic_summary() << "\n";
            if (require_hd) {
                std::cerr << "HD mode was required; refusing silent CLASSIC fallback.\n";
                return 3;
            }
            std::cerr << "Falling back to classic JBA rendering.\n";
        } else if (prefer_hd && hd_assets.available) {
            std::cout << "HD_ART_ACTIVE " << hd_assets.diagnostic_summary()
                      << " (F6 toggles CLASSIC/HD)\n";
        }
        ControlBindings controls(controls_root / "drone-controls.cfg");

        auto world = load_initial_river_world(assets);
        auto paths = load_paths(asset_root);
        auto trajectory_sprites = load_trajectory_sprites(assets);
        auto boss_sprites = load_boss_sprites(assets);
        auto play_assets = load_play_assets(assets);
        auto font = load_font(assets);

        gameplay::GameSession session;
        gameplay::reset_game_session(session, gameplay::GameplaySessionResetScope::FullCampaign);
        gameplay::GameSessionTargetContext targets{};
        targets.trajectory_paths = &paths.view;
        targets.trajectory_sprite_masks = &trajectory_sprites.masks;
        targets.lid_top_sprite_mask = &boss_sprites.lid_mask;
        targets.gemini_sprite_masks = &boss_sprites.gemini_masks;

        AudioHost audio(assets);
        audio.start();
        audio::MainMenuAudioRuntimeState menu_audio{};
        audio.push(audio::begin_original_main_menu_audio(menu_audio).view());

        X11Presenter x11(requested_scale, &hd_assets, prefer_hd);
        // Command-line overrides are temporary; menu/F-key changes below become
        // persistent preferences in drone-video.cfg.
        if (!scale_cli_override && video.scale_mode != 0) video.scale_mode = x11.scale;
        if (!art_cli_override) video.prefer_hd = x11.hd_enabled;
        fidelity::IndexedFramebuffer framebuffer;
        HdFramePlan hd_plan{};
        fidelity::SpecialWeaponHudTimers hud_timers{};
        gameplay::GameSessionTickResult last_tick{};
        EffectRuntime effects{};
        MissionInterstitialUi interstitial{};
        std::array<std::uint8_t, MissionInterstitialUi::surveillance_width *
                                  MissionInterstitialUi::surveillance_height> mission_surveillance{};
        bool mission_surveillance_valid = false;
        bool mission_surveillance_capture_pending = false;
        FrontEndMode frontend = FrontEndMode::MainMenu;
        DemoPlaybackRuntime demo_replay{};
        int main_menu_selection = 0;
        int instructions_page = 1;
        int ordering_page = 1;
        int control_selection = 0;
        bool control_waiting_for_key = false;
        int video_selection = 0;
        std::int32_t drone_outcome_cursor_y = fidelity::drone_outcome_cursor_initial_y;

        bool paused = false;
        bool quit_confirm = false;
        bool debug_hud = false;
        bool weapon_help_visible = false;
        bool weapon_help_pinned = false;
        std::uint32_t weapon_help_ticks_remaining = 0;
        ObjectiveAssistState objective_assist{};
        bool objective_safety_enabled = true;
        DroneFailureCause drone_failure_cause = DroneFailureCause::Unknown;
        bool prev_p = false, prev_q = false, prev_r = false, prev_y = false, prev_escape = false;
        bool prev_f1 = false, prev_f2 = false, prev_f3 = false, prev_f4 = false, prev_f5 = false, prev_f6 = false;
        bool prev_enter = false, prev_up = false, prev_down = false, prev_left = false, prev_right = false;
        KeySnapshot previous_keys(x11.display);

        auto next_tick = std::chrono::steady_clock::now();
        bool running = true;
        while (running) {
            if (x11.pump_close()) break;
            const auto poll_now = std::chrono::steady_clock::now();
            if (poll_now < next_tick) {
                std::this_thread::sleep_until(next_tick);
                continue;
            }
            // Input is sampled once per logical host tick, matching the original
            // polling cadence and avoiding hundreds of synchronous XQueryKeymap
            // round trips per second.
            const KeySnapshot keys(x11.display);
            const auto newly_pressed = first_new_keysym(x11.display, keys, previous_keys);
            const bool control_was_waiting_for_key = control_waiting_for_key;

            const bool p = controls.down(keys, HostControlAction::Pause);
            const bool r = controls.down(keys, HostControlAction::ResumeCancel);
            const bool q = controls.down(keys, HostControlAction::QuitPrompt);
            const bool y = controls.down(keys, HostControlAction::ConfirmQuit);
            const bool escape = keys.down(XK_Escape);
            const bool up = keys.down(XK_Up);
            const bool down = keys.down(XK_Down);
            const bool left = keys.down(XK_Left);
            const bool right = keys.down(XK_Right);
            const bool f1 = keys.down(XK_F1);
            const bool f2 = keys.down(XK_F2);
            const bool f3 = keys.down(XK_F3);
            const bool f4 = keys.down(XK_F4);
            const bool f5 = keys.down(XK_F5);
            const bool f6 = keys.down(XK_F6);
            const bool enter = keys.down(XK_Return) || keys.down(XK_KP_Enter);
            const bool enter_edge = enter && !prev_enter;
            const bool up_edge = up && !prev_up;
            const bool down_edge = down && !prev_down;
            const bool left_edge = left && !prev_left;
            const bool right_edge = right && !prev_right;
            const bool escape_edge = escape && !prev_escape;
            const bool q_edge = q && !prev_q;
            const bool y_edge = y && !prev_y;
            const bool r_edge = r && !prev_r;

            if (f1 && !prev_f1) debug_hud = !debug_hud;
            if (frontend == FrontEndMode::Gameplay && f5 && !prev_f5 &&
                !session.runtime.demo_playback_mode) {
                objective_safety_enabled = !objective_safety_enabled;
                objective_assist.safety_notice_ticks_remaining = 210;
                objective_assist.blocked_stinger_ticks_remaining = 0;
            }
            if (frontend == FrontEndMode::Gameplay && f4 && !prev_f4 &&
                !session.runtime.demo_playback_mode) {
                if (weapon_help_visible) {
                    weapon_help_visible = false;
                    weapon_help_pinned = false;
                    weapon_help_ticks_remaining = 0;
                } else {
                    weapon_help_visible = true;
                    weapon_help_pinned = true;
                    weapon_help_ticks_remaining = 0;
                }
            }
            bool scale_changed = false;
            if (f2 && !prev_f2 && x11.set_scale(x11.scale - 1)) {
                scale_changed = true;
                video.scale_mode = x11.scale;
                video.status = video.save() ? "WINDOW SCALE SAVED" : "SCALE ACTIVE - SAVE FAILED";
            }
            if (f3 && !prev_f3 && x11.set_scale(x11.scale + 1)) {
                scale_changed = true;
                video.scale_mode = x11.scale;
                video.status = video.save() ? "WINDOW SCALE SAVED" : "SCALE ACTIVE - SAVE FAILED";
            }
            bool hd_changed = false;
            if (f6 && !prev_f6 && x11.toggle_hd()) {
                hd_changed = true;
                video.prefer_hd = x11.hd_enabled;
                video.status = video.save() ? "ART MODE SAVED" : "ART MODE ACTIVE - SAVE FAILED";
            }

            if (frontend != FrontEndMode::Gameplay) {
                switch (frontend) {
                case FrontEndMode::MainMenu:
                    if (up_edge) main_menu_selection = (main_menu_selection + static_cast<int>(kMainMenuLabels.size()) - 1) % static_cast<int>(kMainMenuLabels.size());
                    if (down_edge) main_menu_selection = (main_menu_selection + 1) % static_cast<int>(kMainMenuLabels.size());
                    if (enter_edge) {
                        // Exact menu selection order recovered from Win32 run_main_menu.
                        switch (main_menu_selection) {
                        case 0: // START GAME -> raw state 2, then synchronous skill selector.
                            audio.push(audio::leave_original_main_menu_audio(menu_audio, 2).view());
                            audio.push(audio::original_main_menu_air_restart(session.original_audio, 2).view());
                            session.runtime.demo_playback_mode = false;
                            demo_replay.clear();
                            frontend = FrontEndMode::Difficulty;
                            break;
                        case 1: // INSTRUCTIONS -> raw state 3; LowBees remains owned.
                            instructions_page = 1;
                            frontend = FrontEndMode::Instructions;
                            break;
                        case 2: // ORDERING INFORMATION -> raw state 7.
                            audio.push(audio::leave_original_main_menu_audio(menu_audio, 7).view());
                            {
                                drone::audio::AudioEventQueue events{};
                                (void)events.push({drone::audio::AudioCue::OrderingInformation,
                                                   drone::audio::AudioAction::Play});
                                audio.push(events.view());
                            }
                            ordering_page = 1;
                            frontend = FrontEndMode::Ordering;
                            break;
                        case 3: // HIGH SCORES -> raw state 8; LowBees remains owned.
                            frontend = FrontEndMode::HighScores;
                            break;
                        case 4: // CONFIGURE JOYSTICK -> reconstructed host control editor; LowBees remains owned.
                            control_selection = 0;
                            control_waiting_for_key = false;
                            controls.status.clear();
                            frontend = FrontEndMode::ConfigureJoystick;
                            break;
                        case 5: // VIDEO SETTINGS -> host/remaster preferences.
                            video_selection = 0;
                            video.status.clear();
                            frontend = FrontEndMode::VideoSettings;
                            break;
                        case 6: // PLAY DEMO -> raw state 13, then original four-demo selector.
                            audio.push(audio::leave_original_main_menu_audio(menu_audio, 13).view());
                            audio.push(audio::original_main_menu_air_restart(session.original_audio, 13).view());
                            begin_next_original_demo(demo_replay, assets, session, world);
                            effects = EffectRuntime{};
                            hud_timers = fidelity::SpecialWeaponHudTimers{};
                            last_tick = gameplay::GameSessionTickResult{};
                            drone_outcome_cursor_y = fidelity::drone_outcome_cursor_initial_y;
                            paused = false;
                            quit_confirm = false;
                            weapon_help_visible = false;
                            weapon_help_pinned = false;
                            weapon_help_ticks_remaining = 0;
                            objective_assist = ObjectiveAssistState{};
                            drone_failure_cause = DroneFailureCause::Unknown;
                            mission_surveillance_valid = false;
                            mission_surveillance_capture_pending = true;
                            frontend = FrontEndMode::Gameplay;
                            next_tick = std::chrono::steady_clock::now();
                            break;
                        case 7: // EXIT DRONE -> raw state 0.
                            audio.push(audio::leave_original_main_menu_audio(menu_audio, 0).view());
                            running = false;
                            break;
                        }
                    }
                    break;
                case FrontEndMode::Difficulty:
                    if (up_edge || left_edge) {
                        auto value = static_cast<int>(session.runtime.difficulty) - 1;
                        if (value < 1) value = 3;
                        session.runtime.difficulty = static_cast<gameplay::DifficultyLevel>(value);
                    }
                    if (down_edge || right_edge) {
                        auto value = static_cast<int>(session.runtime.difficulty) + 1;
                        if (value > 3) value = 1;
                        session.runtime.difficulty = static_cast<gameplay::DifficultyLevel>(value);
                    }
                    if (escape_edge) {
                        audio.push(audio::begin_original_main_menu_audio(menu_audio).view());
                        frontend = FrontEndMode::MainMenu;
                    } else if (enter_edge) {
                        session.runtime.demo_playback_mode = false;
                        demo_replay.clear();
                        gameplay::reset_game_session(session, gameplay::GameplaySessionResetScope::FullCampaign);
                        world = load_initial_river_world(assets);
                        effects = EffectRuntime{};
                        hud_timers = fidelity::SpecialWeaponHudTimers{};
                        last_tick = gameplay::GameSessionTickResult{};
                        drone_outcome_cursor_y = fidelity::drone_outcome_cursor_initial_y;
                        paused = false;
                        quit_confirm = false;
                        weapon_help_visible = true;
                        weapon_help_pinned = false;
                        weapon_help_ticks_remaining = kStartupWeaponHelpTicks;
                        objective_assist = ObjectiveAssistState{};
                        drone_failure_cause = DroneFailureCause::Unknown;
                        mission_surveillance_valid = false;
                        mission_surveillance_capture_pending = true;
                        frontend = FrontEndMode::Gameplay;
                        next_tick = std::chrono::steady_clock::now();
                    }
                    break;
                case FrontEndMode::Instructions:
                    if ((up_edge || left_edge) && instructions_page > 1) --instructions_page;
                    if (down_edge || right_edge || enter_edge) {
                        if (instructions_page < 9) ++instructions_page;
                        else frontend = FrontEndMode::MainMenu;
                    }
                    if (escape_edge) frontend = FrontEndMode::MainMenu;
                    break;
                case FrontEndMode::Ordering:
                    if (left_edge && ordering_page > 1) --ordering_page;
                    if (enter_edge || right_edge) {
                        if (ordering_page < 5 && assets.exists(std::string("ORDER") + std::to_string(ordering_page + 1) + ".JBA")) {
                            ++ordering_page;
                        } else {
                            drone::audio::AudioEventQueue events{};
                            (void)events.push({drone::audio::AudioCue::OrderingInformation,
                                               drone::audio::AudioAction::StopAndRewind});
                            audio.push(events.view());
                            audio.push(audio::begin_original_main_menu_audio(menu_audio).view());
                            frontend = FrontEndMode::MainMenu;
                        }
                    }
                    if (escape_edge) {
                        drone::audio::AudioEventQueue events{};
                        (void)events.push({drone::audio::AudioCue::OrderingInformation,
                                           drone::audio::AudioAction::StopAndRewind});
                        audio.push(events.view());
                        audio.push(audio::begin_original_main_menu_audio(menu_audio).view());
                        frontend = FrontEndMode::MainMenu;
                    }
                    break;
                case FrontEndMode::HighScores:
                    if (escape_edge || enter_edge) frontend = FrontEndMode::MainMenu;
                    break;
                case FrontEndMode::ConfigureJoystick:
                    if (control_was_waiting_for_key) {
                        if (escape_edge) {
                            control_waiting_for_key = false;
                            controls.status = "REBIND CANCELLED";
                        } else if (newly_pressed) {
                            if (controls.assign(static_cast<HostControlAction>(control_selection), *newly_pressed)) {
                                control_waiting_for_key = false;
                            }
                        }
                    } else {
                        if (up_edge) {
                            control_selection = (control_selection + static_cast<int>(kHostControlDefinitions.size()) - 1) %
                                                static_cast<int>(kHostControlDefinitions.size());
                            controls.status.clear();
                        }
                        if (down_edge) {
                            control_selection = (control_selection + 1) % static_cast<int>(kHostControlDefinitions.size());
                            controls.status.clear();
                        }
                        if (newly_pressed && *newly_pressed == XK_BackSpace) {
                            controls.restore_default(static_cast<HostControlAction>(control_selection));
                        } else if (newly_pressed && (*newly_pressed == XK_d || *newly_pressed == XK_D)) {
                            controls.restore_defaults();
                        } else if (escape_edge) {
                            frontend = FrontEndMode::MainMenu;
                        } else if (enter_edge) {
                            control_waiting_for_key = true;
                            controls.status.clear();
                        }
                    }
                    break;
                case FrontEndMode::VideoSettings: {
                    constexpr int rows = 4;
                    if (up_edge) { video_selection = (video_selection + rows - 1) % rows; video.status.clear(); }
                    if (down_edge) { video_selection = (video_selection + 1) % rows; video.status.clear(); }
                    const bool change_left = left_edge;
                    const bool change_right = right_edge || enter_edge;
                    if (video_selection == 0 && (change_left || change_right)) {
                        const bool wanted = !x11.hd_enabled;
                        if (wanted && !hd_assets.available) {
                            video.status = "HD ASSETS NOT AVAILABLE";
                        } else {
                            hd_changed |= x11.set_hd_enabled(wanted);
                            video.prefer_hd = x11.hd_enabled;
                            video.status = video.save() ? "ART MODE SAVED" : "ART MODE ACTIVE - SAVE FAILED";
                        }
                    } else if (video_selection == 1 && (change_left || change_right)) {
                        const int fit = x11.maximum_fitting_scale();
                        int mode = std::clamp(video.scale_mode, 0, fit);
                        if (change_right) mode = (mode + 1) % (fit + 1);
                        else mode = (mode + fit) % (fit + 1);
                        video.scale_mode = mode;
                        const int target_scale = mode == 0 ? fit : mode;
                        scale_changed |= x11.set_scale(target_scale);
                        video.status = video.save() ? "WINDOW SCALE SAVED" : "SCALE ACTIVE - SAVE FAILED";
                    } else if (video_selection == 2 && (change_left || change_right)) {
                        video.filter = video.filter == HdFilterMode::Smooth ? HdFilterMode::Sharp : HdFilterMode::Smooth;
                        hd_changed |= x11.set_hd_filter(video.filter);
                        video.status = video.save() ? "HD FILTER SAVED" : "FILTER ACTIVE - SAVE FAILED";
                    } else if (video_selection == 3 && enter_edge) {
                        video.restore_defaults(false);
                        video.prefer_hd = hd_assets.available;
                        video.scale_mode = 0;
                        video.filter = HdFilterMode::Smooth;
                        hd_changed |= x11.set_hd_enabled(video.prefer_hd);
                        hd_changed |= x11.set_hd_filter(video.filter);
                        scale_changed |= x11.set_scale(x11.maximum_fitting_scale());
                        video.status = video.save() ? "VIDEO DEFAULTS RESTORED" : "DEFAULTS ACTIVE - SAVE FAILED";
                    }
                    if (escape_edge) frontend = FrontEndMode::MainMenu;
                    break;
                }
                case FrontEndMode::Gameplay:
                    break;
                }
            } else if (interstitial.active && enter_edge &&
                       interstitial.confirm_lock_remaining == 0) {
                interstitial.active = false;
                next_tick = std::chrono::steady_clock::now();
            }

            if (frontend == FrontEndMode::Gameplay) {
                if (p && !prev_p && !interstitial.active && session.post_game.phase == gameplay::PostGameModalPhase::Inactive && !paused && !quit_confirm) {
                    paused = true;
                }
                if (q_edge && !interstitial.active && session.post_game.phase == gameplay::PostGameModalPhase::Inactive && !paused && !quit_confirm) {
                    quit_confirm = true;
                }
                if (y_edge && quit_confirm) {
                    running = false;
                }
                if (r_edge) {
                    if (paused) {
                        paused = false;
                        audio.push(audio::resume_original_gameplay_overlay_audio(session.original_audio).view());
                    } else if (quit_confirm) {
                        quit_confirm = false;
                        audio.push(audio::resume_original_gameplay_overlay_audio(session.original_audio).view());
                    }
                }
            }

            prev_p = p;
            prev_q = q;
            prev_r = r;
            prev_y = y;
            prev_escape = escape;
            prev_f1 = f1;
            prev_f2 = f2;
            prev_f3 = f3;
            prev_f4 = f4;
            prev_f5 = f5;
            prev_f6 = f6;
            prev_enter = enter;
            prev_up = up;
            prev_down = down;
            prev_left = left;
            prev_right = right;
            previous_keys = keys;

            const bool rapid_fire_down = controls.down(keys, HostControlAction::RapidFire);
            if (objective_assist.fire_release_required && !rapid_fire_down) {
                objective_assist.fire_release_required = false;
            }

            auto now = std::chrono::steady_clock::now();
            int catchup = 0;
            while (now >= next_tick && catchup < 5) {
                if (frontend != FrontEndMode::Gameplay) {
                    if (menu_audio.lowbees_owned) {
                        audio.push(audio::tick_original_main_menu_audio(menu_audio).view());
                    }
                } else if (interstitial.active) {
                    // Synchronous original presentation: gameplay does not advance.
                    // 0x0041DA6A starts a 0x3A (=58) presentation/fade lock and
                    // does not poll confirmation until it reaches zero.
                    if (interstitial.confirm_lock_remaining > 0) {
                        --interstitial.confirm_lock_remaining;
                    }
                } else if (session.post_game.phase != gameplay::PostGameModalPhase::Inactive &&
                           session.post_game.phase != gameplay::PostGameModalPhase::Complete) {
                    gameplay::PostGameModalInput modal{};
                    modal.results_presentation_advanced =
                        session.post_game.phase == gameplay::PostGameModalPhase::ResultsConfirmLock;
                    modal.confirm_pressed =
                        session.post_game.phase == gameplay::PostGameModalPhase::ResultsAwaitConfirmation && enter_edge;
                    if (session.post_game.phase == gameplay::PostGameModalPhase::OrderingInformation && enter_edge) {
                        if (ordering_page < 5 && assets.exists(std::string("ORDER") + std::to_string(ordering_page + 1) + ".JBA")) {
                            ++ordering_page;
                        } else {
                            modal.ordering_information_finished = true;
                        }
                    }
                    modal.high_score_table_finished =
                        session.post_game.phase == gameplay::PostGameModalPhase::HighScoreTable && enter_edge;
                    modal.completion_credits_finished =
                        session.post_game.phase == gameplay::PostGameModalPhase::CompletionCredits && enter_edge;
                    const auto pg = gameplay::step_game_session_post_game(session, modal);
                    audio.push(pg.audio_events.view());
                    if (pg.ordering_information_started) ordering_page = 1;
                    if (pg.completed) {
                        frontend = FrontEndMode::MainMenu;
                        main_menu_selection = 0;
                        audio.push(audio::begin_original_main_menu_audio(menu_audio).view());
                    }
                } else if (paused || quit_confirm) {
                    audio.push(audio::tick_original_gameplay_overlay_audio(session.original_audio).view());
                } else {
                    gameplay::GameplayInputFrame input{};
                    input.movement.left = controls.down(keys, HostControlAction::MoveLeft);
                    input.movement.right = controls.down(keys, HostControlAction::MoveRight);
                    input.movement.up = controls.down(keys, HostControlAction::MoveUp);
                    input.movement.down = controls.down(keys, HostControlAction::MoveDown);
                    input.shield = controls.down(keys, HostControlAction::Shield);
                    input.special_load_cycle = controls.down(keys, HostControlAction::SpecialLoad);
                    input.special_launch = controls.down(keys, HostControlAction::SpecialLaunch);

                    const bool objective_visible_before_step =
                        session.encounter.drone.activity == gameplay::canonical_drone_active_activity &&
                        !session.encounter.drone.disarm_completed &&
                        session.encounter.drone.y >= -39 &&
                        session.encounter.drone.y <= gameplay::canonical_drone_hover_y;
                    const bool probe_seeking_before_step =
                        session.encounter.special_weapon.activity == gameplay::SpecialWeaponActivity::LaunchedHoming &&
                        session.encounter.special_weapon.kind == gameplay::SpecialWeaponKind::Probe;
                    const bool probe_attached_before_step =
                        session.encounter.special_weapon.activity == gameplay::SpecialWeaponActivity::ProbeAttachedDecoding;

                    // Remaster-side objective safety.  The deterministic core
                    // still preserves the original ability to detonate a DRONE.
                    // Safety ON only filters host input around a live blue-Probe
                    // disarm attempt and can be disabled instantly with F5.
                    if (objective_safety_enabled && objective_visible_before_step &&
                        session.encounter.special_weapon.activity == gameplay::SpecialWeaponActivity::Inactive) {
                        // The original Down-load action preserves the previous
                        // selection.  For a visible DRONE objective, make the host
                        // choose the documented blue Probe so "Down then Up" is
                        // deterministic instead of silently reloading a Stinger.
                        session.encounter.special_weapon.kind = gameplay::SpecialWeaponKind::Probe;
                    }

                    const bool launching_probe_this_tick =
                        objective_safety_enabled && objective_visible_before_step &&
                        input.special_launch &&
                        session.encounter.special_weapon.activity == gameplay::SpecialWeaponActivity::LoadedTracking &&
                        session.encounter.special_weapon.kind == gameplay::SpecialWeaponKind::Probe;
                    if (launching_probe_this_tick) {
                        for (std::size_t i = 0; i < session.encounter.rapid_missiles.missiles.size(); ++i) {
                            (void)gameplay::deactivate_rapid_missile(session.encounter.rapid_missiles, i);
                        }
                        objective_assist.fire_release_required = rapid_fire_down;
                    }

                    const bool protect_probe_attempt =
                        objective_safety_enabled &&
                        (probe_attached_before_step ||
                         (objective_visible_before_step &&
                          (launching_probe_this_tick || probe_seeking_before_step)));
                    input.rapid_fire = rapid_fire_down &&
                        !objective_assist.fire_release_required && !protect_probe_attempt;

                    if (objective_safety_enabled && objective_visible_before_step &&
                        input.special_launch &&
                        session.encounter.special_weapon.activity == gameplay::SpecialWeaponActivity::LoadedTracking &&
                        session.encounter.special_weapon.kind == gameplay::SpecialWeaponKind::Stinger) {
                        input.special_launch = false;
                        objective_assist.blocked_stinger_ticks_remaining = 210;
                    }

                    bool demo_terminal_after_step = false;
                    if (session.runtime.demo_playback_mode && demo_replay.active()) {
                        (void)demo_replay.timeline.advance_gameplay_update();
                        const auto record_index = demo_replay.timeline.record_index();
                        if (record_index < demo_replay.frames.size()) {
                            const auto demo_frame = gameplay::build_demo_gameplay_frame(
                                demo_replay.frames[record_index]);
                            input = gameplay::apply_demo_playback_input(input, demo_frame);
                            apply_demo_replay_checkpoints(session, demo_frame, paths.view);
                        }
                        demo_terminal_after_step = demo_replay.timeline.terminal() ||
                            record_index >= demo_replay.frames.size();
                    }

                    const auto interstitial_alien_hit_before_step =
                        session.encounter.encounter_alien_ships_hit;
                    const auto interstitial_alien_total_before_step =
                        session.encounter.encounter_alien_ships_total;
                    const auto interstitial_score_before_step = session.campaign.score.total;

                    last_tick = gameplay::step_game_session(session, input, targets);

                    // Host-only objective safety assist. The original core remains
                    // unchanged and still allows deliberate rapid-fire/Stinger
                    // destruction of a DRONE. When a blue Probe first attaches,
                    // purge already-launched player rapid missiles and require one
                    // fire-key release before accepting new rapid fire. This removes
                    // the keyboard race where a successful attachment is followed by
                    // an older missile arriving a few frames later.
                    if (last_tick.probe_attached_to_drone && !session.runtime.demo_playback_mode) {
                        for (std::size_t i = 0; i < session.encounter.rapid_missiles.missiles.size(); ++i) {
                            (void)gameplay::deactivate_rapid_missile(session.encounter.rapid_missiles, i);
                        }
                        objective_assist.fire_release_required = rapid_fire_down;
                        objective_assist.probe_lost_ticks_remaining = 0;
                        drone_failure_cause = DroneFailureCause::Unknown;
                    }
                    if (last_tick.enemy_bomb_probe_impact_effect_requested &&
                        !session.runtime.demo_playback_mode) {
                        objective_assist.probe_lost_ticks_remaining = 210;
                    } else if (objective_assist.probe_lost_ticks_remaining > 0) {
                        --objective_assist.probe_lost_ticks_remaining;
                    }
                    if (last_tick.probe_decode_completed) {
                        objective_assist.fire_release_required = false;
                        objective_assist.probe_lost_ticks_remaining = 0;
                    }
                    if (objective_assist.safety_notice_ticks_remaining > 0) {
                        --objective_assist.safety_notice_ticks_remaining;
                    }
                    if (objective_assist.blocked_stinger_ticks_remaining > 0) {
                        --objective_assist.blocked_stinger_ticks_remaining;
                    }
                    if (last_tick.rapid_missile_hit_drone) {
                        drone_failure_cause = DroneFailureCause::RapidMissile;
                    } else if (last_tick.stinger_hit_drone) {
                        drone_failure_cause = DroneFailureCause::Stinger;
                    } else if (last_tick.drone_hover_timeout_reached) {
                        drone_failure_cause = DroneFailureCause::HoverTimeout;
                    }

                    if (last_tick.special_launched && !session.runtime.demo_playback_mode) {
                        weapon_help_visible = false;
                        weapon_help_pinned = false;
                        weapon_help_ticks_remaining = 0;
                    } else if (weapon_help_visible && !weapon_help_pinned &&
                               weapon_help_ticks_remaining > 0) {
                        --weapon_help_ticks_remaining;
                        if (weapon_help_ticks_remaining == 0) {
                            weapon_help_visible = false;
                        }
                    }
                    const auto cursor_plan = fidelity::plan_drone_outcome_cursor(
                        static_cast<std::uint8_t>(session.campaign.mission.processed_count), drone_outcome_cursor_y);
                    fidelity::advance_drone_outcome_cursor_y(
                        drone_outcome_cursor_y, cursor_plan.target_y,
                        static_cast<std::uint8_t>(last_tick.gameplay_substep_phase));
                    audio.push(last_tick.audio_events.view());
                    update_effects(effects, session, last_tick);
                    if (last_tick.encounter_transition) {
                        apply_scenery_transition(world, assets, last_tick.encounter_transition->scenery);
                    }
                    if (last_tick.mission_interstitial) {
                        interstitial = MissionInterstitialUi{};
                        interstitial.active = true;
                        interstitial.outcome_asset = mission_outcome_asset(*last_tick.mission_interstitial);
                        interstitial.mission_asset = mission_briefing_asset(*last_tick.mission_interstitial);
                        interstitial.surveillance = mission_surveillance;
                        interstitial.surveillance_valid = mission_surveillance_valid;
                        interstitial.alien_ships_hit = interstitial_alien_hit_before_step;
                        interstitial.alien_ships_total = interstitial_alien_total_before_step;
                        interstitial.score = interstitial_score_before_step;
                        interstitial.confirm_lock_remaining =
                            MissionInterstitialUi::confirm_lock_presentations;
                        if (last_tick.mission_interstitial->tone == gameplay::MissionInterstitialTone::Bad) {
                            interstitial.failure_detail = std::string(drone_failure_cause_text(drone_failure_cause));
                        }
                        // The reset performed inside the transition has already
                        // staged the next encounter. Capture its surveillance
                        // image after this interstitial is dismissed.
                        mission_surveillance_capture_pending = true;
                    }
                    if (demo_terminal_after_step) {
                        // Original demo terminal path suppresses the ordinary post-game
                        // presentation and returns to front-end ownership.
                        session.campaign.suppress_results_and_ordering = true;
                        session.runtime.demo_playback_mode = false;
                        demo_replay.clear();
                        interstitial = MissionInterstitialUi{};
                        paused = false;
                        quit_confirm = false;
                        frontend = FrontEndMode::MainMenu;
                        main_menu_selection = 0;
                        audio.push(audio::begin_original_main_menu_audio(menu_audio).view());
                    }
                }
                next_tick += std::chrono::duration_cast<std::chrono::steady_clock::duration>(kTickDuration);
                ++catchup;
                now = std::chrono::steady_clock::now();
            }
            if (catchup == 5 && now >= next_tick) next_tick = now;

            // Present only after one or more logical/UI ticks.  The old dev host
            // submitted identical XImages every ~2 ms between 70 Hz simulation
            // updates, wasting CPU and introducing compositor pacing noise.
            if (catchup != 0 || scale_changed || hd_changed) {
                hd_plan.reset();
                const GameplayControlLegend control_legend{
                    .main_fire = controls.display(HostControlAction::RapidFire),
                    .shield = controls.display(HostControlAction::Shield),
                    .special_select = controls.display(HostControlAction::SpecialLoad),
                    .special_launch = controls.display(HostControlAction::SpecialLaunch),
                    .resume_cancel = controls.display(HostControlAction::ResumeCancel),
                };
                if (frontend != FrontEndMode::Gameplay) {
                    render_frontend(framebuffer, &hd_plan, assets, font, session, controls, frontend,
                                    main_menu_selection, instructions_page, ordering_page,
                                    control_selection, control_waiting_for_key, x11.scale,
                                    video, VideoSettingsView{
                                        .hd_available = hd_assets.available,
                                        .hd_enabled = x11.hd_enabled,
                                        .active_scale = x11.scale,
                                        .maximum_scale = x11.maximum_fitting_scale(),
                                        .hd_asset_count = hd_assets.png_file_count,
                                        .last_background_hd = x11.last_hd_background,
                                        .last_sprite_hits = x11.last_hd_sprite_hits,
                                        .last_sprite_misses = x11.last_hd_sprite_misses,
                                    }, video_selection);
                } else if (interstitial.active) {
                    if (assets.exists(interstitial.mission_asset) && assets.exists(interstitial.outcome_asset)) {
                        render_mission_interstitial(framebuffer, &hd_plan, assets, font, interstitial);
                    } else render_game(framebuffer, &hd_plan, world, session, last_tick, trajectory_sprites, boss_sprites,
                                     play_assets, font, effects, hud_timers, drone_outcome_cursor_y, false, false, debug_hud,
                                     control_legend, weapon_help_visible, objective_safety_enabled, objective_assist);
                } else if (session.post_game.phase != gameplay::PostGameModalPhase::Inactive &&
                           session.post_game.phase != gameplay::PostGameModalPhase::Complete) {
                    render_post_game(framebuffer, &hd_plan, assets, session, font, ordering_page);
                } else {
                    render_game(framebuffer, &hd_plan, world, session, last_tick, trajectory_sprites, boss_sprites,
                                play_assets, font, effects, hud_timers, drone_outcome_cursor_y, paused, quit_confirm, debug_hud,
                                control_legend, weapon_help_visible, objective_safety_enabled, objective_assist);
                    if (mission_surveillance_capture_pending && !paused && !quit_confirm) {
                        capture_mission_surveillance(framebuffer, mission_surveillance);
                        mission_surveillance_valid = true;
                        mission_surveillance_capture_pending = false;
                    }
                }
                x11.present(framebuffer, &hd_plan);
            }

        }
        audio.stop();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "drone_playable_host: " << e.what() << '\n';
        return 1;
    }
}
