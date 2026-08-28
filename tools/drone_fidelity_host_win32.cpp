#include <drone/fidelity/indexed_framebuffer.hpp>
#include <drone/formats/jba.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint32_t> g_pixels;
int g_scale = 3;

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd, &ps);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = 320;
        info.bmiHeader.biHeight = -200;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        SetStretchBltMode(dc, COLORONCOLOR);
        StretchDIBits(dc, 0, 0, 320 * g_scale, 200 * g_scale,
                      0, 0, 320, 200, g_pixels.data(), &info, DIB_RGB_COLORS, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE || wparam == 'Q') DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}
} // namespace

int main(int argc, char** argv) try {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: drone_fidelity_host <image-or-sheet.jba> [integer-scale]\n";
        return 2;
    }
    if (argc == 3) {
        g_scale = std::stoi(argv[2]);
        if (g_scale < 1 || g_scale > 8) throw std::runtime_error("scale must be between 1 and 8");
    }

    drone::fidelity::IndexedFramebuffer framebuffer;
    framebuffer.load(drone::formats::load_jba_320x200(argv[1]));
    g_pixels.resize(framebuffer.pixel_count);
    for (std::size_t i = 0; i < framebuffer.pixel_count; ++i) {
        const auto& c = framebuffer.palette()[framebuffer.pixels()[i]];
        g_pixels[i] = static_cast<std::uint32_t>(c.b) |
                      (static_cast<std::uint32_t>(c.g) << 8) |
                      (static_cast<std::uint32_t>(c.r) << 16);
    }

    HINSTANCE instance = GetModuleHandle(nullptr);
    const wchar_t* klass = L"DroneFidelityHost";
    WNDCLASSW wc{};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = instance;
    wc.lpszClassName = klass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    RECT rect{0, 0, 320 * g_scale, 200 * g_scale};
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    HWND hwnd = CreateWindowExW(0, klass, L"Drone Fidelity Host",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
                                nullptr, nullptr, instance, nullptr);
    if (!hwnd) throw std::runtime_error("CreateWindowEx failed");
    ShowWindow(hwnd, SW_SHOW);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
} catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
}
