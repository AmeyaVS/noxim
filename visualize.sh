#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
CALLER_DIR="$(pwd -P)"
OUTPUT_DIR="$ROOT_DIR/other/visualizer-output"
DEFAULT_TRACE_BASE="$OUTPUT_DIR/noxim_trace"
TRACE_BASE="$DEFAULT_TRACE_BASE"
TRACE_SCOPE="router,buffers"
CONFIG_FILE=""
POWER_FILE=""
AUTO_OPEN=1
USER_SET_TRACE=0
USER_SET_SCOPE=0
USER_SET_CONFIG=0
USER_SET_POWER=0

declare -a NOXIM_ARGS=()

usage() {
  cat <<'EOF'
Usage: ./visualize.sh [NOXIM OPTIONS]

Run a traced Noxim simulation and convert the resulting VCD trace into a
self-contained HTML viewer.

Defaults:
  -config      config_examples/default_config.yaml
  -power       bin/power.yaml
  -trace       other/visualizer-output/noxim_trace
  -trace_scope router,buffers
  auto-open    enabled when a desktop opener is available

Examples:
  ./visualize.sh -sim 40 -warmup 0 -seed 0 -pir 0.3 poisson
  ./visualize.sh -config config_examples/multi_channel.yaml -sim 20 -seed 0
  ./visualize.sh -trace /tmp/my_run -trace_scope router,buffers -sim 30
  ./visualize.sh --no-open -sim 20 -seed 0
EOF
}

resolve_path() {
  local path="$1"

  if [[ "$path" = /* ]]; then
    printf '%s\n' "$path"
  else
    printf '%s/%s\n' "$CALLER_DIR" "$path"
  fi
}

require_value() {
  local option="$1"

  if (($# == 1)); then
    echo "[visualize] Error: missing value for $option" >&2
    usage >&2
    exit 1
  fi
}

open_viewer() {
  local html_file="$1"

  if (( AUTO_OPEN == 0 )); then
    return 0
  fi

  if command -v open >/dev/null 2>&1; then
    if open "$html_file" >/dev/null 2>&1; then
      return 0
    fi
  fi

  if command -v xdg-open >/dev/null 2>&1; then
    if xdg-open "$html_file" >/dev/null 2>&1; then
      return 0
    fi
  fi

  echo "[visualize] Warning: could not auto-open viewer, open it manually: $html_file" >&2
  return 0
}

while (($#)); do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    --no-open)
      AUTO_OPEN=0
      shift
      ;;
    -config)
      require_value "$1" "$@"
      CONFIG_FILE="$(resolve_path "$2")"
      USER_SET_CONFIG=1
      NOXIM_ARGS+=("$1" "$CONFIG_FILE")
      shift 2
      ;;
    -power)
      require_value "$1" "$@"
      POWER_FILE="$(resolve_path "$2")"
      USER_SET_POWER=1
      NOXIM_ARGS+=("$1" "$POWER_FILE")
      shift 2
      ;;
    -trace)
      require_value "$1" "$@"
      TRACE_BASE="$(resolve_path "$2")"
      TRACE_BASE="${TRACE_BASE%.vcd}"
      USER_SET_TRACE=1
      NOXIM_ARGS+=("$1" "$TRACE_BASE")
      shift 2
      ;;
    -trace_scope)
      require_value "$1" "$@"
      TRACE_SCOPE="$2"
      USER_SET_SCOPE=1
      NOXIM_ARGS+=("$1" "$2")
      shift 2
      ;;
    *)
      NOXIM_ARGS+=("$1")
      shift
      ;;
  esac
done

mkdir -p "$OUTPUT_DIR"

if [[ ! -x "$ROOT_DIR/bin/noxim" ]]; then
  echo "[visualize] bin/noxim not found, building it first"
  "$ROOT_DIR/build.sh"
fi

if (( USER_SET_CONFIG == 0 )); then
  CONFIG_FILE="$ROOT_DIR/config_examples/default_config.yaml"
  NOXIM_ARGS+=("-config" "$CONFIG_FILE")
fi

if (( USER_SET_POWER == 0 )); then
  POWER_FILE="$ROOT_DIR/bin/power.yaml"
  NOXIM_ARGS+=("-power" "$POWER_FILE")
fi

if (( USER_SET_TRACE == 0 )); then
  NOXIM_ARGS+=("-trace" "$TRACE_BASE")
fi

if (( USER_SET_SCOPE == 0 )); then
  NOXIM_ARGS+=("-trace_scope" "$TRACE_SCOPE")
fi

echo "[visualize] Running traced simulation"
cd "$ROOT_DIR"
"$ROOT_DIR/bin/noxim" "${NOXIM_ARGS[@]}"

VCD_FILE="${TRACE_BASE}.vcd"
HTML_FILE="${TRACE_BASE}.html"

if [[ ! -f "$VCD_FILE" ]]; then
  echo "[visualize] Error: expected trace file $VCD_FILE was not produced" >&2
  exit 1
fi

echo "[visualize] Building HTML viewer"
python3 "$ROOT_DIR/other/noxim_trace_viewer.py" \
  --vcd "$VCD_FILE" \
  --output "$HTML_FILE" \
  --config "$CONFIG_FILE"

echo "[visualize] Viewer ready: $HTML_FILE"
open_viewer "$HTML_FILE"
