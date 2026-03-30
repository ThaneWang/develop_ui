/**
 * @file ui_hw_hal.h
 * @brief 外设/驱动侧统一占位接口（搭建 UI 阶段仅 `printf`/`LOG_DEBUG`）；量产前在此对接 BSP。
 */
#ifndef UI_HW_HAL_H
#define UI_HW_HAL_H

#include <stdbool.h>
#include <stdint.h>

#include "ui_app_state.h"
#include "ui_display.h"

/**
 * 屏幕旋转意图：enabled=false 表示关闭旋转（恢复 0°）；true 时按顺时针象限 rot 应用。
 */
void ui_hw_display_rotation_set(bool enabled, ui_screen_rotation_t rot);

/** 蓝牙射频/协议栈开关占位。 */
void ui_hw_bluetooth_set_enabled(bool on);

/** 背光 0～100 占位。 */
void ui_hw_brightness_set(uint8_t level_0_100);

/** 音量 0～100 占位。 */
void ui_hw_volume_set(uint8_t level_0_100);

/** 拍摄模式写入产品侧后的驱动占位（与 `ui_app_set_shoot_mode` 成功路径联动）。 */
void ui_hw_shoot_mode_apply(ui_shoot_mode_t m);

#endif
