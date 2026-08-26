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

SDL3 is the leading Phase 2 host candidate because it can cover Linux, macOS, Windows, and iOS/iPadOS families while keeping the game core framework-independent. It is intentionally not embedded in the core library.

## Linux

Initial development target for the portable desktop host should support common Wayland/X11 environments through the selected host library. Packaging is deferred until behavioral parity is farther along; early deliverables should at least provide a normal CMake/Ninja build.

## macOS

The same desktop host should build natively for Apple Silicon and, where practical, x86_64 only if maintaining that target remains useful. Signing/notarization and app-bundle polish belong to release hardening, not the reconstruction core.

## Windows

The remaster is a new native executable. It does not need to preserve DirectDraw/DirectInput/DirectSound APIs; those original APIs are evidence for contracts. Visual Studio and/or Ninja-based CMake builds should remain supported.

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
