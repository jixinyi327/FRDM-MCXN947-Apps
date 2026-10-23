#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$APP_DIR/../.." && pwd)"
BUILD_DIR="${OSC_BUILD_DIR:-$REPO_ROOT/mcuxpresso-sdk/build/oscilloscope}"
PRIMARY_BUILD_DIR="$BUILD_DIR/oscilloscope"

source "$REPO_ROOT/development-plan/scripts/env.sh" >/dev/null
cd "$REPO_ROOT/mcuxpresso-sdk"

west build --sysbuild \
  -b frdmmcxn947 \
  --toolchain armgcc \
  --config debug \
  "$APP_DIR" \
  -d "$BUILD_DIR" \
  -p always \
  -Dcore_id=cm33_core0

if [[ "${1:-}" == "--flash" ]]; then
  LinkServer reset \
    'MCXN947:FRDM-MCXN947' \
    --ms 250 --update-mode none >/dev/null 2>&1 || true
  west flash -d "$PRIMARY_BUILD_DIR" -r linkserver
fi

UI_ELF="$PRIMARY_BUILD_DIR/oscilloscope_cm33_core0.elf"
OSC_BUILD_DIR="$BUILD_DIR" \
  bash "$APP_DIR/scripts/check_dual_core_layout.sh"

LVGL_UI_INPUT=touch \
  bash "$REPO_ROOT/skills/lvgl-ui/scripts/audit_lvgl_ui.sh" "$APP_DIR/ui"

UI_APP=oscilloscope \
UI_SOURCE="$APP_DIR" \
UI_MAIN="$APP_DIR/main.c" \
UI_ELF="$UI_ELF" \
  bash "$REPO_ROOT/skills/frdm-mcxn947-lvgl-ui-performance/scripts/audit_ui_config.sh"
