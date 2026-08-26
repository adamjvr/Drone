# Ghidra Jython script: apply Phase-1 labels to the 1999 Win32 shareware executable.
# Run only after importing the binary at its normal image base 0x00400000.

from ghidra.program.model.symbol import SourceType

labels = {
    0x004012B0: "load_fullscreen_jba",
    0x004011E0: "set_palette_range",
    0x00404B60: "directx_platform_init",
    0x00406650: "directsound_init",
    0x00404D60: "ddraw_shutdown",
    0x00404DC0: "drone_window_proc",
    0x00404E30: "drone_winmain_like",
    0x00406200: "load_wav",
    0x004068E0: "directinput_create",
    0x0040B380: "game_shutdown",
    0x0040BA50: "game_dispatch_update",
    0x00421CD0: "crt_fopen_like",
    0x00421D20: "crt_free_like",
    0x00421D70: "crt_malloc_like",
    0x00421F00: "crt_fscanf_like",
}
globals_ = {
    0x0042B188: "game_state_raw",
    0x0042B1B4: "frame_limiter_enabled",
    0x00440DF8: "qpc_buffer_ptr",
    0x004677E8: "previous_qpc_low",
    0x0045A140: "current_qpc_low",
    0x0048003C: "qpc_delta_low",
    0x004D9580: "main_hwnd",
    0x004D9584: "surface_pixels",
    0x004DA780: "surface_pitch",
}
for addr,name in labels.items():
    a=toAddr(addr); f=getFunctionAt(a)
    if f is None: f=createFunction(a,name)
    else: f.setName(name, SourceType.USER_DEFINED)
for addr,name in globals_.items():
    createLabel(toAddr(addr), name, True)
print("Applied Drone Phase-1 labels")
