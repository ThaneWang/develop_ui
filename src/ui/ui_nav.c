/**
 * @file ui_nav.c
 * @brief 全屏页面切换：`lv_obj_clean` + `ui_page_*_create`，供 `lv_async_call` 与事件层调用，避免在事件回调内直接清屏。
 */
#include "ui_nav.h"
#include "ui_i18n.h"
#include "ui_page_main.h"
#include "ui_page_replay.h"
#include "ui_display.h"
#include "../logging.h"
#include "lvgl/lvgl.h"

void ui_nav_replace_with_replay_async(void *user_data)
{
    LV_UNUSED(user_data);
    lv_obj_t *scr = lv_scr_act();
    ui_display_apply_rotation(LV_DISPLAY_ROTATION_0);
    ui_i18n_reset_bindings();
    lv_obj_clean(scr);
    ui_page_replay_create(scr);
}

void ui_nav_replace_with_main_async(void *user_data)
{
    LV_UNUSED(user_data);
    lv_obj_t *scr = lv_scr_act();
    ui_i18n_reset_bindings();
    lv_obj_clean(scr);
    ui_page_main_create(scr);
    LOG_DEBUG("主页已恢复（与进入回放前一致）");
}
