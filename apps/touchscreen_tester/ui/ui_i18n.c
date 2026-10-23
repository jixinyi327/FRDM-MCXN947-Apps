/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ui_i18n.h"

#include <stddef.h>
#include <stdint.h>

#define UI_I18N_MAX_CALLBACKS 8U

typedef struct {
    const char *zh;
    const char *en;
} ui_translation_t;

static const ui_translation_t s_translations[UI_TEXT_COUNT] = {
    [UI_TEXT_MENU_FREE_DRAW] = {"自由绘制", "Free Draw"},
    [UI_TEXT_MENU_BLOCK_CHECK] = {"坏块检测", "Block Check"},
    [UI_TEXT_MENU_ACCURACY] = {"精度测试", "Accuracy"},
    [UI_TEXT_MENU_SETTINGS] = {"设置", "Settings"},
    [UI_TEXT_BUTTON_TAB] = {"标签", "TAB"},
    [UI_TEXT_BUTTON_RESET] = {"重置", "Reset"},
    [UI_TEXT_BUTTON_CANCEL] = {"取消", "Cancel"},
    [UI_TEXT_BUTTON_CONFIRM_RESET] = {"确认重置", "Reset"},
    [UI_TEXT_PAGE_FREE_DRAW_TITLE] = {"自由绘制", "Free Draw"},
    [UI_TEXT_PAGE_FREE_DRAW_SUBTITLE] = {"连续绘制以检查触摸连续性", "Draw continuously to inspect touch continuity"},
    [UI_TEXT_PAGE_BLOCK_CHECK_TITLE] = {"坏块检测", "Block Check"},
    [UI_TEXT_PAGE_BLOCK_CHECK_SUBTITLE] = {"扫描全部区域并观察未覆盖位置", "Sweep every cell and watch for untouched regions"},
    [UI_TEXT_PAGE_ACCURACY_TITLE] = {"触摸精度", "Touch Accuracy"},
    [UI_TEXT_PAGE_ACCURACY_SUBTITLE] = {"点击每个移动靶点的中心", "Tap the center of each moving target"},
    [UI_TEXT_PAGE_SETTINGS_TITLE] = {"设置", "Settings"},
    [UI_TEXT_PAGE_SETTINGS_SUBTITLE] = {"调整测试显示和重置测试状态", "Adjust test presentation and reset test state"},
    [UI_TEXT_FREE_DRAW_READY] = {"自由绘制就绪", "Free draw ready"},
    [UI_TEXT_FREE_DRAW_READY_DETAIL] = {"在画布上连续绘制以检查触摸", "Draw continuously to inspect touch"},
    [UI_TEXT_FREE_DRAW_DRAWING] = {"绘制中", "Drawing"},
    [UI_TEXT_FREE_DRAW_COORDS_HIDDEN_FORMAT] = {"坐标隐藏 | 线宽 %u 像素", "Coordinates hidden | width %u px"},
    [UI_TEXT_FREE_DRAW_TOUCH_INPUT] = {"触摸输入", "Touch input"},
    [UI_TEXT_FREE_DRAW_COORDS_FORMAT] = {"x:%3d  y:%3d | 线宽 %u 像素", "x:%3d  y:%3d | width %u px"},
    [UI_TEXT_FREE_DRAW_CLEARED] = {"画布已清空", "Canvas cleared"},
    [UI_TEXT_FREE_DRAW_CLEARED_DETAIL] = {"开始新的绘制测试", "Draw to start a new test"},
    [UI_TEXT_FREE_DRAW_WIDTH_UPDATED] = {"线宽已更新", "Line width updated"},
    [UI_TEXT_FREE_DRAW_WIDTH_DETAIL_FORMAT] = {"%u 像素 | 连续笔画", "%u px | continuous stroke"},
    [UI_TEXT_FREE_DRAW_COORDS_SHOWN] = {"触摸坐标已显示", "Touch coordinates shown"},
    [UI_TEXT_FREE_DRAW_COORDS_HIDDEN] = {"触摸坐标已隐藏", "Touch coordinates hidden"},
    [UI_TEXT_FREE_DRAW_WIDTH_SUMMARY_FORMAT] = {"绘制线宽 %u 像素", "Draw width %u px"},
    [UI_TEXT_FREE_DRAW_CANVAS_FORMAT] = {"画布 %u x %u | 线宽 %u 像素", "Canvas %u x %u | width %u px"},
    [UI_TEXT_BLOCK_COVERAGE_COMPLETE] = {"网格覆盖完成", "Coverage complete"},
    [UI_TEXT_BLOCK_COVERAGE_COMPLETE_DETAIL] = {"84 / 84 区域 | 完成", "84 / 84 cells | complete"},
    [UI_TEXT_BLOCK_COVERAGE] = {"网格覆盖", "Block coverage"},
    [UI_TEXT_BLOCK_COVERAGE_FORMAT] = {"%u / %u 区域 | 剩余 %u", "%u / %u cells | %u left"},
    [UI_TEXT_ACC_HIT_FORMAT] = {"命中 %u.%u 像素", "HIT %u.%u px"},
    [UI_TEXT_ACC_STATS_FORMAT] = {"平均 %u.%u | 最佳 %u.%u 像素 | 次数 %u", "Avg %u.%u | Best %u.%u px | N %u"},
    [UI_TEXT_ACC_MISS_FORMAT] = {"未命中 %u.%u 像素", "MISS %u.%u px"},
    [UI_TEXT_ACC_RETRY] = {"请点击靶点内并重试", "Tap inside target and try again"},
    [UI_TEXT_ACC_RESET] = {"精度已重置", "Accuracy reset"},
    [UI_TEXT_ACC_START] = {"点击移动靶点开始", "Tap the moving target to begin"},
    [UI_TEXT_SETTINGS_LINE_WIDTH_LABEL] = {"绘制线宽", "Draw line width"},
    [UI_TEXT_SETTINGS_COORDINATES_LABEL] = {"显示触摸坐标", "Show touch coordinates"},
    [UI_TEXT_SETTINGS_LANGUAGE_LABEL] = {"界面语言", "Interface language"},
    [UI_TEXT_SETTINGS_LANGUAGE_TOGGLE] = {"English", "Chinese"},
    [UI_TEXT_SETTINGS_WIDTH_UPDATED] = {"线宽已更新", "Line width updated"},
    [UI_TEXT_SETTINGS_WIDTH_DETAIL_FORMAT] = {"绘制线宽 %u 像素", "Draw width: %u px"},
    [UI_TEXT_SETTINGS_COORDS_SHOWN] = {"触摸坐标已显示", "Touch coordinates shown"},
    [UI_TEXT_SETTINGS_COORDS_HIDDEN] = {"触摸坐标已隐藏", "Touch coordinates hidden"},
    [UI_TEXT_SETTINGS_COORDS_DETAIL] = {"已应用到自由绘制页", "Applied to the free draw page"},
    [UI_TEXT_SETTINGS_READY] = {"设置就绪", "Settings ready"},
    [UI_TEXT_SETTINGS_READY_DETAIL] = {"更改立即生效", "Changes apply immediately"},
    [UI_TEXT_SETTINGS_ALL_RESET] = {"全部测试数据已重置", "All test data reset"},
    [UI_TEXT_SETTINGS_ALL_RESET_DETAIL] = {"绘制、网格和精度数据已清除", "Drawing, grid and stats cleared"},
    [UI_TEXT_CONFIRM_TITLE] = {"重置当前页面?", "Reset current page?"},
    [UI_TEXT_CONFIRM_FREE_DRAW] = {"确定要清空当前画布吗?", "Clear the current drawing canvas?"},
    [UI_TEXT_CONFIRM_BLOCK_CHECK] = {"清空 12 x 7 覆盖网格?", "Clear the 12 x 7 coverage grid?"},
    [UI_TEXT_CONFIRM_ACCURACY] = {"清空精度统计并将靶点复位?", "Clear accuracy statistics and recenter the target?"},
    [UI_TEXT_CONFIRM_SETTINGS] = {"清空绘制、网格和精度测试数据?", "Clear drawing, grid and accuracy test data?"},
};

static ui_language_t s_language = UI_LANGUAGE_ZH;
static ui_language_changed_cb_t s_callbacks[UI_I18N_MAX_CALLBACKS];
static uint32_t s_callback_count;

void ui_i18n_init(void)
{
    s_language = UI_LANGUAGE_ZH;
    s_callback_count = 0U;
}

ui_language_t ui_i18n_get_language(void)
{
    return s_language;
}

bool ui_i18n_set_language(ui_language_t language)
{
    uint32_t i;

    if ((language != UI_LANGUAGE_ZH) && (language != UI_LANGUAGE_EN))
    {
        return false;
    }
    if (language == s_language)
    {
        return true;
    }

    s_language = language;
    for (i = 0; i < s_callback_count; i++)
    {
        s_callbacks[i](s_language);
    }
    return true;
}

void ui_i18n_toggle(void)
{
    (void)ui_i18n_set_language(
        (s_language == UI_LANGUAGE_ZH) ? UI_LANGUAGE_EN : UI_LANGUAGE_ZH);
}

const char *ui_tr(ui_text_id_t id)
{
    if ((id < 0) || (id >= UI_TEXT_COUNT))
    {
        return "";
    }

    return (s_language == UI_LANGUAGE_ZH) ?
           s_translations[id].zh :
           s_translations[id].en;
}

bool ui_i18n_subscribe(ui_language_changed_cb_t callback)
{
    if ((callback == NULL) || (s_callback_count >= UI_I18N_MAX_CALLBACKS))
    {
        return false;
    }

    s_callbacks[s_callback_count++] = callback;
    return true;
}
