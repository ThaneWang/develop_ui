/**
 * @file ui_bt_state.h
 * @brief 蓝牙开关等跨页持久占位状态（模拟器无真实协议）。
 */
#ifndef UI_BT_STATE_H
#define UI_BT_STATE_H

#include <stdbool.h>

/** 蓝牙总开关是否开启（占位）。 */
bool ui_bt_is_enabled(void);
/** 设置蓝牙总开关（占位）。 */
void ui_bt_set_enabled(bool on);

/** 低功耗策略是否开启（占位）。 */
bool ui_bt_low_power_is_on(void);
/** 设置低功耗策略（占位）。 */
void ui_bt_set_low_power(bool on);

/**
 * 仅启动时由持久化层调用：恢复蓝牙与低功耗到内存并 `ui_hw_bluetooth_set_enabled`，不写盘。
 * 产品工程可从 NVM 解析后调用本条。
 */
void ui_bt_state_apply_from_persist(bool bluetooth_on, bool low_power_on);

#endif
