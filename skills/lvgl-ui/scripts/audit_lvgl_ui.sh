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

printf '\n== Result ==\n'
printf 'failures=%d warnings=%d\n' "$fail_count" "$warn_count"

if (( fail_count > 0 )); then
    exit 1
fi
