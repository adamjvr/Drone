# Apply / Build / Verify

Milestone ZIPs are structured so their contents can be extracted directly into the root of the Git repository named **`Drone`**.

## Recommended branch for this documentation-hardened Phase 1 snapshot

```bash
cd ~/Github/Drone
git switch -c re/phase1-doc-hardening
```

## Apply

```bash
unzip -o ~/Downloads/Drone_Phase1_Documentation_Hardening.zip
```

If you are applying a differently named archive, substitute that filename; the archive itself contains repository-root files rather than an extra top-level directory.

## macOS / Linux build

```bash
cmake -S . -B build -G Ninja
cmake --build build -j
ctest --test-dir build --output-on-failure
```

If Ninja is unavailable, omit `-G Ninja` and use the default CMake generator.

## Windows build

From a Developer PowerShell / environment with CMake and a C++ toolchain:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The production library and synthetic tests require no original game files.

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
  demo-info \
  .reference/work/windows/Data/Demoa2.dat
```

## Verify corpus metadata if research files change

The canonical public inventory snapshots are:

```text
manifests/dos_shareware_files.csv
manifests/windows_shareware_files.csv
```

Local bootstrap also produces fresh inventories under `.reference/work/`. When deliberately updating the canonical evidence set, compare these rather than casually overwriting the checked-in manifests.

## Commit

```bash
git add -A
git status
git commit -m "Harden Drone Phase 1 reverse-engineering documentation"
```

Before committing, inspect `git status` and confirm no `.reference/`, Ghidra database, decoded original asset, or other proprietary payload has been staged.

## Documentation/research metadata validation

The public test suite also validates repository-internal research metadata when Python 3 is available. It checks local Markdown links, stable unique ledger IDs, canonical corpus manifest row counts/hashes, and the known Wise stream/install counts.

Run it directly with:

```bash
python3 scripts/check_research_docs.py
```
