/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app_ui.h"

#include <stdint.h>

#include "lvgl.h"
#include "screen_accuracy.h"
#include "screen_block_check.h"
#include "screen_free_draw.h"
#include "screen_settings.h"
#include "ui_common.h"

#define PAGE_COUNT 4U
#define MENU_ITEM_COUNT PAGE_COUNT
#define MENU_CLOSED_WIDTH UI_FOOTER_BUTTON_WIDTH
#define MENU_CLOSED_HEIGHT UI_FOOTER_HEIGHT
#define MENU_OPEN_WIDTH 206
#define MENU_OPEN_HEIGHT 228
#define MENU_ANIM_MS 180
#define PAGE_ANIM_MS 200
#define PAGE_SLIDE_PX 14

typedef struct {
    lv_obj_t *outgoing;
    lv_obj_t *incoming;
    int32_t direction;
    uint8_t target_index;
} page_anim_context_t;

static const char *s_page_titles[PAGE_COUNT] = {
    "Free Draw",
    "Block Check",
    "Accuracy",
    "Settings",
};

static const char *s_page_badges[PAGE_COUNT] = {
    "D",
    "B",
    "A",
    "S",
};

static lv_obj_t *s_pages[PAGE_COUNT];
static lv_obj_t *s_menu_items[MENU_ITEM_COUNT];
static lv_obj_t *s_page_layer;
static lv_obj_t *s_menu_overlay;
static lv_obj_t *s_menu_panel;
static lv_obj_t *s_menu_button;
static lv_obj_t *s_control_bar;
static lv_obj_t *s_reset_button;
static lv_obj_t *s_info_label;
static lv_anim_t s_menu_animation;
static lv_anim_t s_page_animation;
static page_anim_context_t s_page_anim_context;
static int32_t s_menu_progress;
static int32_t s_menu_target_progress;
static int8_t s_pending_page = -1;
static uint8_t s_current_page;
static bool s_page_transition_active;

static void menu_refresh_selection(void)
{
    uint32_t i;

    for (i = 0; i < MENU_ITEM_COUNT; i++)
    {
        bool selected = (i == s_current_page);

        lv_obj_set_style_bg_color(s_menu_items[i],
                                  selected ? UI_COLOR_ACCENT_SOFT : UI_COLOR_SURFACE, 0);
    }
}

static void page_transition_apply_result(void)
{
    if (!s_page_transition_active)
    {
        return;
    }

    lv_obj_set_style_translate_x(s_page_anim_context.outgoing, 0, 0);
    lv_obj_set_style_translate_x(s_page_anim_context.incoming, 0, 0);
    lv_obj_add_flag(s_page_anim_context.outgoing, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_page_anim_context.incoming, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_page_anim_context.incoming);
    s_current_page = s_page_anim_context.target_index;
    s_page_transition_active = false;
    menu_refresh_selection();
    ui_info_focus((ui_page_t)s_current_page);
}

static void page_transition_cancel_and_apply(void)
{
    if (!s_page_transition_active)
    {
        return;
    }

    lv_anim_delete(&s_page_anim_context, NULL);
    page_transition_apply_result();
}

static void page_animation_cb(void *var, int32_t progress)
{
    page_anim_context_t *context = (page_anim_context_t *)var;
    int32_t outgoing_x = -(context->direction * PAGE_SLIDE_PX * progress) / 1000;
    int32_t incoming_x = (context->direction * PAGE_SLIDE_PX * (1000 - progress)) / 1000;

    lv_obj_set_style_translate_x(context->outgoing, outgoing_x, 0);
    lv_obj_set_style_translate_x(context->incoming, incoming_x, 0);
}

static void page_animation_completed(lv_anim_t *animation)
{
    LV_UNUSED(animation);
    page_transition_apply_result();
}

static void switch_page(uint8_t target_index)
{
    lv_anim_t animation;

    if (target_index >= PAGE_COUNT)
    {
        return;
    }

    if (s_page_transition_active)
    {
        page_transition_cancel_and_apply();
    }

    if (target_index == s_current_page)
    {
        return;
    }

    s_page_anim_context.outgoing = s_pages[s_current_page];
    s_page_anim_context.incoming = s_pages[target_index];
    s_page_anim_context.direction =
        (target_index > s_current_page) ? 1 : -1;
    s_page_anim_context.target_index = target_index;

    lv_obj_remove_flag(s_page_anim_context.incoming, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_page_anim_context.incoming);
    lv_obj_set_style_translate_x(s_page_anim_context.outgoing, 0, 0);
    lv_obj_set_style_translate_x(s_page_anim_context.incoming,
                                 s_page_anim_context.direction * PAGE_SLIDE_PX, 0);

    s_page_transition_active = true;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, &s_page_anim_context);
    lv_anim_set_values(&animation, 0, 1000);
    lv_anim_set_duration(&animation, PAGE_ANIM_MS);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&animation, page_animation_cb);
    lv_anim_set_completed_cb(&animation, page_animation_completed);
    s_page_animation = animation;
    lv_anim_start(&s_page_animation);
}

static void menu_animation_cb(void *var, int32_t progress)
{
    lv_obj_t *panel = (lv_obj_t *)var;
    int32_t width = MENU_CLOSED_WIDTH +
                    ((MENU_OPEN_WIDTH - MENU_CLOSED_WIDTH) * progress) / 1000;
    int32_t height = MENU_CLOSED_HEIGHT +
                     ((MENU_OPEN_HEIGHT - MENU_CLOSED_HEIGHT) * progress) / 1000;
    int32_t radius = 8 + (6 * progress) / 1000;
    lv_opa_t opacity = (lv_opa_t)((progress * (int32_t)LV_OPA_COVER) / 1000);

    s_menu_progress = progress;
    lv_obj_set_size(panel, width, height);
    lv_obj_set_style_radius(panel, radius, 0);
    lv_obj_set_style_opa(panel, opacity, 0);
}

static void menu_switch_after_close(void)
{
    if (s_pending_page >= 0)
    {
        uint8_t target = (uint8_t)s_pending_page;

        s_pending_page = -1;
        switch_page(target);
    }
}

static void menu_animation_completed(lv_anim_t *animation)
{
    LV_UNUSED(animation);

    if (s_menu_target_progress == 0)
    {
        lv_obj_add_flag(s_menu_overlay, LV_OBJ_FLAG_HIDDEN);
        menu_switch_after_close();
    }
}

static void animate_menu(int32_t target_progress)
{
    lv_anim_t animation;

    lv_anim_delete(&s_menu_panel, menu_animation_cb);
    s_menu_target_progress = target_progress;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, s_menu_panel);
    lv_anim_set_values(&animation, s_menu_progress, target_progress);
    lv_anim_set_duration(&animation, MENU_ANIM_MS);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&animation, menu_animation_cb);
    lv_anim_set_completed_cb(&animation, menu_animation_completed);
    s_menu_animation = animation;
    lv_anim_start(&s_menu_animation);
}

static void open_menu(void)
{
    s_pending_page = -1;
    lv_obj_remove_flag(s_menu_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_menu_overlay);
    lv_obj_move_foreground(s_control_bar);
    lv_obj_move_foreground(s_menu_button);
    animate_menu(1000);
}

static void close_menu(void)
{
    if (lv_obj_has_flag(s_menu_overlay, LV_OBJ_FLAG_HIDDEN))
    {
        menu_switch_after_close();
        return;
    }

    animate_menu(0);
}

static void menu_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (s_menu_target_progress == 1000)
    {
        close_menu();
    }
    else
    {
        open_menu();
    }
}

static void menu_overlay_event_cb(lv_event_t *event)
{
    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) &&
        (lv_event_get_target_obj(event) == s_menu_overlay))
    {
        close_menu();
    }
}

static void menu_item_event_cb(lv_event_t *event)
{
    uint8_t index;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    lv_event_stop_bubbling(event);
    index = (uint8_t)(uintptr_t)lv_event_get_user_data(event);
    if (index == s_current_page)
    {
        close_menu();
        return;
    }

    s_pending_page = (int8_t)index;
    close_menu();
}

static void reset_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    s_pending_page = -1;
    close_menu();

    switch ((ui_page_t)s_current_page)
    {
        case UI_PAGE_FREE_DRAW:
            screen_free_draw_clear();
            break;
        case UI_PAGE_BLOCK_CHECK:
            screen_block_check_reset();
            break;
        case UI_PAGE_ACCURACY:
            screen_accuracy_reset();
            break;
        case UI_PAGE_SETTINGS:
            screen_free_draw_clear();
            screen_block_check_reset();
            screen_accuracy_reset();
            ui_info_set(UI_PAGE_SETTINGS, "All test data reset");
            break;
        default:
            break;
    }
}

static void info_bar_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED)
    {
        s_pending_page = -1;
        close_menu();
    }
}

static lv_obj_t *create_footer_button(lv_obj_t *parent, const char *text)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *label;

    lv_obj_set_size(button, UI_FOOTER_BUTTON_WIDTH, UI_FOOTER_HEIGHT);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_remove_flag(button, LV_OBJ_FLAG_SCROLLABLE);

    label = ui_create_label(button, text, &lv_font_montserrat_16, lv_color_white());
    lv_obj_center(label);

    return button;
}

static lv_obj_t *create_menu_item(lv_obj_t *parent, uint8_t index)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *badge = lv_obj_create(button);
    lv_obj_t *badge_label;
    lv_obj_t *text_label;

    lv_obj_set_size(button, LV_PCT(100), 42);
    lv_obj_set_style_bg_color(button, UI_COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_radius(button, 9, 0);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_add_event_cb(button, menu_item_event_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)index);

    lv_obj_set_size(badge, 28, 28);
    lv_obj_set_pos(badge, 8, 7);
    lv_obj_set_style_bg_color(badge, UI_COLOR_ACCENT_SOFT, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(badge, LV_OBJ_FLAG_CLICKABLE);

    badge_label = ui_create_label(badge, s_page_badges[index],
                                  &lv_font_montserrat_16, UI_COLOR_ACCENT);
    lv_obj_center(badge_label);

    text_label = ui_create_label(button, s_page_titles[index],
                                 &lv_font_montserrat_16, UI_COLOR_INK);
    lv_obj_set_pos(text_label, 46, 11);

    return button;
}

static void create_menu(lv_obj_t *root)
{
    lv_obj_t *reset_label;
    lv_obj_t *info_bar;
    lv_obj_t *tab_label;
    uint32_t i;

    s_menu_overlay = lv_obj_create(root);
    lv_obj_set_size(s_menu_overlay, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
    lv_obj_set_pos(s_menu_overlay, 0, 0);
    lv_obj_set_style_bg_opa(s_menu_overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_menu_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_menu_overlay, 0, 0);
    lv_obj_remove_flag(s_menu_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_menu_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_menu_overlay, menu_overlay_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_menu_overlay, LV_OBJ_FLAG_HIDDEN);

    s_menu_panel = lv_obj_create(s_menu_overlay);
    lv_obj_set_size(s_menu_panel, MENU_CLOSED_WIDTH, MENU_CLOSED_HEIGHT);
    lv_obj_align(s_menu_panel, LV_ALIGN_BOTTOM_RIGHT,
                 -UI_FOOTER_MARGIN,
                 -(UI_FOOTER_MARGIN + UI_FOOTER_HEIGHT + UI_FOOTER_GAP));
    ui_apply_card_style(s_menu_panel);
    lv_obj_set_style_pad_all(s_menu_panel, 8, 0);
    lv_obj_set_style_pad_row(s_menu_panel, 6, 0);
    lv_obj_set_style_radius(s_menu_panel, 8, 0);
    lv_obj_set_style_opa(s_menu_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(s_menu_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_menu_panel, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    for (i = 0; i < MENU_ITEM_COUNT; i++)
    {
        s_menu_items[i] = create_menu_item(s_menu_panel, (uint8_t)i);
    }
    menu_refresh_selection();

    s_control_bar = lv_obj_create(root);
    lv_obj_set_size(s_control_bar,
                    UI_SCREEN_WIDTH - (UI_FOOTER_MARGIN * 2),
                    UI_FOOTER_HEIGHT);
    lv_obj_set_pos(s_control_bar, UI_FOOTER_MARGIN, UI_FOOTER_TOP);
    lv_obj_set_style_bg_opa(s_control_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_control_bar, 0, 0);
    lv_obj_set_style_pad_all(s_control_bar, 0, 0);
    lv_obj_remove_flag(s_control_bar, LV_OBJ_FLAG_SCROLLABLE);

    s_reset_button = create_footer_button(s_control_bar, "Reset");
    lv_obj_set_pos(s_reset_button, 0, 0);
    lv_obj_set_style_bg_color(s_reset_button, UI_COLOR_SURFACE, 0);
    lv_obj_set_style_border_color(s_reset_button, UI_COLOR_DANGER, 0);
    lv_obj_set_style_border_width(s_reset_button, 1, 0);
    reset_label = lv_obj_get_child(s_reset_button, 0);
    lv_obj_set_style_text_color(reset_label, UI_COLOR_DANGER, 0);
    lv_obj_add_event_cb(s_reset_button, reset_event_cb, LV_EVENT_CLICKED, NULL);

    info_bar = lv_obj_create(s_control_bar);
    lv_obj_set_size(info_bar, 296, UI_FOOTER_HEIGHT);
    lv_obj_set_pos(info_bar, UI_FOOTER_BUTTON_WIDTH + UI_FOOTER_GAP, 0);
    lv_obj_set_style_bg_color(info_bar, UI_COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(info_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(info_bar, UI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(info_bar, 1, 0);
    lv_obj_set_style_radius(info_bar, 8, 0);
    lv_obj_set_style_pad_all(info_bar, 0, 0);
    lv_obj_remove_flag(info_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(info_bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(info_bar, info_bar_event_cb, LV_EVENT_CLICKED, NULL);

    s_info_label = ui_create_label(info_bar, "", &lv_font_montserrat_16, UI_COLOR_INK);
    lv_obj_set_width(s_info_label, 272);
    lv_obj_set_pos(s_info_label, 12, 8);
    lv_label_set_long_mode(s_info_label, LV_LABEL_LONG_DOT);
    ui_info_bind_label(s_info_label);

    s_menu_button = create_footer_button(s_control_bar, "TAB");
    lv_obj_set_pos(s_menu_button,
                   UI_SCREEN_WIDTH - (UI_FOOTER_MARGIN * 2) - UI_FOOTER_BUTTON_WIDTH,
                   0);
    lv_obj_set_style_bg_color(s_menu_button, UI_COLOR_ACCENT, 0);
    lv_obj_add_event_cb(s_menu_button, menu_button_event_cb, LV_EVENT_CLICKED, NULL);
    tab_label = lv_obj_get_child(s_menu_button, 0);
    lv_obj_set_style_text_color(tab_label, lv_color_white(), 0);
}

void app_ui_init(void)
{
    lv_obj_t *root = lv_screen_active();
    uint32_t i;

    lv_obj_set_style_bg_color(root, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    s_page_layer = lv_obj_create(root);
    lv_obj_set_size(s_page_layer, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
    lv_obj_set_pos(s_page_layer, 0, 0);
    lv_obj_set_style_bg_opa(s_page_layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_page_layer, 0, 0);
    lv_obj_set_style_pad_all(s_page_layer, 0, 0);
    lv_obj_remove_flag(s_page_layer, LV_OBJ_FLAG_SCROLLABLE);

    s_pages[0] = screen_free_draw_create(s_page_layer);
    s_pages[1] = screen_block_check_create(s_page_layer);
    s_pages[2] = screen_accuracy_create(s_page_layer);
    s_pages[3] = screen_settings_create(s_page_layer);

    s_current_page = 0U;
    for (i = 0; i < PAGE_COUNT; i++)
    {
        if (i != s_current_page)
        {
            lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    create_menu(root);
    screen_free_draw_set_line_width(4U);
    menu_refresh_selection();
    ui_info_focus(UI_PAGE_FREE_DRAW);
}
