/**
 * @file ui_bt_nav.h
 * @brief 蓝牙设置全屏页与主页之间的导航（单入口、防重入、与 `ui_nav` 重建主页共用实现）。
 */
#ifndef UI_BT_NAV_H
#define UI_BT_NAV_H

#include <stdbool.h>

/** 从主页进入蓝牙设置（内部 `lv_async_call`，勿在回调里再套一层 async）。 */
void ui_bt_nav_open_async(void);
/** 关闭蓝牙设置并重建主页（内部 async）。 */
void ui_bt_nav_close_async(void);
/** 当前是否为蓝牙设置页（由本模块在创建/销毁时维护）。 */
bool ui_bt_nav_is_bt_page_active(void);
/**
 * 主界面已构建完成时调用，将「在蓝牙页」标志清零。
 * 须由 **`ui_page_main_create`** 开头调用，避免从回放/开机动画等路径进主页后标志残留。
 */
void ui_bt_nav_on_main_shown(void);

#endif
