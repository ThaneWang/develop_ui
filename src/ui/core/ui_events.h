/**
 * @file ui_events.h
 * @brief 声明 `ui_evt_*` 回调，供页面在创建控件时绑定。
 */
#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#include "lvgl/lvgl.h"

/** 调试：将 `user_data` 区划名打印到控制台与日志。 */
void ui_evt_zone_click_cb(lv_event_t *e);
/** 回放页：左滑返回主页（阈值见 `ui_common.h` 与 `.cursor/ui_swipe_gestures.md`） */
void ui_evt_replay_zuohua_pointer_cb(lv_event_t *e);

#endif
