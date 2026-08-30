# Linux Fidelity Host Validation

Phase 3 promotes the Linux/X11 fidelity host from a visual smoke-test shell to a reproducible capture boundary for the clean 320×200 indexed framebuffer.

## Input contract

The Linux host accepts either:

- a recovered full-screen 320×200 JBA file; or
- a clean `DRONEFB1` snapshot.

`DRONEFB1` input is especially useful for public/CI validation because it requires no original game payload.

## Interactive presentation

Legacy invocation remains valid:

```bash
./build-debug/drone_fidelity_host \
  .reference/work/windows/Sights/Titlesh.jba \
  3
```

Equivalent explicit scale form:

```bash
./build-debug/drone_fidelity_host \
  frame.drfb \
  --scale 3
```

The host converts indexed pixels through the supplied working palette and presents through X11. `Escape` or `Q` exits.

## Headless capture

A validation machine does not need an X server to exercise the host capture boundary:

```bash
./build-debug/drone_fidelity_host \
  clean-frame.drfb \
  --headless \
  --capture /tmp/captured.drfb
```

or use deterministic landmark naming:

```bash
./build-debug/drone_fidelity_host \
  clean-frame.drfb \
  --headless \
  --capture-dir .reference/framebuffers/linux-clean \
  --landmark "demo1 tick 420" \
  --sequence 420
```

This produces:

```text
00000420-demo1-tick-420.drfb
```

Capture labels are normalized to lowercase ASCII-ish filename components; non-alphanumeric runs become a single dash. The sequence is zero-padded to eight decimal digits.

`--headless` requires a capture target. `--capture` and `--capture-dir` are mutually exclusive.

## Capture is observational

The reusable `fidelity/host_capture` module accepts a **const** `IndexedFramebuffer`. Regression coverage snapshots the framebuffer before and after capture and requires exact equality. Capturing a frame therefore must not advance simulation, mutate palette state, alter pixels, or consume random state.

The intended future live-game boundary is:

```text
complete indexed drawing
→ complete working-palette mutation
→ named capture landmark (optional)
→ host pacing
→ host palette upload/conversion
→ host presentation
```

For original Win32 parity the preferred historical reference remains the software indexed framebuffer + working palette immediately before final presentation. Linux capture landmarks are clean-engine observations of the same semantic boundary, not substitutes for original-runtime evidence.

## Capture + metadata in one command

`scripts/capture_linux_fidelity_host.py` runs the host headlessly and immediately fingerprints the result using `framebuffer_fixture.py`:

```bash
python3 scripts/capture_linux_fidelity_host.py \
  ./build-debug/drone_fidelity_host \
  /tmp/clean-probe-120.drfb \
  .reference/framebuffers/linux-clean \
  --landmark "gameplay probe tick 120" \
  --sequence 120 \
  --fixture-id linux-clean-probe-120 \
  --source-build phase3-clean \
  --scenario gameplay-probe \
  --tick 120 \
  --region hud:0:0:320:32 \
  --region world:0:32:320:168 \
  --metadata .reference/framebuffers/linux-clean/probe-120.json
```

The `.drfb` remains local/ignored. The JSON contains hashes and descriptive metadata only.

## Automated Linux gate

When X11 development files are present, CMake registers:

```text
drone_linux_fidelity_host_capture
```

The test deliberately removes `DISPLAY`, feeds the actual Linux host a synthetic public `DRONEFB1` frame, requests a headless landmark capture, and requires the output to be byte-for-byte identical. It then exercises the capture+fingerprint wrapper and verifies its metadata.

This makes Linux fidelity-host validation usable on Cortana and CI without a GUI session.

## Integer display scaling

The playable X11 host keeps the fidelity framebuffer at the canonical 320x200 indexed
resolution and scales only the final host presentation. Scaling is nearest-neighbor by exact
integer replication, so simulation coordinates, collision, palette behavior, and framebuffer
validation remain unchanged.

The host chooses the largest scale from 1x through 8x that fits the current X11 screen with a
small desktop margin. It can be overridden with either:

```sh
DRONE_SCALE=5 ./RUN-DRONE.sh
# or
drone_playable_host assets --scale 5
```

`--scale auto` restores automatic selection. During runtime, F2 decreases the integer scale
and F3 increases it. The X11 image buffer and fixed-size window are rebuilt at the new exact
multiple; no filtered or fractional stretch path is used.

## Runtime control editor

The recovered `CONFIGURE JOYSTICK` main-menu entry now opens the Linux host's
working control editor instead of a presentation-only modal.  The original
seven-item main-menu ordering is preserved; the editor is a host extension for
the reconstructed input layer.

Editable gameplay bindings are:

- move left / right / up / down;
- main fire / rapid missiles;
- shield;
- select/cycle blue Probe or red Stinger;
- launch the selected Probe/Stinger;
- pause;
- quit prompt;
- confirm quit; and
- resume/cancel.

Inside the editor, `Up`/`Down` select an action and `Enter` waits for a new key.
`Backspace` restores the selected action's original default, `D` restores all
original defaults, and `Escape` cancels a pending rebind or returns to the main
menu.  Duplicate action keys are rejected so an accidental rebind cannot
silently steal another gameplay action.

The editor also states the mission-critical weapon distinction recovered from the
original Instructions pages: a **blue Probe disarms the DRONE**, while a **red
Stinger missile homes on enemies**. The default mapping is `Ctrl` for main fire,
`Down` to select/load/cycle Probe/Stinger, and `Up` to launch the selected special.

Bindings are persisted as `drone-controls.cfg` beside the playable package.  A
missing file means the recovered/default keyboard mapping is used.  The file is
written only after the user changes or restores a binding.

Live gameplay now carries a host-only weapon help overlay at the start of a
non-demo game. It shows the active (possibly remapped) keys, identifies the blue
Probe as the DRONE-disarm weapon, and disappears after the first successful
special launch. `F4` toggles the help at any time. While a special remains loaded,
the host displays `PROBE READY` or `STINGER READY` together with the active launch
key. The pause overlay likewise shows the active main-fire, select, launch, shield,
and resume bindings. These are presentation aids only and do not alter the
reconstructed 320x200 gameplay/input semantics.

Front-end navigation remains fixed and independent from gameplay remapping so a
bad gameplay binding cannot strand the user in the menus.  Instructions accept
left/up for previous page and right/down/Enter for next page; Ordering accepts
left for previous and right/Enter for next; Escape returns to the main menu.
The difficulty selector also accepts Escape to cancel back to the menu.
