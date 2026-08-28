# Apply / Build / Verify

Milestone ZIPs are structured so their contents can be extracted directly into the root of the Git repository named **`Drone`**.

## Recommended branch for this Phase 2 gameplay-reconstruction checkpoint

Start from the Phase 1 baseline already merged to `main`:

```bash
cd ~/Github/Drone
git switch main
git pull --ff-only origin main
git switch -c re/phase2-gameplay-reconstruction
```

## Apply

```bash
unzip -o ~/Downloads/Drone_Phase2_Gameplay_Reconstruction.zip
```

The archive contains repository-root files rather than an extra top-level directory.

## macOS / Linux build

```bash
cmake -S . -B build -G Ninja
cmake --build build -j
ctest --test-dir build --output-on-failure
python3 scripts/check_research_docs.py
```

If Ninja is unavailable, omit `-G Ninja` and use the default CMake generator.

On Linux, `drone_fidelity_host` is built when X11 development files are available. On macOS, the target uses Cocoa/CoreGraphics.

## Windows build

From a Developer PowerShell / environment with CMake and a C++ toolchain:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The Windows fidelity host uses Win32/GDI. The production library and synthetic tests require no original game files.

## Reconstruct canonical local evidence

Place the two known source packages under `.reference/input/`:

```text
.reference/input/drone_sw.zip
.reference/input/drone_sw(win).zip
```

Example on macOS/Linux:

```bash
mkdir -p .reference/input
cp ~/Downloads/drone_sw.zip .reference/input/
cp ~/Downloads/'drone_sw(win).zip' .reference/input/

python3 scripts/bootstrap_reference.py .reference/input .reference/work
python3 scripts/verify_reference.py .reference/work
```

The bootstrap refuses source packages whose SHA-256 values do not match the canonical evidence set. The Windows installer is decompressed directly and is not executed.

Expected principal-binary verification includes:

```text
PASS dos/DRONE_SW.EXE e7de54f9cd158289df58a7eee4ecbe6f1b9b04972e7eae88b26bfe32515a0fc0
PASS windows/Drone_sw.exe 4fffc0406157b0def539b619fa53d1bb6c59537a44a6551df3aa3c060eaf3784
```

## Exercise recovered formats against local evidence

```bash
./build/drone_inspect \
  jba-info \
  .reference/work/windows/Sights/Titlesh.jba \
  /tmp/drone-title.ppm

./build/drone_inspect \
  clv-info \
  .reference/work/dos/DRONE.CLV \
  /tmp/drone.wav

./build/drone_inspect \
  fly-info \
  .reference/work/windows/Data/Current.fly

./build/drone_inspect \
  fly-info \
  .reference/work/windows/Data/Rightdiv.fly

./build/drone_inspect \
  demo-info \
  .reference/work/windows/Data/Demoa2.dat
```

Reproduce replay/sprite metadata without copying original payloads into Git:

```bash
python3 scripts/analyze_demo_assets.py \
  .reference/work/windows/Data \
  --dos-root .reference/work/dos \
  --output /tmp/demo_replays.csv

diff -u manifests/demo_replays.csv /tmp/demo_replays.csv

python3 scripts/analyze_sprite_sheets.py \
  .reference/work/windows \
  --output /tmp/recovered_sprite_frames.csv

diff -u manifests/recovered_sprite_frames.csv /tmp/recovered_sprite_frames.csv

`manifests/world_scenery_roles.csv` is metadata-only and records executable-referenced top/mid/bottom scenery families plus whether each file is physically present in the canonical Windows shareware corpus. Registered-only isle/house/night rows intentionally have no payload hash. `scripts/check_research_docs.py` validates that contract.
```

The gameplay probe exercises the independently reconstructed player/rapid-missile rendering slice against user-supplied assets:

```bash
./build/drone_gameplay_probe \
  .reference/work/windows \
  /tmp/drone-gameplay-probe.ppm \
  36
```

`CURRENT.FLY` uses the counted form. Known runtime trajectory names such as `Rightdiv.fly` are interpreted with the recovered hard-coded loader count while still reporting the physical file length.

## Run the first native fidelity host

The host currently presents a decoded full-screen JBA through the clean 320×200 indexed framebuffer. It is a presentation boundary test, not yet reconstructed gameplay.

```bash
./build/drone_fidelity_host \
  .reference/work/windows/Sights/Titlesh.jba \
  3
```

Use `Escape` or `Q` to exit. The optional final argument is integer scale.

Linux can validate the same host binary without an X display by using a clean snapshot and landmark capture:

```bash
./build/drone_fidelity_host \
  /tmp/clean-frame.drfb \
  --headless \
  --capture-dir /tmp/drone-captures \
  --landmark "validation frame" \
  --sequence 1
```

See `docs/LINUX_FIDELITY_HOST.md` for the capture + fingerprint workflow.

## Reproduce the DOS LE object image

```bash
python3 scripts/extract_dos_le.py \
  .reference/work/dos/DRONE_SW.EXE \
  .reference/work/dos-le
```

For the canonical DOS executable, reconstructed object 1 must hash to:

```text
6498fbae9d748d962bb6ee308d9acdbe1d86a8b6eae1cd2b2a6da0841e67a4ec
```

The extractor also writes `le_metadata.json` describing the load objects, entry point, stack, and page mapping used for reproducible DOS analysis.

## Reproduce FLY corpus metadata

```bash
python3 scripts/analyze_fly_assets.py \
  .reference/work/windows/Data \
  --csv /tmp/fly_trajectories.csv

diff -u manifests/fly_trajectories.csv /tmp/fly_trajectories.csv
```

The checked-in manifest stores hashes, sizes, physical/loader counts, and numerical ranges only; it does not contain original trajectory records.

## Documentation/research metadata validation

```bash
python3 scripts/check_research_docs.py
```

The validator checks repository-internal links, stable ledger IDs, corpus manifest integrity, Wise stream/install counts, and the FLY trajectory manifest invariants established during Phase 2.

## Commit

```bash
git status
git add -A
git status
git commit -m "Advance Drone Phase 2 gameplay reconstruction"
```

Before committing, confirm no `.reference/`, Ghidra project/database, decoded original asset, executable, or other proprietary payload has been staged.

## Optional Phase-3 framebuffer validation

After a normal build, local indexed-framebuffer captures can be checked without adding original frame bytes to Git:

```bash
./build/drone_framecheck info /path/to/frame.drfb
./build/drone_framecheck compare /path/to/reference.drfb /path/to/candidate.drfb
python3 scripts/framebuffer_fixture.py verify /path/to/metadata.json /path/to/reference.drfb
```

See `docs/FRAMEBUFFER_VALIDATION.md` for the `DRONEFB1` format and provenance requirements.


## Phase architecture gates

After a normal build/test run, the durable phase-exit checks can also be invoked directly:

```bash
python3 scripts/check_phase2_exit.py
python3 scripts/check_phase3_exit.py
```

At the Phase-3 closure checkpoint both must pass. Phase 4 may add new integration tests without weakening these completed-phase invariants.

## Phase-4 continuous-session probe

The first complete-session integration milestone has an asset-free deterministic state oracle:

```bash
./build-debug/drone_session_probe 120
```

CTest runs the same probe through `drone_game_session_probe`. This validation is independent of the proprietary reference bootstrap.
