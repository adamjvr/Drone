# Platform Plan

## Target platforms

The published remaster target set is:

- Linux;
- macOS;
- iPadOS;
- Windows.

All platforms should execute the same `drone_core` simulation. Platform-specific code is a host/adaptation layer, not a fork of gameplay logic.

## Shared host responsibilities

The first modern host should provide:

- app/window/surface lifecycle;
- canonical keyboard/gamepad/touch action state;
- presentation of the 320×200 indexed fidelity framebuffer;
- palette updates;
- audio event/sample playback;
- monotonic clock access for the scheduler;
- filesystem/config paths;
- logging and trace capture.

Phase 2 has implemented the first fidelity host as thin native backends around a shared indexed-framebuffer contract:

- Linux: X11;
- Windows: Win32/GDI;
- macOS: Cocoa/CoreGraphics.

This is not a permanent ban on a future shared host library. It is an intentionally small proof that host code can remain outside `drone_core` and that the original 320×200 indexed presentation can be exercised without committing the reconstruction to a heavyweight framework.

## Linux

The Phase 2 fidelity host currently targets X11 directly and builds through CMake/Ninja. Wayland-native presentation can be added later or supplied through a future host abstraction; it is not allowed to change simulation behavior. Packaging is deferred until behavioral parity is farther along.

## macOS

The Phase 2 tree contains a Cocoa/CoreGraphics fidelity-host backend intended for native macOS builds. Apple Silicon is the primary target; x86_64 support is optional if it remains useful. Signing/notarization and app-bundle polish belong to release hardening, not the reconstruction core.

## Windows

The remaster is a new native executable. The Phase 2 tree contains a Win32/GDI fidelity-host backend; it intentionally does not preserve DirectDraw/DirectInput/DirectSound APIs because those original APIs are evidence for contracts rather than compatibility requirements. Visual Studio and/or Ninja-based CMake builds should remain supported.

## iPadOS

The iPad build should use the same simulation/core and asset interfaces as desktop. Touch controls become another mapping into canonical game actions. iPadOS-specific lifecycle, sandbox paths, audio interruptions, display orientation, and controller integration belong to the host layer.

The iPad port should begin after the desktop host proves the architecture, not by cloning gameplay code into an Apple-specific application.

## Scheduler

The host scheduler must not assume a modern display refresh rate equals the original simulation rate. Once the original cadence is proven, the host should run simulation at that cadence and present independently where possible.

## Build-system direction

CMake remains the cross-platform source of truth for core/tool/test targets. Platform packaging projects or generated Xcode/Visual Studio metadata may wrap it, but should not become the only definition of source membership or behavior.

## Release hardening gates

Before calling a platform production-ready:

- fidelity tests pass;
- input mapping is complete;
- suspend/resume/focus behavior is defined;
- audio lifecycle is robust;
- filesystem/config paths are platform-correct;
- controller handling is tested;
- packaging/signing requirements are documented;
- no original proprietary assets are accidentally embedded unless distribution rights have been explicitly resolved.
