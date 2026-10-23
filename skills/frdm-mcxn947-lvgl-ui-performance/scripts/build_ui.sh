#!/usr/bin/env bash
set -euo pipefail

BUILD_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$BUILD_SCRIPT_DIR/../../.." && pwd)"

MCXN_ROOT="${MCXN_ROOT:-$REPO_ROOT}"
MCXN_SDK="${MCXN_SDK:-$MCXN_ROOT/mcuxpresso-sdk}"
UI_APP="${UI_APP:-lvgl_examples/lvgl_guider_bm}"
UI_BUILD_DIR="${UI_BUILD_DIR:-$MCXN_SDK/build/lvgl_s035_core0}"
UI_BOARD="${UI_BOARD:-frdmmcxn947}"
UI_CORE="${UI_CORE:-cm33_core0}"
UI_CONFIG="${UI_CONFIG:-debug}"
UI_SOURCE="${UI_SOURCE:-$MCXN_SDK/mcuxsdk/examples/$UI_APP}"
UI_MAIN="${UI_MAIN:-$UI_SOURCE/$(basename "$UI_APP").c}"

PROJECT_ENV="$MCXN_ROOT/development-plan/scripts/env.sh"
if [[ ! -f "$PROJECT_ENV" ]]; then
    printf 'Project environment not found: %s\n' "$PROJECT_ENV" >&2
    exit 1
fi

# shellcheck disable=SC1090
source "$PROJECT_ENV" >/dev/null

if [[ ! -d "$UI_SOURCE" ]]; then
    printf 'UI source not found: %s\n' "$UI_SOURCE" >&2
    exit 1
fi

if [[ ! -f "$UI_MAIN" ]]; then
    printf 'UI main source not found: %s\n' "$UI_MAIN" >&2
    printf 'Set UI_MAIN explicitly if the application entry file has another name.\n' >&2
    exit 1
fi

cd "$MCXN_SDK"

printf 'Building %s for %s@%s\n' "$UI_SOURCE" "$UI_BOARD" "$UI_CORE"
west build -b "$UI_BOARD" "$UI_SOURCE" \
    -d "$UI_BUILD_DIR" -p always \
    --config "$UI_CONFIG" -Dcore_id="$UI_CORE"

printf '\nBuild output: %s\n' "$UI_BUILD_DIR"

if [[ "${1:-}" == "--flash" ]]; then
    west flash -d "$UI_BUILD_DIR" -r linkserver
fi

UI_ELF="$(find "$UI_BUILD_DIR" -maxdepth 1 -type f -name "*_${UI_CORE}.elf" -print -quit)"
if [[ -z "$UI_ELF" ]]; then
    printf 'Built ELF not found under: %s\n' "$UI_BUILD_DIR" >&2
    exit 1
fi

printf 'ELF output:   %s\n' "$UI_ELF"

UI_APP="$UI_APP" \
UI_SOURCE="$UI_SOURCE" \
UI_MAIN="$UI_MAIN" \
UI_ELF="$UI_ELF" \
"$BUILD_SCRIPT_DIR/audit_ui_config.sh"
