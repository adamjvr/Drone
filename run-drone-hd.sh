#!/usr/bin/env bash
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$ROOT/build-hd-art/drone_playable_host"
HD="$ROOT/assets_hd"

ASSETS=""
for candidate in "$ROOT/runtime-assets" "$ROOT/assets"; do
  if [[ -s "$candidate/SHIP.JBA" && -s "$candidate/TITLESH.JBA" && -s "$candidate/DRONE.JBA" ]]; then
    ASSETS="$candidate"
    break
  fi
done

if [[ -z "$ASSETS" ]]; then
  echo "ERROR: no valid classic runtime asset root found." >&2
  echo "Rerun the HD repo updater; your terminal will remain open." >&2
elif [[ ! -x "$BIN" ]]; then
  echo "ERROR: HD-capable Drone binary is not built at: $BIN" >&2
  echo "Rerun the repo update/build script; your terminal will remain open." >&2
else
  SELFTEST="$($BIN "$ASSETS" --hd-root "$HD" --hd-self-test 2>&1)"
  SELFTEST_RC=$?
  echo "$SELFTEST"
  if [[ $SELFTEST_RC -eq 0 ]] && grep -q '^HD_SELFTEST OK ' <<<"$SELFTEST"; then
    # Do not force --require-hd here. The persistent Video Settings preference
    # owns the startup choice; --hd-art/--classic-art remain temporary CLI overrides.
    "$BIN" "$ASSETS" --hd-root "$HD" "$@"
  else
    echo "ERROR: HD self-test failed; game was not launched with an unverified HD cache." >&2
  fi
fi
