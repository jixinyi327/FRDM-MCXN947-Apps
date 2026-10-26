#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$APP_DIR/../.." && pwd)"
BUILD_DIR="${OSC_BUILD_DIR:-$REPO_ROOT/mcuxpresso-sdk/build/oscilloscope}"
PRIMARY_BUILD_DIR="$BUILD_DIR/oscilloscope"
PROBE_ID="${MCXN_PROBE:-#1}"
FLASH_AFTER_BUILD=false

usage()
{
  printf 'Usage: %s [--flash]\n' "$(basename "$0")"
  printf '  --flash  Build, validate and flash the core0 image with LinkServer.\n'
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --flash)
      FLASH_AFTER_BUILD=true
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage >&2
      exit 2
      ;;
  esac
  shift
done

source "$REPO_ROOT/development-plan/scripts/env.sh" >/dev/null
cd "$REPO_ROOT/mcuxpresso-sdk"

west build --sysbuild \
  -b frdmmcxn947 \
  --toolchain armgcc \
  --config debug \
  "$APP_DIR" \
  -d "$BUILD_DIR" \
  -p never \
  -Dcore_id=cm33_core0

UI_ELF="$PRIMARY_BUILD_DIR/oscilloscope_cm33_core0.elf"
OSC_BUILD_DIR="$BUILD_DIR" \
  bash "$APP_DIR/scripts/check_dual_core_layout.sh"

if command -v rg >/dev/null 2>&1; then
  LVGL_UI_INPUT=touch \
    bash "$REPO_ROOT/skills/lvgl-ui/scripts/audit_lvgl_ui.sh" "$APP_DIR/ui"

  UI_APP=oscilloscope \
  UI_SOURCE="$APP_DIR" \
  UI_MAIN="$APP_DIR/main.c" \
  UI_ELF="$UI_ELF" \
    bash "$REPO_ROOT/skills/frdm-mcxn947-lvgl-ui-performance/scripts/audit_ui_config.sh"
else
  printf 'WARN  ripgrep (rg) is unavailable; skipping optional UI audits.\n'
fi

if [[ "$FLASH_AFTER_BUILD" == true ]]; then
  BINARY="$PRIMARY_BUILD_DIR/oscilloscope_cm33_core0.bin"

  if [[ ! -r "$BINARY" ]]; then
    printf 'ERROR: flash image not found: %s\n' "$BINARY" >&2
    exit 1
  fi
  if ! command -v lsusb >/dev/null 2>&1; then
    printf 'ERROR: lsusb is not installed; cannot verify MCU-LINK.\n' >&2
    exit 1
  fi

  if command -v timeout >/dev/null 2>&1; then
    USB_LIST="$(timeout 5s lsusb 2>&1)" || {
      printf 'ERROR: lsusb failed or timed out.\n%s\n' "$USB_LIST" >&2
      exit 1
    }
  else
    USB_LIST="$(lsusb 2>&1)"
  fi

  if ! grep -Fq '1fc9:0143' <<<"$USB_LIST"; then
    printf 'ERROR: MCU-LINK FRDM-MCXN947 is not visible in WSL.\n' >&2
    printf 'Current lsusb output:\n%s\n' "$USB_LIST" >&2
    printf 'Repair the usbipd mapping manually, then rerun --flash.\n' >&2
    exit 1
  fi

  printf '\n== Flash ==\n'
  if command -v timeout >/dev/null 2>&1; then
    timeout 60s LinkServer flash \
      --probe "$PROBE_ID" \
      --update-mode none \
      MCXN947:FRDM-MCXN947 \
      load --addr 0x0 "$BINARY"
  else
    LinkServer flash \
      --probe "$PROBE_ID" \
      --update-mode none \
      MCXN947:FRDM-MCXN947 \
      load --addr 0x0 "$BINARY"
  fi
  printf 'PASS  flashed %s\n' "$BINARY"
fi
