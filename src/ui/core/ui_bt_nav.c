/**
 * @file ui_bt_nav.c
 * @brief 蓝牙设置 ↔ 主页：独立状态位、防重复排队、关闭时走 **`ui_nav_rebuild_main`** 与回放/左滑回主页一致。
 */
#include "ui_bt_nav.h"

#include "lvgl/lvgl.h"
#include "ui_nav.h"
#include "ui_page_bt_settings.h"
#include "ui_page_main.h"
#include "ui_app_state.h"
#include "ui_hw_hal.h"
#include "ui_i18n.h"
#include "../../logging.h"

static bool s_on_bt_page;
static bool s_open_inflight;
static bool s_close_inflight;

bool ui_bt_nav_is_bt_page_active(void)
{
    return s_on_bt_page;
}

void ui_bt_nav_on_main_shown(void)
{
    s_on_bt_page = false;
    s_open_inflight = false;
    s_close_inflight = false;
}

static void ui_bt_nav_open_task(void *user_data)
{
    LV_UNUSED(user_data);
    s_open_inflight = false;
    if(s_on_bt_page) {
        return;
    }
    lv_obj_t *scr = lv_scr_act();
    ui_page_main_scr_detach_gestures(scr);
    ui_hw_display_rotation_set(false, UI_SCREEN_ROT_CW_0);
    ui_app_shoot_mode_observer_unregister_all();
    ui_i18n_reset_bindings();
    lv_obj_clean(scr);
    lv_indev_reset(NULL, NULL);
    ui_page_bt_settings_create(scr);
    s_on_bt_page = true;
    LOG_DEBUG("ui_bt_nav: opened BT settings");
}

void ui_bt_nav_open_async(void)
{
    if(s_on_bt_page || s_open_inflight || s_close_inflight) {
        return;
    }
    s_open_inflight = true;
    lv_async_call(ui_bt_nav_open_task, NULL);
}

static void ui_bt_nav_close_task(void *user_data)
{
    LV_UNUSED(user_data);
    s_close_inflight = false;
    if(!s_on_bt_page) {
        return;
    }
    s_on_bt_page = false;
    ui_nav_rebuild_main(lv_scr_act());
    LOG_DEBUG("ui_bt_nav: closed -> main");
}

void ui_bt_nav_close_async(void)
{
    if(!s_on_bt_page || s_close_inflight || s_open_inflight) {
        return;
    }
    s_close_inflight = true;
    lv_async_call(ui_bt_nav_close_task, NULL);
}
