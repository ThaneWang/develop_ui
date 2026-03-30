/**
 * @file ui_display.h
 * @brief 显示旋转封装：对应 LVGL `lv_display_set_rotation()` 与 `LV_DISPLAY_ROTATION_0/90/180/270`（见 `lvgl/src/display/lv_display.h`）。
 */
#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

#include "lvgl/lvgl.h"

/** 设置默认 display 的旋转枚举（受 `UI_FEATURE_DISPLAY_ROTATION` 控制）。 */
void ui_display_apply_rotation(lv_display_rotation_t rotation);

#endif
