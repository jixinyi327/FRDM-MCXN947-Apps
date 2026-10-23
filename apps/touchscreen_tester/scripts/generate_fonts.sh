#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$APP_DIR/../.." && pwd)"
I18N_SOURCE="$APP_DIR/ui/ui_i18n.c"
FONT_SOURCE="$REPO_ROOT/mcuxpresso-sdk/mcuxsdk/middleware/lvgl/scripts/built_in_font/SourceHanSansSC-Normal.otf"
FONT_OUTPUT_DIR="$APP_DIR/ui/fonts"

if [[ ! -f "$I18N_SOURCE" || ! -f "$FONT_SOURCE" ]]; then
    printf 'Missing translation source or Source Han Sans font.\n' >&2
    exit 1
fi

if ! command -v npx >/dev/null; then
    printf 'npx is required to run lv_font_conv.\n' >&2
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

npx --yes lv_font_conv \
    --size 16 --bpp 4 --format lvgl --lv-include lvgl.h \
    --font "$FONT_SOURCE" \
    -r 0x20-0x7F --symbols "$symbols" --no-kerning \
    --lv-font-name ui_font_zh_16 \
    -o "$FONT_OUTPUT_DIR/ui_font_zh_16.c"
sed -i '${/^$/d;}' "$FONT_OUTPUT_DIR/ui_font_zh_16.c"

npx --yes lv_font_conv \
    --size 24 --bpp 4 --format lvgl --lv-include lvgl.h \
    --font "$FONT_SOURCE" \
    -r 0x20-0x7F --symbols "$symbols" --no-kerning \
    --lv-font-name ui_font_zh_24 \
    -o "$FONT_OUTPUT_DIR/ui_font_zh_24.c"
sed -i '${/^$/d;}' "$FONT_OUTPUT_DIR/ui_font_zh_24.c"

printf 'Generated Chinese UI font subsets from %s\n' "$I18N_SOURCE"
