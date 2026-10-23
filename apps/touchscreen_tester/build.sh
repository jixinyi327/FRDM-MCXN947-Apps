#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$APP_DIR/../.." && pwd)"
SKILL_DIR="$REPO_ROOT/skills/frdm-mcxn947-lvgl-ui-performance"

export MCXN_ROOT="$REPO_ROOT"
export UI_APP="touchscreen_tester"
export UI_SOURCE="$APP_DIR"
export UI_MAIN="$APP_DIR/main.c"
export UI_BUILD_DIR="${UI_BUILD_DIR:-$REPO_ROOT/mcuxpresso-sdk/build/touchscreen_tester_core0}"

exec "$SKILL_DIR/scripts/build_ui.sh" "$@"
