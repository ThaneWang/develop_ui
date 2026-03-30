/**
 * @file ui_bt_state.c
 * @brief 蓝牙总开关与低功耗等跨页占位状态（模拟器无真实协议栈）。
 */
#include "ui_bt_state.h"

static bool s_bt_enabled;
static bool s_low_power = true;

/** 查询蓝牙总开关是否开启。 */
bool ui_bt_is_enabled(void)
{
    return s_bt_enabled;
}

/** 设置蓝牙总开关（仅内存占位）。 */
void ui_bt_set_enabled(bool on)
{
    s_bt_enabled = on;
}

/** 查询低功耗策略开关是否开启。 */
bool ui_bt_low_power_is_on(void)
{
    return s_low_power;
}

/** 设置低功耗策略（仅内存占位）。 */
void ui_bt_set_low_power(bool on)
{
    s_low_power = on;
}
