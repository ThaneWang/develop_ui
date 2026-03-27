/**
 * @file ui_bt_state.h
 * @brief 蓝牙开关等跨页持久占位状态（模拟器无真实协议）。
 */
#ifndef UI_BT_STATE_H
#define UI_BT_STATE_H

#include <stdbool.h>

bool ui_bt_is_enabled(void);
void ui_bt_set_enabled(bool on);

bool ui_bt_low_power_is_on(void);
void ui_bt_set_low_power(bool on);

#endif
