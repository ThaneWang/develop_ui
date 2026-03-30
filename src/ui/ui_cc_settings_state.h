/**
 * @file ui_cc_settings_state.h
 * @brief 控制中心可记忆项：亮度、音量、屏幕旋转意图（与 `ui_sim_settings_persist` 同步）；模拟器占位，量产可接 NVM。
 */
#ifndef UI_CC_SETTINGS_STATE_H
#define UI_CC_SETTINGS_STATE_H

#include <stdbool.h>
#include <stdint.h>

/** 启动时由 `ui_sim_settings_boot_load` 调用：写内存并同步 HAL（背光/音量/旋转），不写盘。 */
void ui_cc_settings_boot_restore(uint8_t brightness_0_100, uint8_t volume_0_100, uint8_t rotation_enabled,
                                 uint8_t rotation_idx_0_3);

uint8_t ui_cc_settings_get_brightness(void);
uint8_t ui_cc_settings_get_volume(void);
bool ui_cc_settings_get_rotation_enabled(void);
uint8_t ui_cc_settings_get_rotation_index(void);

void ui_cc_settings_set_brightness(uint8_t level_0_100);
void ui_cc_settings_set_volume(uint8_t level_0_100);
/** 更新旋转意图、HAL 与持久化文件。 */
void ui_cc_settings_set_rotation(bool enabled, uint8_t idx_0_3);

/** 按当前记忆状态刷新 HAL（主页重建、自回放返回等导航复位旋转后调用）。 */
void ui_cc_settings_apply_rotation_to_hw(void);

#endif
