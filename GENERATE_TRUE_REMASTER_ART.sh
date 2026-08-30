#!/usr/bin/env bash

# Drone true-remaster art generator — resume-safe stdin-isolation fix.
# Safe to invoke from an interactive terminal: this script never uses exit,
# exec, set -e, kill, pkill, or killall.
#
# Fixes the original generator's process-substitution bug: Real-ESRGAN could
# consume bytes from the same stdin stream used by `while read`, corrupting the
# next source pathname. This version snapshots the complete NUL-delimited file
# list into a Bash array first and gives every external worker /dev/null stdin.
# Existing valid outputs are preserved and skipped, so an interrupted/broken
# 317/599 run resumes rather than starting over.

ROOT="${1:-$HOME/GitHub/Drone}"
FAITHFUL="$ROOT/assets_hd"
OUT="$ROOT/assets_remastered"
CACHE="$ROOT/.drone-remaster-cache"
TOOL_DIR="$CACHE/realesrgan-ncnn-vulkan-20220424-ubuntu"
TOOL_ZIP="$CACHE/realesrgan-ncnn-vulkan-20220424-ubuntu.zip"
TOOL_URL="https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.5.0/realesrgan-ncnn-vulkan-20220424-ubuntu.zip"
TOOL_SHA256="e5aa6eb131234b87c0c51f82b89390f5e3e642b7b70f2b9bbe95b6a285a40c96"
STAGE="$CACHE/stage-v2"
MODEL_FULL="${DRONE_REMASTER_FULL_MODEL:-realesrgan-x4plus}"
MODEL_SPRITE="${DRONE_REMASTER_SPRITE_MODEL:-realesrgan-x4plus-anime}"
GPU_ID="${DRONE_REMASTER_GPU:-0}"
TILE="${DRONE_REMASTER_TILE:-128}"
FORCE_REBUILD="${DRONE_REMASTER_FORCE_REBUILD:-0}"
OK=1

say() { printf '%s\n' "$*"; }

image_dims() {
  ffprobe -v error -select_streams v:0 -show_entries stream=width,height \
    -of csv=s=x:p=0 "$1" </dev/null 2>/dev/null
}

say "Drone true-remaster generator v2 (resume-safe)"
say "Repo: $ROOT"
say "Faithful 12x source: $FAITHFUL"
say "Remastered output: $OUT"
say

if [[ ! -d "$FAITHFUL/decoded" || ! -d "$FAITHFUL/sprite_frames" ]]; then
  say "ERROR: $FAITHFUL is not a complete installed 12x art corpus."
  say "Apply/install the fixed HD-art package first, then rerun this generator."
  OK=0
fi

for cmd in curl unzip sha256sum ffmpeg ffprobe find sort awk sed comm; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    say "ERROR: required command is missing: $cmd"
    OK=0
  fi
done

if [[ $OK -eq 1 ]]; then
  mkdir -p "$CACHE" "$TOOL_DIR"
  ESR="$TOOL_DIR/realesrgan-ncnn-vulkan"

  if [[ ! -x "$ESR" ]]; then
    say "Downloading the pinned official Real-ESRGAN NCNN/Vulkan build..."
    TMP_ZIP="$TOOL_ZIP.partial"
    rm -f "$TMP_ZIP"
    if curl -L --fail --retry 3 --connect-timeout 20 \
         -o "$TMP_ZIP" "$TOOL_URL" </dev/null; then
      GOT_SHA="$(sha256sum "$TMP_ZIP" | awk '{print $1}')"
      if [[ "$GOT_SHA" == "$TOOL_SHA256" ]]; then
        mv -f "$TMP_ZIP" "$TOOL_ZIP"
        rm -rf "$TOOL_DIR"
        mkdir -p "$TOOL_DIR"
        if unzip -q -o "$TOOL_ZIP" -d "$TOOL_DIR" </dev/null; then
          CANDIDATE="$(find "$TOOL_DIR" -type f -name realesrgan-ncnn-vulkan -print -quit)"
          if [[ -n "$CANDIDATE" ]]; then
            chmod +x "$CANDIDATE"
            if [[ "$CANDIDATE" != "$ESR" ]]; then
              cp -f "$CANDIDATE" "$ESR"
            fi
            CANDIDATE_DIR="$(dirname "$CANDIDATE")"
            if [[ -d "$CANDIDATE_DIR/models" && "$CANDIDATE_DIR/models" != "$TOOL_DIR/models" ]]; then
              rm -rf "$TOOL_DIR/models"
              cp -a "$CANDIDATE_DIR/models" "$TOOL_DIR/models"
            fi
          else
            say "ERROR: Real-ESRGAN executable was not present in the downloaded archive."
            OK=0
          fi
        else
          say "ERROR: could not unpack Real-ESRGAN."
          OK=0
        fi
      else
        say "ERROR: Real-ESRGAN archive hash mismatch."
        say "Expected: $TOOL_SHA256"
        say "Received: $GOT_SHA"
        rm -f "$TMP_ZIP"
        OK=0
      fi
    else
      say "ERROR: could not download Real-ESRGAN from the official release URL."
      rm -f "$TMP_ZIP"
      OK=0
    fi
  fi
fi

if [[ $OK -eq 1 ]]; then
  ESR="$TOOL_DIR/realesrgan-ncnn-vulkan"
  MODELS="$TOOL_DIR/models"
  if [[ ! -x "$ESR" || ! -d "$MODELS" ]]; then
    say "ERROR: Real-ESRGAN executable/models are incomplete under $TOOL_DIR"
    OK=0
  fi
fi

if [[ $OK -eq 1 ]]; then
  say
  say "Preparing resume-safe remaster workspace..."
  mkdir -p "$STAGE/low" "$STAGE/sr4" "$STAGE/alpha" "$OUT/decoded" "$OUT/sprite_frames"

  MANIFEST="$STAGE/manifest.tsv"
  : > "$MANIFEST"

  # Snapshot the complete source list BEFORE running any worker. This is the
  # critical fix: child tools can no longer steal pathname bytes from stdin.
  SOURCES=()
  mapfile -d '' -t SOURCES < <(
    find "$FAITHFUL/decoded" "$FAITHFUL/sprite_frames" \
      -type f -iname '*.png' -print0 | sort -z
  )
  TOTAL="${#SOURCES[@]}"
  N=0
  FAILS=0
  RESUMED=0
  GENERATED_NOW=0

  say "Source PNGs queued: $TOTAL"
  if [[ "$FORCE_REBUILD" != "1" ]]; then
    say "Resume mode: existing valid remastered outputs will be reused."
  else
    say "Force rebuild mode: all assets will be regenerated."
  fi
  say

  for SRC in "${SOURCES[@]}"; do
    N=$((N + 1))

    case "$SRC" in
      "$FAITHFUL"/*) REL="${SRC#"$FAITHFUL"/}" ;;
      *)
        say "[$N/$TOTAL] WARN source escaped faithful root: $SRC"
        FAILS=$((FAILS + 1))
        continue
        ;;
    esac

    DEST="$OUT/$REL"
    mkdir -p "$(dirname "$DEST")"

    DIMS="$(image_dims "$SRC")"
    W="${DIMS%x*}"
    H="${DIMS#*x}"
    if [[ -z "$W" || -z "$H" || "$W" == "$DIMS" ]]; then
      say "[$N/$TOTAL] WARN cannot read source dimensions: $REL"
      FAILS=$((FAILS + 1))
      continue
    fi

    # Preserve work from the broken first run. Only reuse a destination when it
    # is a readable PNG with exactly the expected 12x dimensions.
    if [[ "$FORCE_REBUILD" != "1" && -f "$DEST" ]]; then
      OUT_DIMS="$(image_dims "$DEST")"
      if [[ "$OUT_DIMS" == "$DIMS" ]]; then
        RESUMED=$((RESUMED + 1))
        printf '%s\t%s\t%s\t%s\tresume-existing\n' \
          "$(printf '%s' "$REL" | sha256sum | awk '{print substr($1,1,20)}')" \
          "$REL" "$W" "$H" >> "$MANIFEST"
        say "[$N/$TOTAL] resume existing $REL"
        continue
      else
        say "[$N/$TOTAL] rebuilding invalid existing output $REL ($OUT_DIMS != $DIMS)"
      fi
    fi

    LW=$((W / 12))
    LH=$((H / 12))
    if [[ $LW -lt 1 ]]; then LW=1; fi
    if [[ $LH -lt 1 ]]; then LH=1; fi

    ID="$(printf '%s' "$REL" | sha256sum | awk '{print substr($1,1,20)}')"
    LOW="$STAGE/low/$ID.png"
    SR="$STAGE/sr4/$ID.png"
    ALPHA="$STAGE/alpha/$ID.png"
    IS_SPRITE=0
    if [[ "$REL" == sprite_frames/* ]]; then IS_SPRITE=1; fi

    if [[ $LW -lt 8 || $LH -lt 8 ]]; then
      if cp -f "$SRC" "$DEST"; then
        GENERATED_NOW=$((GENERATED_NOW + 1))
        printf '%s\t%s\t%s\t%s\tfaithful-tiny\n' "$ID" "$REL" "$W" "$H" >> "$MANIFEST"
        say "[$N/$TOTAL] faithful tiny $REL"
      else
        say "[$N/$TOTAL] WARN could not copy tiny faithful asset: $REL"
        FAILS=$((FAILS + 1))
      fi
      continue
    fi

    PREP_OK=1
    if [[ $IS_SPRITE -eq 1 ]]; then
      if ! ffmpeg -hide_banner -loglevel error -y -i "$SRC" \
           -vf "scale=${LW}:${LH}:flags=area,format=rgb24" "$LOW" </dev/null; then
        PREP_OK=0
      fi
      if ! ffmpeg -hide_banner -loglevel error -y -i "$SRC" \
           -vf "alphaextract,scale=${LW}:${LH}:flags=neighbor" "$ALPHA" </dev/null; then
        PREP_OK=0
      fi
      MODEL="$MODEL_SPRITE"
    else
      if ! ffmpeg -hide_banner -loglevel error -y -i "$SRC" \
           -vf "scale=${LW}:${LH}:flags=area,format=rgb24" "$LOW" </dev/null; then
        PREP_OK=0
      fi
      MODEL="$MODEL_FULL"
    fi

    if [[ $PREP_OK -ne 1 ]]; then
      say "[$N/$TOTAL] WARN prep failed: $REL -- copying faithful asset"
      if cp -f "$SRC" "$DEST"; then GENERATED_NOW=$((GENERATED_NOW + 1)); fi
      FAILS=$((FAILS + 1))
      continue
    fi

    say "[$N/$TOTAL] AI restore $REL  (${LW}x${LH} -> 4x -> ${W}x${H})"
    if "$ESR" -i "$LOW" -o "$SR" -m "$MODELS" -n "$MODEL" -s 4 \
         -g "$GPU_ID" -t "$TILE" -j 1:2:2 -f png </dev/null >/dev/null 2>&1; then
      if [[ $IS_SPRITE -eq 1 ]]; then
        if ffmpeg -hide_banner -loglevel error -y -i "$SR" -i "$ALPHA" \
             -filter_complex "[0:v]scale=${W}:${H}:flags=lanczos[rgb];[1:v]scale=${W}:${H}:flags=neighbor[a];[rgb][a]alphamerge,format=rgba[out]" \
             -map "[out]" -frames:v 1 "$DEST" </dev/null; then
          GENERATED_NOW=$((GENERATED_NOW + 1))
          printf '%s\t%s\t%s\t%s\t%s\n' "$ID" "$REL" "$W" "$H" "$MODEL" >> "$MANIFEST"
        else
          say "  WARN alpha merge failed -- faithful fallback"
          if cp -f "$SRC" "$DEST"; then GENERATED_NOW=$((GENERATED_NOW + 1)); fi
          FAILS=$((FAILS + 1))
        fi
      else
        if ffmpeg -hide_banner -loglevel error -y -i "$SR" \
             -vf "scale=${W}:${H}:flags=lanczos" -frames:v 1 "$DEST" </dev/null; then
          GENERATED_NOW=$((GENERATED_NOW + 1))
          printf '%s\t%s\t%s\t%s\t%s\n' "$ID" "$REL" "$W" "$H" "$MODEL" >> "$MANIFEST"
        else
          say "  WARN final resize failed -- faithful fallback"
          if cp -f "$SRC" "$DEST"; then GENERATED_NOW=$((GENERATED_NOW + 1)); fi
          FAILS=$((FAILS + 1))
        fi
      fi
    else
      say "  WARN Real-ESRGAN failed -- faithful fallback"
      if cp -f "$SRC" "$DEST"; then GENERATED_NOW=$((GENERATED_NOW + 1)); fi
      FAILS=$((FAILS + 1))
    fi

    rm -f "$LOW" "$SR" "$ALPHA"
  done

  PNGS="$(find "$OUT/decoded" "$OUT/sprite_frames" -type f -iname '*.png' | wc -l | tr -d ' ')"
  SPRITES="$(find "$OUT/sprite_frames" -type f -iname '*.png' | wc -l | tr -d ' ')"
  FAITHFUL_PNGS="$(find "$FAITHFUL/decoded" "$FAITHFUL/sprite_frames" -type f -iname '*.png' | wc -l | tr -d ' ')"
  FAITHFUL_SPRITES="$(find "$FAITHFUL/sprite_frames" -type f -iname '*.png' | wc -l | tr -d ' ')"

  if [[ "$PNGS" == "$FAITHFUL_PNGS" && "$SPRITES" == "$FAITHFUL_SPRITES" ]]; then
    {
      echo "Drone true-remaster asset corpus v2"
      echo "Generated from faithful 12x baseline via Real-ESRGAN NCNN/Vulkan"
      echo "Full-screen model: $MODEL_FULL"
      echo "Sprite model: $MODEL_SPRITE"
      echo "Files: $PNGS"
      echo "Sprites: $SPRITES"
      echo "Reused from prior valid output: $RESUMED"
      echo "Generated this run: $GENERATED_NOW"
      echo "Faithful fallbacks during this run: $FAILS"
      date -u '+Generated UTC: %Y-%m-%dT%H:%M:%SZ'
    } > "$OUT/.drone-remastered"
    cp -f "$MANIFEST" "$OUT/REMASTER_MANIFEST.tsv"
    say
    say "TRUE REMASTER GENERATION COMPLETE"
    say "Output PNGs: $PNGS"
    say "Sprite PNGs: $SPRITES"
    say "Reused existing: $RESUMED"
    say "Generated this run: $GENERATED_NOW"
    say "Faithful fallbacks this run: $FAILS"
    say "Output: $OUT"
    say
    say "Run: $ROOT/run-drone-remastered.sh --scale 6"
  else
    say
    say "ERROR: output corpus is incomplete."
    say "Expected PNGs: $FAITHFUL_PNGS  Generated: $PNGS"
    say "Expected sprites: $FAITHFUL_SPRITES  Generated: $SPRITES"
    say "The existing output has been preserved for another resume attempt."

    EXPECTED_LIST="$STAGE/expected.txt"
    ACTUAL_LIST="$STAGE/actual.txt"
    MISSING_LIST="$STAGE/missing.txt"
    printf '%s\n' "${SOURCES[@]#"$FAITHFUL"/}" | sort > "$EXPECTED_LIST"
    find "$OUT/decoded" "$OUT/sprite_frames" -type f -iname '*.png' -print \
      | sed "s#^$OUT/##" | sort > "$ACTUAL_LIST"
    comm -23 "$EXPECTED_LIST" "$ACTUAL_LIST" > "$MISSING_LIST"
    say "First missing outputs:"
    sed -n '1,20p' "$MISSING_LIST"
  fi
fi

say
say "Your interactive terminal has not been terminated by this script."
