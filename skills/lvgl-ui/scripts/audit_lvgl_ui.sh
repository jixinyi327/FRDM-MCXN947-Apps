#!/usr/bin/env bash
set -u

SOURCE_DIR="${1:-$PWD/ui}"
INPUT_MODE="${LVGL_UI_INPUT:-auto}"

fail_count=0
warn_count=0

fail() {
    fail_count=$((fail_count + 1))
    printf 'FAIL  %s\n' "$1"
}

warn() {
    warn_count=$((warn_count + 1))
    printf 'WARN  %s\n' "$1"
}

pass() {
    printf 'PASS  %s\n' "$1"
}

if [[ ! -d "$SOURCE_DIR" ]]; then
    printf 'UI source directory not found: %s\n' "$SOURCE_DIR" >&2
    exit 2
fi

if ! command -v rg >/dev/null; then
    printf 'ripgrep (rg) is required\n' >&2
    exit 2
fi

printf '== LVGL UI source audit ==\n'
printf 'Source: %s\n\n' "$SOURCE_DIR"

mapfile -t source_files < <(find "$SOURCE_DIR" -type f \( -name '*.c' -o -name '*.h' \) -print | sort)
if (( ${#source_files[@]} == 0 )); then
    fail 'no C/H source files found'
else
    pass "found ${#source_files[@]} C/H source files"
fi

generated_files="$(find "$SOURCE_DIR" -type f \
    \( -path '*/generated/*' -o -path '*/Generated/*' \) \
    \( -name '*.c' -o -name '*.h' \) -print)"
if [[ -n "$generated_files" ]]; then
    warn 'generated C/H files are present; verify they are not hand-edited'
else
    pass 'no generated C/H files found inside the audited source tree'
fi

style_calls="$(rg -n 'lv_obj_set_style_' "${source_files[@]}" 2>/dev/null || true)"
if [[ -n "$style_calls" ]]; then
    warn 'inline style setters exist; verify reuse, animation and state policies'
    printf '%s\n' "$style_calls" | sed -n '1,30p' | sed 's/^/      /'
else
    pass 'no inline style setters found'
fi

color_files="$(rg -l 'lv_color_(hex|make)' "${source_files[@]}" 2>/dev/null || true)"
color_files_outside_theme="$(printf '%s\n' "$color_files" | sed '/^$/d' | rg -v 'theme' || true)"
if [[ -n "$color_files_outside_theme" ]]; then
    warn 'color constructors found outside the theme module'
    printf '%s\n' "$color_files_outside_theme" | sed 's/^/      /'
else
    pass 'color constructors are confined to the theme module'
fi

if rg -q 'lv_style_init|lv_style_set_' "${source_files[@]}"; then
    pass 'named LVGL styles are present'
else
    warn 'no reusable lv_style_t definitions found'
fi

if rg -q 'LV_STATE_(PRESSED|FOCUSED|DISABLED|CHECKED)' "${source_files[@]}"; then
    pass 'explicit widget state selectors are present'
else
    warn 'no explicit pressed/focused/disabled/checked state selectors found'
fi

if rg -q 'lv_obj_set_ext_click_area' "${source_files[@]}"; then
    pass 'extended touch hit areas are configured'
else
    warn 'no lv_obj_set_ext_click_area calls; verify small controls meet 38x38 px'
fi

if rg -q 'lv_group_(create|add_obj|set_default|focus_obj)' "${source_files[@]}"; then
    pass 'focus-group APIs are present'
elif [[ "$INPUT_MODE" == "touch" ]]; then
    pass 'touch-only input mode: focus group not required by this project'
else
    warn 'no focus-group APIs found; required only for encoder/D-pad navigation'
fi

if rg -q 'LV_STATE_DISABLED|LV_OBJ_FLAG_(HIDDEN|CLICKABLE)' "${source_files[@]}"; then
    pass 'visibility, clickability or disabled-state controls are present'
else
    warn 'verify hidden and disabled controls are excluded from interaction'
fi

printf '\n== Localization and font configuration ==\n'

NON_FONT_FILES=()
for file in "${source_files[@]}"; do
    if [[ "$file" != */fonts/* ]]; then
        NON_FONT_FILES+=("$file")
    fi
done

PROJECT_DIR="$(cd "$SOURCE_DIR/.." && pwd)"
if [[ -n "${LVGL_UI_CONFIG:-}" ]]; then
    CONFIG_FILE="$LVGL_UI_CONFIG"
elif [[ -f "$PROJECT_DIR/prj.conf" ]]; then
    CONFIG_FILE="$PROJECT_DIR/prj.conf"
else
    CONFIG_FILE="$(find "$PROJECT_DIR" -maxdepth 2 -type f -name prj.conf -print -quit 2>/dev/null || true)"
fi

if [[ -n "$CONFIG_FILE" && -f "$CONFIG_FILE" ]]; then
    pass "LVGL project config: $CONFIG_FILE"
else
    warn 'prj.conf not found; pass LVGL_UI_CONFIG to validate text/font encoding'
fi

if (( ${#NON_FONT_FILES[@]} > 0 )); then
    NON_ASCII_FILES="$(rg -l -P '[^\x00-\x7F]' "${NON_FONT_FILES[@]}" 2>/dev/null || true)"
else
    NON_ASCII_FILES=""
fi

if [[ -n "$NON_ASCII_FILES" ]]; then
    if [[ -z "$CONFIG_FILE" || ! -f "$CONFIG_FILE" ]]; then
        warn 'non-ASCII UI text exists but UTF-8 configuration could not be verified'
    elif rg -q '^CONFIG_LV_TXT_ENC_ASCII=y$' "$CONFIG_FILE"; then
        fail 'non-ASCII UI text requires UTF-8 encoding, but ASCII encoding is enabled'
    elif rg -q '^CONFIG_LV_TXT_ENC_UTF8=y$' "$CONFIG_FILE"; then
        pass 'non-ASCII UI text uses UTF-8 encoding'
    else
        fail 'non-ASCII UI text exists but CONFIG_LV_TXT_ENC_UTF8 is not enabled'
    fi
else
    pass 'no non-font source files contain non-ASCII literals'
fi

COMPRESSED_FONT_FILES="$(rg -l 'bitmap_format[[:space:]]*=[[:space:]]*1' "${source_files[@]}" 2>/dev/null || true)"
if [[ -n "$COMPRESSED_FONT_FILES" ]]; then
    if [[ -z "$CONFIG_FILE" || ! -f "$CONFIG_FILE" ]]; then
        warn 'compressed font data exists but decoder configuration could not be verified'
    elif rg -q '^CONFIG_LV_USE_FONT_COMPRESSED=y$' "$CONFIG_FILE"; then
        pass 'compressed font data matches LV_USE_FONT_COMPRESSED'
    else
        fail 'compressed font data requires CONFIG_LV_USE_FONT_COMPRESSED=y'
    fi
else
    pass 'no compressed custom font data detected'
fi

printf '\n== Result ==\n'
printf 'failures=%d warnings=%d\n' "$fail_count" "$warn_count"

if (( fail_count > 0 )); then
    exit 1
fi
