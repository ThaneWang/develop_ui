/**
 * @file ui_page_bt_settings.c
 * @brief 蓝牙设置页：开关、配对/重连动画占位、低功耗、Wi‑Fi 密码占位（仅日志）。
 */
#include "ui_page_bt_settings.h"
#include "ui_bt_state.h"
#include "ui_common.h"
#include "ui_i18n.h"
#include "ui_nav.h"
#include "ui_style.h"
#include "../logging.h"
#include "lvgl/lvgl.h"

static lv_obj_t *s_pair_spinner;
static lv_obj_t *s_reconn_spinner;
static bool s_pair_active;
static bool s_reconn_active;

static void bt_back_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_async_call(ui_nav_replace_with_main_async, NULL);
}

static void bt_master_sw_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    const bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    ui_bt_set_enabled(on);
    printf("蓝牙: %s\n", on ? "开启" : "关闭");
    LOG_DEBUG("BT master %s", on ? "on" : "off");
}

static void bt_pair_row_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(s_pair_spinner == NULL || !lv_obj_is_valid(s_pair_spinner)) {
        return;
    }
    s_pair_active = !s_pair_active;
    if(s_pair_active) {
        lv_obj_clear_flag(s_pair_spinner, LV_OBJ_FLAG_HIDDEN);
        printf("蓝牙配对中\n");
        LOG_DEBUG("BT pairing...");
    }
    else {
        lv_obj_add_flag(s_pair_spinner, LV_OBJ_FLAG_HIDDEN);
        printf("蓝牙配对已停止\n");
        LOG_DEBUG("BT pairing stopped");
    }
}

static void bt_reconn_row_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(s_reconn_spinner == NULL || !lv_obj_is_valid(s_reconn_spinner)) {
        return;
    }
    s_reconn_active = !s_reconn_active;
    if(s_reconn_active) {
        lv_obj_clear_flag(s_reconn_spinner, LV_OBJ_FLAG_HIDDEN);
        printf("蓝牙重连中\n");
        LOG_DEBUG("BT reconnect...");
    }
    else {
        lv_obj_add_flag(s_reconn_spinner, LV_OBJ_FLAG_HIDDEN);
        printf("蓝牙重连已停止\n");
        LOG_DEBUG("BT reconnect stopped");
    }
}

static void bt_low_power_sw_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    const bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    ui_bt_set_low_power(on);
    printf("低功耗策略: %s\n", on ? "开" : "关");
    LOG_DEBUG("BT low power %s", on ? "on" : "off");
}

static void bt_wifi_row_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    printf("设置wifi密码\n");
    LOG_DEBUG("BT WiFi set password (placeholder)");
}

static void bt_row_focus_style(lv_obj_t *row)
{
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(row, 8, LV_PART_MAIN);
}

static lv_obj_t *bt_add_row(lv_obj_t *parent, bool clickable)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, 52);
    lv_obj_set_style_bg_color(row, lv_color_hex(UI_ZONE_CYAN), 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    if(clickable) {
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        bt_row_focus_style(row);
    }
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(row, 12, 0);
    return row;
}

void ui_page_bt_settings_create(lv_obj_t *scr)
{
    s_pair_spinner = NULL;
    s_reconn_spinner = NULL;
    s_pair_active = false;
    s_reconn_active = false;

    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_pad_all(scr, 12, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(scr, 10, 0);

    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_width(hdr, LV_PCT(100));
    lv_obj_set_height(hdr, 48);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hdr, 12, 0);

    lv_obj_t *back_btn = lv_obj_create(hdr);
    lv_obj_set_size(back_btn, 72, 40);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_remove_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back_btn, bt_back_cb, LV_EVENT_CLICKED, NULL);
    bt_row_focus_style(back_btn);
    lv_obj_t *back_l = lv_label_create(back_btn);
    ui_i18n_bind_label(back_l, UI_STR_SETTINGS_BACK);
    ui_style_zone_label(back_l);
    lv_obj_center(back_l);

    lv_obj_t *title = lv_label_create(hdr);
    ui_i18n_bind_label(title, UI_STR_BT_PAGE_TITLE);
    ui_style_zone_label(title);
    lv_obj_set_flex_grow(title, 1);

    /* 总开关 */
    lv_obj_t *row_master = bt_add_row(scr, false);
    lv_obj_t *lb_m = lv_label_create(row_master);
    ui_i18n_bind_label(lb_m, UI_STR_BT_MASTER_SWITCH);
    ui_style_zone_label(lb_m);
    lv_obj_set_flex_grow(lb_m, 1);

    lv_obj_t *sw_m = lv_switch_create(row_master);
    if(ui_bt_is_enabled()) {
        lv_obj_add_state(sw_m, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sw_m, bt_master_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 配对 */
    lv_obj_t *row_p = bt_add_row(scr, true);
    lv_obj_add_event_cb(row_p, bt_pair_row_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb_p = lv_label_create(row_p);
    ui_i18n_bind_label(lb_p, UI_STR_BT_PAIR);
    ui_style_zone_label(lb_p);
    lv_obj_set_flex_grow(lb_p, 1);
    s_pair_spinner = lv_spinner_create(row_p);
    lv_obj_set_size(s_pair_spinner, 36, 36);
    lv_obj_remove_flag(s_pair_spinner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_pair_spinner, LV_OBJ_FLAG_HIDDEN);

    /* 重连 */
    lv_obj_t *row_r = bt_add_row(scr, true);
    lv_obj_add_event_cb(row_r, bt_reconn_row_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb_r = lv_label_create(row_r);
    ui_i18n_bind_label(lb_r, UI_STR_BT_RECONNECT);
    ui_style_zone_label(lb_r);
    lv_obj_set_flex_grow(lb_r, 1);
    s_reconn_spinner = lv_spinner_create(row_r);
    lv_obj_set_size(s_reconn_spinner, 36, 36);
    lv_obj_remove_flag(s_reconn_spinner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_reconn_spinner, LV_OBJ_FLAG_HIDDEN);

    /* 低功耗策略 */
    lv_obj_t *row_lp = bt_add_row(scr, false);
    lv_obj_t *lb_lp = lv_label_create(row_lp);
    ui_i18n_bind_label(lb_lp, UI_STR_BT_LOW_POWER);
    ui_style_zone_label(lb_lp);
    lv_obj_set_flex_grow(lb_lp, 1);
    lv_obj_t *sw_lp = lv_switch_create(row_lp);
    if(ui_bt_low_power_is_on()) {
        lv_obj_add_state(sw_lp, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sw_lp, bt_low_power_sw_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Wi‑Fi SSID / 密码（占位） */
    lv_obj_t *row_w = bt_add_row(scr, true);
    lv_obj_add_event_cb(row_w, bt_wifi_row_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb_w = lv_label_create(row_w);
    ui_i18n_bind_label(lb_w, UI_STR_BT_WIFI_SET);
    ui_style_zone_label(lb_w);
    lv_label_set_long_mode(lb_w, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lb_w, LV_PCT(92));

    LOG_DEBUG("Bluetooth settings page created");
}
