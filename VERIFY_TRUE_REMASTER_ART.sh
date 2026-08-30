#!/usr/bin/env bash
ROOT="${1:-$HOME/GitHub/Drone}"
HD="$ROOT/assets_remastered"
BIN="$ROOT/build-hd-art/drone_playable_host"
ASSETS=""
for candidate in "$ROOT/runtime-assets" "$ROOT/assets"; do
  if [[ -s "$candidate/SHIP.JBA" && -s "$candidate/TITLESH.JBA" && -s "$candidate/DRONE.JBA" ]]; then
    ASSETS="$candidate"
    break
  fi
done

if [[ ! -f "$HD/.drone-remastered" ]]; then
  echo "REMASTER VERIFY FAIL: marker missing: $HD/.drone-remastered"
elif [[ -z "$ASSETS" ]]; then
  echo "REMASTER VERIFY FAIL: classic runtime corpus missing"
elif [[ ! -x "$BIN" ]]; then
  echo "REMASTER VERIFY FAIL: playable host missing: $BIN"
else
  echo "=== REMASTER CORPUS ==="
  cat "$HD/.drone-remastered"
  echo
  echo "=== RUNTIME SELF-TEST ==="
  "$BIN" "$ASSETS" --hd-root "$HD" --hd-self-test
fi
