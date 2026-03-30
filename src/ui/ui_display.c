/**
 * @file ui_display.c
 * @brief 屏幕旋转：经 `ui_hw_display_rotation_set` 统一走外设占位；后续在 `ui_hw_hal.c` 接 LVGL/BSP。
 */
#include "ui_display.h"

#include "ui_hw_hal.h"

void ui_display_set_screen_rotation(ui_screen_rotation_t rot)
{
    ui_hw_display_rotation_set(true, rot);
}
