#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$APP_DIR/../.." && pwd)"
I18N_SOURCE="$APP_DIR/ui/ui_i18n.c"
FONT_SOURCE="$REPO_ROOT/mcuxpresso-sdk/mcuxsdk/middleware/lvgl/scripts/built_in_font/SourceHanSansSC-Normal.otf"
FONT_OUTPUT_DIR="$APP_DIR/ui/fonts"
LV_FONT_CONV_VERSION="${LV_FONT_CONV_VERSION:-1.5.3}"
LV_FONT_CONV="$REPO_ROOT/tools/lv_font_conv/node_modules/.bin/lv_font_conv"

source "$REPO_ROOT/development-plan/scripts/env.sh" >/dev/null

if [[ ! -f "$I18N_SOURCE" || ! -f "$FONT_SOURCE" ]]; then
    printf 'Missing translation source or Source Han Sans font.\n' >&2
    exit 1
fi

local_version=""
if [[ -x "$LV_FONT_CONV" ]]; then
    local_version="$("$LV_FONT_CONV" --version 2>/dev/null || true)"
fi

if [[ "$local_version" == "$LV_FONT_CONV_VERSION" ]]; then
    FONT_CONV_COMMAND=("$LV_FONT_CONV")
elif command -v npx >/dev/null; then
    FONT_CONV_COMMAND=(npx --yes "lv_font_conv@$LV_FONT_CONV_VERSION")
else
    printf 'lv_font_conv is unavailable; run development-plan/scripts/manage_dev_env.sh --setup.\n' >&2
    exit 1
fi

symbols="$(python3 - "$I18N_SOURCE" <<'PY'
from pathlib import Path
import sys

text = Path(sys.argv[1]).read_text(encoding="utf-8")
print("".join(sorted({char for char in text if ord(char) > 127})))
PY
)"

mkdir -p "$FONT_OUTPUT_DIR"
cd "$REPO_ROOT"

"${FONT_CONV_COMMAND[@]}" \
    --size 16 --bpp 4 --format lvgl --lv-include lvgl.h \
    --font "mcuxpresso-sdk/mcuxsdk/middleware/lvgl/scripts/built_in_font/SourceHanSansSC-Normal.otf" \
    -r 0x20-0x7F --symbols "$symbols" --no-kerning \
    --lv-font-name ui_font_zh_16 \
    -o "apps/touchscreen_tester/ui/fonts/ui_font_zh_16.c"
sed -i '${/^$/d;}' "apps/touchscreen_tester/ui/fonts/ui_font_zh_16.c"

"${FONT_CONV_COMMAND[@]}" \
    --size 24 --bpp 4 --format lvgl --lv-include lvgl.h \
    --font "mcuxpresso-sdk/mcuxsdk/middleware/lvgl/scripts/built_in_font/SourceHanSansSC-Normal.otf" \
    -r 0x20-0x7F --symbols "$symbols" --no-kerning \
    --lv-font-name ui_font_zh_24 \
    -o "apps/touchscreen_tester/ui/fonts/ui_font_zh_24.c"
sed -i '${/^$/d;}' "apps/touchscreen_tester/ui/fonts/ui_font_zh_24.c"

printf 'Generated Chinese UI font subsets from %s\n' "$I18N_SOURCE"
