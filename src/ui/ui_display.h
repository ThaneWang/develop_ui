/**
 * @file ui_display.h
 * @brief 屏幕物理旋转占位接口；与 LVGL 解耦，后续在 `ui_display.c` 内对接 BSP / `lv_display_set_rotation` 等。
 */
#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

/** 相对默认竖向的顺时针步进（0° / 90° / 180° / 270°），供产品层与驱动对齐。 */
typedef enum {
    UI_SCREEN_ROT_CW_0 = 0,
    UI_SCREEN_ROT_CW_90,
    UI_SCREEN_ROT_CW_180,
    UI_SCREEN_ROT_CW_270,
} ui_screen_rotation_t;

/** 应用屏幕旋转；当前为空实现，导入目标机显示栈时在此集中实现。 */
void ui_display_set_screen_rotation(ui_screen_rotation_t rot);

#endif
