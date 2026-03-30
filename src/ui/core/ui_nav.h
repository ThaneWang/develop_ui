/**
 * @file ui_nav.h
 * @brief 声明异步页面替换函数，供 `ui_events` 或 `lv_async_call` 使用。
 */
#ifndef UI_NAV_H
#define UI_NAV_H

#include "lvgl/lvgl.h"

/** 当前活动屏上清子树并 **`ui_page_main_create`**（非 async；供蓝牙关闭等与「回主页」一致）。 */
void ui_nav_rebuild_main(lv_obj_t *scr);

/** 供 lv_async_call：清屏并切换到对应页面 */
void ui_nav_replace_with_replay_async(void *user_data);
void ui_nav_replace_with_main_async(void *user_data);
void ui_nav_replace_with_bt_settings_async(void *user_data);

#endif
