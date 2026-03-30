/**
 * @file ui_page_bt_settings.c
 * @brief 蓝牙设置页：开关、配对/重连动画占位、低功耗、Wi‑Fi 密码占位（仅日志）。
 * 全屏进/出由 **`ui_bt_nav`** 统一调度；返回勿直接 `lv_async_call(ui_nav_replace_with_main_async)`。
 */
#include "ui_page_bt_settings.h"
#include "ui_bt_state.h"
#include "ui_common.h"
#include "ui_indev.h"
#include "ui_i18n.h"
#include "ui_bt_nav.h"
#include "ui_style.h"
#include "../../logging.h"
#include "lvgl/lvgl.h"

static lv_obj_t *s_pair_spinner;
static lv_obj_t *s_reconn_spinner;
static bool s_pair_active;
static bool s_reconn_active;

/** 返回按钮：走蓝牙导航模块关闭（内部 async，防与打开任务交错）。 */
static void bt_back_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_bt_nav_close_async();
}

/** 总开关：`VALUE_CHANGED` 时写 `ui_bt_state`（经 `ui_hw_bluetooth_set_enabled` 打占位日志）。 */
static void bt_master_sw_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    const bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    ui_bt_set_enabled(on);
}

/** 配对行点击：切换配对动画占位显隐。 */
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

/** 重连行点击：切换重连动画占位显隐。 */
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

/** 低功耗开关：`VALUE_CHANGED` 时写 `ui_bt_set_low_power`。 */
static void bt_low_power_sw_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    const bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    ui_bt_set_low_power(on);
    printf("低功耗策略: %s\n", on ? "开" : "关");
    LOG_DEBUG("BT low power %s", on ? "on" : "off");
}

/** Wi‑Fi 密码占位行：仅打印日志。 */
static void bt_wifi_row_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    printf("设置wifi密码\n");
    LOG_DEBUG("BT WiFi set password (placeholder)");
}

/** 在列表容器中追加一行（与控制中心系统设置列表行一致）；`clickable` 为真时注册交互描边/缩放。 */
static lv_obj_t *bt_add_row(lv_obj_t *parent, bool clickable)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    ui_style_cc_settings_row_apply(row);
    if(clickable) {
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        ui_style_cc_interactive_focus(row);
    }
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return row;
}

/** 构建蓝牙设置全屏：顶栏、总开关、配对/重连/低功耗/Wi‑Fi 占位行。 */
void ui_page_bt_settings_create(lv_obj_t *scr)
{
    s_pair_spinner = NULL;
    s_reconn_spinner = NULL;
    s_pair_active = false;
    s_reconn_active = false;
    ui_indev_apply_pointer_profile();

    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_THEME_CC_PAGE_BG), 0);
    lv_obj_set_style_pad_all(scr, 8, 0);
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
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(UI_THEME_CC_BACK_BTN_BG), 0);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_remove_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back_btn, bt_back_cb, LV_EVENT_CLICKED, NULL);
    ui_style_cc_interactive_focus(back_btn);
    lv_obj_t *back_l = lv_label_create(back_btn);
    ui_i18n_bind_label(back_l, UI_STR_SETTINGS_BACK);
    lv_label_set_long_mode(back_l, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(back_l);
    lv_obj_center(back_l);

    lv_obj_t *title = lv_label_create(hdr);
    ui_i18n_bind_label(title, UI_STR_BT_PAGE_TITLE);
    lv_label_set_long_mode(title, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(title);
    lv_obj_set_flex_grow(title, 1);

    lv_obj_t *set_list = lv_obj_create(scr);
    lv_obj_set_width(set_list, LV_PCT(100));
    lv_obj_set_flex_grow(set_list, 1);
    lv_obj_set_style_bg_opa(set_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(set_list, 0, 0);
    lv_obj_add_flag(set_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(set_list, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scroll_dir(set_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(set_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_layout(set_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(set_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(set_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(set_list, 8, 0);

    /* 总开关 */
    lv_obj_t *row_master = bt_add_row(set_list, false);
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
    lv_obj_t *row_p = bt_add_row(set_list, true);
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
    lv_obj_t *row_r = bt_add_row(set_list, true);
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
    lv_obj_t *row_lp = bt_add_row(set_list, false);
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
    lv_obj_t *row_w = bt_add_row(set_list, true);
    lv_obj_add_event_cb(row_w, bt_wifi_row_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb_w = lv_label_create(row_w);
    ui_i18n_bind_label(lb_w, UI_STR_BT_WIFI_SET);
    ui_style_zone_label(lb_w);
    lv_label_set_long_mode(lb_w, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lb_w, LV_PCT(92));

    LOG_DEBUG("Bluetooth settings page created");
}
