#include <drone/fidelity/framebuffer_snapshot.hpp>
#include <drone/fidelity/host_capture.hpp>
#include <drone/fidelity/indexed_framebuffer.hpp>
#include <drone/formats/jba.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

struct Options {
    std::filesystem::path input;
    int scale{3};
    bool headless{};
    std::optional<std::filesystem::path> capture_path;
    std::optional<std::filesystem::path> capture_directory;
    std::string landmark{"initial-frame"};
    std::uint64_t sequence{};
};

unsigned long component_to_mask(const std::uint8_t value, const unsigned long mask) {
    if (mask == 0) return 0;
    unsigned shift = 0;
    while (((mask >> shift) & 1UL) == 0UL) ++shift;
    const auto shifted_mask = mask >> shift;
    const unsigned long scaled = (static_cast<unsigned long>(value) * shifted_mask + 127UL) / 255UL;
    return (scaled << shift) & mask;
}

unsigned long pixel_value(const Visual* visual, const drone::formats::Rgb8& rgb) {
    return component_to_mask(rgb.r, visual->red_mask) |
           component_to_mask(rgb.g, visual->green_mask) |
           component_to_mask(rgb.b, visual->blue_mask);
}

[[noreturn]] void usage_error(const std::string& message) {
    throw std::runtime_error(
        message +
        "\nUsage: drone_fidelity_host <image.jba|snapshot.drfb> [integer-scale] "
        "[--scale N] [--capture FILE | --capture-dir DIR] "
        "[--landmark NAME] [--sequence N] [--headless]");
}

int parse_scale(const std::string& text) {
    const int scale = std::stoi(text);
    if (scale < 1 || scale > 8) usage_error("scale must be between 1 and 8");
    return scale;
}

Options parse_options(int argc, char** argv) {
    if (argc < 2) usage_error("input image/snapshot is required");
    Options options;
    options.input = argv[1];
    bool legacy_scale_consumed = false;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const char* name) -> std::string {
            if (++i >= argc) usage_error(std::string(name) + " requires a value");
            return argv[i];
        };
        if (arg == "--scale") {
            options.scale = parse_scale(require_value("--scale"));
        } else if (arg == "--capture") {
            options.capture_path = require_value("--capture");
        } else if (arg == "--capture-dir") {
            options.capture_directory = require_value("--capture-dir");
        } else if (arg == "--landmark") {
            options.landmark = require_value("--landmark");
        } else if (arg == "--sequence") {
            options.sequence = std::stoull(require_value("--sequence"));
        } else if (arg == "--headless") {
            options.headless = true;
        } else if (!legacy_scale_consumed && !arg.empty() && arg.front() != '-') {
            options.scale = parse_scale(arg);
            legacy_scale_consumed = true;
        } else {
            usage_error("unknown argument: " + arg);
        }
    }
    if (options.capture_path && options.capture_directory) {
        usage_error("--capture and --capture-dir are mutually exclusive");
    }
    if (options.headless && !options.capture_path && !options.capture_directory) {
        usage_error("--headless requires --capture or --capture-dir");
    }
    return options;
}

bool has_drfb_magic(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("unable to open fidelity-host input: " + path.string());
    std::array<char, 8> magic{};
    in.read(magic.data(), magic.size());
    return in.gcount() == static_cast<std::streamsize>(magic.size()) &&
        magic == drone::fidelity::framebuffer_snapshot_magic;
}

drone::fidelity::IndexedFramebuffer load_input(const std::filesystem::path& path) {
    if (has_drfb_magic(path)) {
        return drone::fidelity::make_indexed_framebuffer(
            drone::fidelity::load_framebuffer_snapshot(path));
    }
    drone::fidelity::IndexedFramebuffer framebuffer;
    framebuffer.load(drone::formats::load_jba_320x200(path));
    return framebuffer;
}

void maybe_capture(
    const Options& options,
    const drone::fidelity::IndexedFramebuffer& framebuffer) {
    if (options.capture_path) {
        drone::fidelity::write_fidelity_host_capture(framebuffer, *options.capture_path);
        std::cout << "captured " << options.capture_path->string() << '\n';
    } else if (options.capture_directory) {
        const auto path = drone::fidelity::write_fidelity_host_landmark_capture(
            framebuffer,
            *options.capture_directory,
            {.label = options.landmark, .sequence = options.sequence});
        std::cout << "captured " << path.string() << '\n';
    }
}

} // namespace

int main(int argc, char** argv) try {
    const Options options = parse_options(argc, argv);
    auto framebuffer = load_input(options.input);
    maybe_capture(options, framebuffer);
    if (options.headless) return 0;

    Display* display = XOpenDisplay(nullptr);
    if (!display) throw std::runtime_error("XOpenDisplay failed; use --headless for capture-only validation");

    const int screen = DefaultScreen(display);
    Visual* visual = DefaultVisual(display, screen);
    const int depth = DefaultDepth(display, screen);
    const int window_width = static_cast<int>(drone::fidelity::IndexedFramebuffer::width) * options.scale;
    const int window_height = static_cast<int>(drone::fidelity::IndexedFramebuffer::height) * options.scale;

    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen), 0, 0,
        static_cast<unsigned>(window_width), static_cast<unsigned>(window_height), 0,
        BlackPixel(display, screen), BlackPixel(display, screen));
    XStoreName(display, window, "Drone Fidelity Host");
    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask);

    XSizeHints hints{};
    hints.flags = PMinSize | PMaxSize;
    hints.min_width = hints.max_width = window_width;
    hints.min_height = hints.max_height = window_height;
    XSetWMNormalHints(display, window, &hints);

    Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete, 1);
    XMapWindow(display, window);

    XImage* image = XCreateImage(
        display, visual, static_cast<unsigned>(depth), ZPixmap, 0, nullptr,
        static_cast<unsigned>(window_width), static_cast<unsigned>(window_height), 32, 0);
    if (!image) {
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        throw std::runtime_error("XCreateImage failed");
    }
    image->data = static_cast<char*>(std::calloc(static_cast<std::size_t>(image->bytes_per_line), window_height));
    if (!image->data) {
        XDestroyImage(image);
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        throw std::bad_alloc();
    }

    const auto& pixels = framebuffer.pixels();
    const auto& palette = framebuffer.palette();
    for (std::size_t y = 0; y < drone::fidelity::IndexedFramebuffer::height; ++y) {
        for (std::size_t x = 0; x < drone::fidelity::IndexedFramebuffer::width; ++x) {
            const auto packed = pixel_value(visual, palette[pixels[y * drone::fidelity::IndexedFramebuffer::width + x]]);
            for (int sy = 0; sy < options.scale; ++sy) {
                for (int sx = 0; sx < options.scale; ++sx) {
                    XPutPixel(image,
                              static_cast<int>(x) * options.scale + sx,
                              static_cast<int>(y) * options.scale + sy,
                              packed);
                }
            }
        }
    }

    GC gc = XCreateGC(display, window, 0, nullptr);
    auto present = [&] {
        XPutImage(display, window, gc, image, 0, 0, 0, 0,
                  static_cast<unsigned>(window_width), static_cast<unsigned>(window_height));
        XFlush(display);
    };

    bool running = true;
    while (running) {
        XEvent event{};
        XNextEvent(display, &event);
        switch (event.type) {
        case Expose:
            if (event.xexpose.count == 0) present();
            break;
        case KeyPress: {
            const KeySym sym = XLookupKeysym(&event.xkey, 0);
            if (sym == XK_Escape || sym == XK_q || sym == XK_Q) running = false;
            break;
        }
        case ClientMessage:
            if (static_cast<Atom>(event.xclient.data.l[0]) == wm_delete) running = false;
            break;
        default:
            break;
        }
    }

    XFreeGC(display, gc);
    XDestroyImage(image); // also frees image->data
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
} catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
}
