/**
 * @file ui_nav.h
 * @brief 声明异步页面替换函数，供 `ui_events` 或 `lv_async_call` 使用。
 */
#ifndef UI_NAV_H
#define UI_NAV_H

/** 供 lv_async_call：清屏并切换到对应页面 */
void ui_nav_replace_with_replay_async(void *user_data);
void ui_nav_replace_with_main_async(void *user_data);

#endif
