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
else
  SELFTEST="$($BIN "$ASSETS" --hd-root "$HD" --hd-self-test 2>&1)"
  SELFTEST_RC=$?
  echo "$SELFTEST"
  if [[ $SELFTEST_RC -eq 0 ]] && grep -q '^HD_SELFTEST OK ' <<<"$SELFTEST"; then
    "$BIN" "$ASSETS" --hd-root "$HD" "$@" --require-hd
  else
    echo "ERROR: HD self-test failed; game was not launched in a silent classic fallback." >&2
  fi
fi
