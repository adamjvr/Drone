#include <drone/fidelity/indexed_framebuffer.hpp>
#include <drone/formats/jba.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

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

int parse_scale(const int argc, char** argv) {
    if (argc < 3) return 3;
    const int scale = std::stoi(argv[2]);
    if (scale < 1 || scale > 8) throw std::runtime_error("scale must be between 1 and 8");
    return scale;
}

} // namespace

int main(int argc, char** argv) try {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: drone_fidelity_host <image-or-sheet.jba> [integer-scale]\n";
        return 2;
    }

    const int scale = parse_scale(argc, argv);
    drone::fidelity::IndexedFramebuffer framebuffer;
    framebuffer.load(drone::formats::load_jba_320x200(argv[1]));

    Display* display = XOpenDisplay(nullptr);
    if (!display) throw std::runtime_error("XOpenDisplay failed; an X11 display is required");

    const int screen = DefaultScreen(display);
    Visual* visual = DefaultVisual(display, screen);
    const int depth = DefaultDepth(display, screen);
    const int window_width = static_cast<int>(drone::fidelity::IndexedFramebuffer::width) * scale;
    const int window_height = static_cast<int>(drone::fidelity::IndexedFramebuffer::height) * scale;

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
            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    XPutPixel(image,
                              static_cast<int>(x) * scale + sx,
                              static_cast<int>(y) * scale + sy,
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
