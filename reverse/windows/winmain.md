# Win32 startup / main-loop reconstruction

Primary routine: `0x00404E30`.

High-confidence flow:

```text
RegisterClassExA(window_proc = 0x404DC0)
CreateWindowExA(...)
ShowCursor(...)
directx_platform_init(hwnd) // 0x404B60; DirectDraw 320x200x8 + input + sound
subsystem_init              // 0x407B30
loop:
    while PeekMessageA(...):
        if WM_QUIT -> shutdown
        TranslateMessage
        DispatchMessageA
    lock/acquire DirectDraw surface
    update globals with framebuffer pointer + pitch
    game_dispatch_update()  // 0x40BA50
    release/present DirectDraw surface
    goto loop
shutdown:
    ddraw_shutdown()         // 0x404D60
    game_shutdown()          // 0x40B380
```

The surface descriptor size written before the lock path is `0x6C` (108), consistent with the older DirectDraw `DDSURFACEDESC` family.
