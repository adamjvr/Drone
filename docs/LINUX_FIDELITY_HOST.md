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
Probe as the DRONE-disarm weapon, and auto-hides after roughly five seconds unless
`F4` pins it. While a special remains loaded, the host displays `PROBE READY` or
`STINGER READY` together with the active launch key. The pause overlay likewise
shows the active main-fire, select, launch, shield, and resume bindings. These are
presentation aids only and do not alter the reconstructed 320x200 gameplay/input
semantics. Red-Stinger target reticles now use the actual retained target geometry
for session-owned trajectory ships as well as reconstructed bosses, so an ordinary
hostile lock no longer appears to drift toward the dummy screen-center target.

### Probe-attachment fire safety assist

A full live-session regression proves that an uncontested blue-Probe collision
continues through decode, disarm, Y=201 outcome commit, settlement, and the GOOD1
interstitial. The user-visible BAD1-after-attachment report was therefore isolated
to destructive fire arriving after a visually successful attachment rather than a
mission-ledger failure. The recovered core intentionally still permits this: rapid
missiles and a red Stinger can destroy an active DRONE until destruction begins.

The playable Linux host now adds a narrowly scoped keyboard safety assist without
changing that core rule. On the exact tick a blue Probe attaches it:

- deactivates any player rapid missiles that were already in flight;
- suppresses new rapid-fire requests only while the fire key is still held;
- re-enables rapid fire as soon as the player releases the fire key once; and
- displays `PROBE ATTACHED - DECODING/DISARMING` plus an explicit `DO NOT FIRE`
  status.

A deliberate new fire press after that release can still destroy the DRONE, so the
original failure mechanic remains reachable. If an enemy bomb knocks the attached
Probe off, a temporary `PROBE KNOCKED OFF - TRY AGAIN` warning is shown. BAD
interstitials also receive a host-only cause line when the source is established:
player rapid missile, red Stinger, or the original unresolved-hover timeout.

`F1` keeps the host-only lifecycle diagnostic overlay. In addition to active-group/actor/spawn/escape/kill counters it now reports the number of groups that entered mode-10 breakaway on the current tick and the number of shared CRT RNG draws consumed by the recovered breakaway gate (`BRK` / `RNG`). This is specifically useful for separating legitimate off-screen path retirement from missing-AI transitions during long play sessions.

Front-end navigation remains fixed and independent from gameplay remapping so a
bad gameplay binding cannot strand the user in the menus.  Instructions accept
left/up for previous page and right/down/Enter for next page; Ordering accepts
left for previous and right/Enter for next; Escape returns to the main menu.
The difficulty selector also accepts Escape to cancel back to the menu.

## 2026-08-30 — DRONE objective-safety / Probe clarity pass

The deterministic gameplay core continues to preserve the recovered original rule that
rapid missiles and red Stingers can detonate an active DRONE. The Linux playable host
now layers an optional, explicitly host-only **Objective Safety** mode on top of that
core so the reconstructed control scheme does not sabotage a successful Probe attempt
through ambiguous selection state or held-fire races.

Objective Safety is enabled by default and toggled with **F5**. While enabled:

- when a visible DRONE objective is active and the shared special entity is inactive,
  the host preselects the documented blue Probe before the original Down-load action;
- launching a blue Probe purges already-flying player rapid missiles before the core
  advances and suppresses new rapid fire while the Probe is seeking the DRONE;
- rapid fire remains suppressed for the full attached/decode/disarm settlement, not
  merely until the fire key is released;
- attempting to launch a loaded red Stinger at a visible DRONE is blocked and the HUD
  tells the player to cycle to the blue Probe;
- F5 can disable the assist immediately for original-risk/parity play.

The former objective banner incorrectly said that Down "selects Probe". Recovered
Win32 behavior establishes that Down from inactive actually loads the **currently
selected** Probe/Stinger kind, so that wording could lead a player to reload a Stinger
while following the help exactly. The runtime HUD now reports the actual selected kind,
seeking state, and attached two-stage decode/disarm percentage.

No `GameSession`, mission-outcome, collision, score, or original timing rule is changed
by this assist. It is presentation/input filtering in the remaster host only.


## Mission-success interstitial compositor correction (2026-08-30)

A successful live Probe-disarm playthrough exposed that the Linux host was showing `GOOD1.JBA` as a fullscreen page, producing a mostly-white screen containing only "Way to go there, Sparky!". Reinspection of Win32 `run_mission_outcome_transition` (`0x0041D690`) proves `GOODn/BADn` are not standalone pages. The original extracts only a 280x37 source region into a transparent sprite, loads the mission briefing background, blits that banner at `(17,27)`, copies the saved 160x100 every-other-pixel surveillance capture into `(14,81)`, and writes encounter hit/missed/total/percentage/score values at the recovered x=272 stat column. The host now reproduces that single composite card and the 58-presentation confirmation lock instead of requiring two Enter presses.

The same playthrough also demonstrated a display usability defect: manual `F3` scaling could create an 8x fixed window (2560x1600) larger than the physical desktop, clipping mission cards. Runtime and requested integer scales are now capped to the largest exact 320x200 multiple that fits the current X11 display while retaining nearest-neighbor presentation.

## 2026-08-30 — 12x remastered-art presentation layer

The playable X11 host can now present the separately generated 12x Drone art corpus
without changing the reconstructed 320x200 simulation, collision geometry, AI,
mission timing, or indexed-framebuffer reference path. The renderer keeps the classic
framebuffer authoritative, then substitutes high-resolution asset-backed layers at
presentation time:

- scrolling world pages are taken from the 12x decoded PNGs;
- recovered/trajectory/runtime-known sprite families use the transparent 12x isolated
  frame PNGs when available;
- HUD text, diagnostics, palette-only effects, and any art without a recovered HD
  mapping fall back to the classic indexed framebuffer;
- missing HD files never block play: the presenter falls back to classic rendering.

The expected repo-local runtime tree is `assets_hd/decoded` plus
`assets_hd/sprite_frames`. The importer supports the exact generated delivery names:

- `Drone_4K_Art_Assets_12x_2026-08-30.zip`; or
- `Drone_4K_Art_Assets_12x_2026-08-30_Part_1_of_2.zip` together with
  `Drone_4K_Art_Assets_12x_2026-08-30_Part_2_of_2.zip`.

The generated package's `INSTALL_HD_ART.sh` looks for those files in `~/Downloads`,
verifies their known SHA-256 hashes, merges the split form when necessary, and installs
`4k_12x/decoded` and `4k_12x/sprite_frames` into `assets_hd/`. `assets_hd/` is ignored
by Git because the generated PNG corpus is a local runtime asset cache rather than
source/history evidence.

HD art is enabled automatically when the tree is available. `F6` switches between HD
and CLASSIC presentation live; `--hd-art`, `--classic-art`, and `--hd-root DIR` expose
the same behavior on the command line. Integer display scaling remains independent of
art mode and still clamps to the largest 320x200 multiple that fits the X11 desktop.

## Verified 12x HD-art runtime

The HD presenter must not silently fall back to the classic indexed renderer.
`--hd-self-test` validates the installed `assets_hd` corpus by decoding known
anchor assets (`TITLESH`, `RIVERTOP`, and `SHIP_00`) and prints `HD_SELFTEST OK`
with the discovered PNG count. `--require-hd` exits with an error if the HD
corpus is unavailable instead of continuing in classic mode.

A verified generated corpus currently reports 599 PNGs. While HD mode is
active, the X11 title includes `HD:<count>` (for example `[6x HD:599]`). F6
changes that title to `CLASSIC` and back, providing an unambiguous A/B check.
The standalone delivery statically links the PNG/zlib decode implementation;
source builds use the system libpng development package.
