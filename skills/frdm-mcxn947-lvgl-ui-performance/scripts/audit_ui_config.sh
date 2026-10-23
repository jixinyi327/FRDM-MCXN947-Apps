#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
MCXN_ROOT="${MCXN_ROOT:-$REPO_ROOT}"
MCXN_SDK="${MCXN_SDK:-$MCXN_ROOT/mcuxpresso-sdk}"

PROJECT_ENV="$MCXN_ROOT/development-plan/scripts/env.sh"
if [[ -f "$PROJECT_ENV" ]]; then
    # Load the pinned west, GCC and LinkServer toolchain for standalone audits.
    # shellcheck disable=SC1090
    source "$PROJECT_ENV" >/dev/null 2>&1 || true
fi

SDK_EXAMPLES="$MCXN_SDK/mcuxsdk/examples"
UI_APP="${UI_APP:-lvgl_examples/lvgl_guider_bm}"
UI_SOURCE="${UI_SOURCE:-$SDK_EXAMPLES/$UI_APP}"
UI_MAIN="${UI_MAIN:-$UI_SOURCE/$(basename "$UI_APP").c}"

LVGL_SUPPORT="${LVGL_SUPPORT:-$SDK_EXAMPLES/lvgl_examples/lvgl_sdk/lvgl_support/mcx_flexio/lvgl_support.c}"
LVGL_SUPPORT_H="${LVGL_SUPPORT_H:-$SDK_EXAMPLES/lvgl_examples/lvgl_sdk/lvgl_support/mcx_flexio/lvgl_support.h}"
ELF="${UI_ELF:-$MCXN_SDK/build/lvgl_s035_core0/lvgl_guider_bm_cm33_core0.elf}"

fail_count=0
warn_count=0

pass() {
    printf 'PASS  %s\n' "$1"
}

warn() {
    warn_count=$((warn_count + 1))
    printf 'WARN  %s\n' "$1"
}

fail() {
    fail_count=$((fail_count + 1))
    printf 'FAIL  %s\n' "$1"
}

has() {
    local pattern="$1"
    local file="$2"
    grep -Eq -- "$pattern" "$file"
}

printf '== FRDM-MCXN947 LVGL UI audit ==\n'
printf 'Repository: %s\n' "$MCXN_ROOT"
printf 'SDK:        %s\n\n' "$MCXN_SDK"

for file in "$LVGL_SUPPORT" "$LVGL_SUPPORT_H" "$UI_MAIN"; do
    if [[ ! -f "$file" ]]; then
        fail "missing expected source: $file"
    fi
done

if (( fail_count > 0 )); then
    printf '\nAudit cannot continue until the expected SDK source exists.\n'
    exit 2
fi

printf '== Display configuration ==\n'

if has 'LCD_VIRTUAL_BUF_HEIGHT \(LCD_HEIGHT / 4\)' "$LVGL_SUPPORT_H"; then
    pass 'partial stripe is one quarter of screen height'
else
    warn 'partial stripe is not the accepted 1/4 height; verify RAM and tearing'
fi

if has 'lv_display_set_buffers\(.*sizeof\(s_frameBuffer\[0\]\)' "$LVGL_SUPPORT"; then
    pass 'LVGL buffer size is passed in bytes'
else
    fail 'lv_display_set_buffers may be using pixels instead of bytes'
fi

if has 'lv_display_set_buffers.*s_frameBuffer\[1\]' "$LVGL_SUPPORT"; then
    pass 'two LVGL partial buffers are configured'
else
    warn 'single-buffer configuration detected; verify its latency and tearing tradeoff'
fi

if has 'kST7796[Ss]_TEVSync' "$LVGL_SUPPORT"; then
    pass 'VBlank TE synchronization is enabled'
elif has 'kST7796S_TEHVSync' "$LVGL_SUPPORT"; then
    warn 'TE H/V mode is experimental; re-baseline latency and tearing'
elif has 'kST7796S_TEDisabled' "$LVGL_SUPPORT"; then
    fail 'TE is disabled'
fi

if has 'DEMO_WaitForTE' "$LVGL_SUPPORT"; then
    pass 'TE wait path is present'
else
    fail 'TE wait path is missing'
fi

if has 'if \(s_teSyncPending\)' "$LVGL_SUPPORT" &&
   has 'lv_display_flush_is_last' "$LVGL_SUPPORT" &&
   has 's_teSyncPending = true' "$LVGL_SUPPORT"; then
    pass 'TE is gated once per complete LVGL update'
else
    fail 'TE batching markers are missing; the UI may wait once per strip'
fi

printf '\n== Buffer handoff ==\n'

if has 'NULL != s_display' "$LVGL_SUPPORT"; then
    pass 'DMA callback guards the pre-LVGL display-pointer state'
else
    fail 'missing s_display NULL guard; initial LCD clear can HardFault'
fi

if has 'lv_display_flush_ready\(s_display\)' "$LVGL_SUPPORT"; then
    pass 'SmartDMA completion releases the active LVGL buffer'
else
    fail 'DMA completion does not release the LVGL buffer'
fi

if has 'kStatus_Success !=' "$LVGL_SUPPORT" &&
   has 'ST7796S_WritePixels' "$LVGL_SUPPORT"; then
    pass 'ST7796S write status is checked with an error path'
else
    fail 'ST7796S write status/error handling is missing'
fi

flush_block="$(sed -n '/static void DEMO_FlushDisplay/,/^}$/p' "$LVGL_SUPPORT")"
if printf '%s\n' "$flush_block" | grep -Eq 'portMAX_DELAY|while \(!s_memWriteDone\)'; then
    warn 'flush callback appears to block on SmartDMA completion'
else
    pass 'flush callback is non-blocking (pipelined buffers)'
fi

printf '\n== UI cadence ==\n'

if has '#define[[:space:]]+(LVGL_TASK_PERIOD_TICK|UI_TASK_PERIOD_TICKS)[[:space:]]+16U' "$UI_MAIN"; then
    pass 'UI target is about 60 FPS (16 ms)'
elif has '#define[[:space:]]+(LVGL_TASK_PERIOD_TICK|UI_TASK_PERIOD_TICKS)[[:space:]]+33U' "$UI_MAIN"; then
    warn 'UI target is about 30 FPS; acceptable for lower resource use'
elif has '#define[[:space:]]+(LVGL_TASK_PERIOD_TICK|UI_TASK_PERIOD_TICKS)[[:space:]]+5U' "$UI_MAIN"; then
    warn 'UI loop is 5 ms, much faster than the panel scan'
fi

printf '\n== Link result ==\n'

if [[ ! -f "$ELF" ]]; then
    warn "built ELF not found: $ELF; run build_ui.sh first"
elif ! command -v arm-none-eabi-size >/dev/null; then
    warn 'arm-none-eabi-size unavailable; load the project toolchain environment'
else
    read -r text_size data_size bss_size _dec _hex _name < <(arm-none-eabi-size "$ELF" | tail -n 1)
    static_ram=$((data_size + bss_size))
    printf 'ELF:  %s\n' "$ELF"
    printf 'text=%s data=%s bss=%s static_ram=%s\n' \
        "$text_size" "$data_size" "$bss_size" "$static_ram"

    if (( static_ram > 319488 )); then
        fail 'static RAM exceeds the 312 KiB primary data-region limit'
    elif (( static_ram > 280000 )); then
        warn 'static RAM has less than about 10% headroom'
    else
        pass 'static RAM fits the primary data region'
    fi
fi

printf '\n== Project patches ==\n'
PATCH_DIR="$MCXN_ROOT/firmware/patches"
if compgen -G "$PATCH_DIR/*.patch" >/dev/null; then
    while IFS= read -r patch_file; do
        patch_name="$(basename "$patch_file")"
        if command -v sha256sum >/dev/null; then
            patch_hash="$(sha256sum "$patch_file" | cut -d' ' -f1)"
            printf '%s  %s\n' "$patch_hash" "$patch_name"
        else
            printf '%s\n' "$patch_name"
        fi
    done < <(find "$PATCH_DIR" -maxdepth 1 -type f -name '*.patch' -print | sort)
else
    warn "no project patches found under $PATCH_DIR"
fi

printf '\n== Source revisions ==\n'
if [[ -d "$MCXN_ROOT/.git" ]]; then
    printf 'project:  '
    git -C "$MCXN_ROOT" rev-parse --short HEAD 2>/dev/null || true
    printf 'worktree: '
    if [[ -n "$(git -C "$MCXN_ROOT" status --porcelain)" ]]; then
        printf 'dirty\n'
        warn 'project repository has uncommitted changes'
    else
        printf 'clean\n'
    fi
fi

if [[ -d "$SDK_EXAMPLES/.git" ]]; then
    printf 'examples: '
    git -C "$SDK_EXAMPLES" rev-parse --short HEAD 2>/dev/null || true
    printf 'examples worktree: '
    if [[ -n "$(git -C "$SDK_EXAMPLES" status --porcelain)" ]]; then
        printf 'dirty (expected while project patches are applied)\n'
    else
        printf 'clean\n'
    fi
fi

printf '\n== Result ==\n'
printf 'failures=%d warnings=%d\n' "$fail_count" "$warn_count"

if (( fail_count > 0 )); then
    exit 1
fi
