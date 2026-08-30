#!/usr/bin/env bash
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$ROOT/build-hd-art/drone_playable_host"
HD="$ROOT/assets_remastered"
ASSETS=""
for candidate in "$ROOT/runtime-assets" "$ROOT/assets"; do
  if [[ -s "$candidate/SHIP.JBA" && -s "$candidate/TITLESH.JBA" && -s "$candidate/DRONE.JBA" ]]; then
    ASSETS="$candidate"
    break
  fi
done

if [[ -z "$ASSETS" ]]; then
  echo "ERROR: no valid classic runtime asset root found."
elif [[ ! -x "$BIN" ]]; then
  echo "ERROR: fixed HD-capable binary is missing: $BIN"
elif [[ ! -f "$HD/.drone-remastered" ]]; then
  echo "ERROR: true-remaster corpus has not been generated yet."
  echo "Run: $ROOT/GENERATE_TRUE_REMASTER_ART.sh"
else
  echo "Launching with TRUE REMASTER art: $HD"
  "$BIN" "$ASSETS" --hd-root "$HD" --hd-art "$@"
fi
