/**
 * @file ui_indev.c
 * @brief 遍历 LVGL 指针输入设备并写入 `UI_INDEV_*`（不修改 LVGL 源码）。
 */
#include "ui_indev.h"
#include "ui_common.h"
#include "lvgl/lvgl.h"

void ui_indev_apply_pointer_profile(void)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while(indev != NULL) {
        if(lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            lv_indev_set_long_press_time(indev, (uint16_t)UI_INDEV_LONG_PRESS_MS);
            lv_indev_set_long_press_repeat_time(indev, (uint16_t)UI_INDEV_LONG_PRESS_REPEAT_MS);
            {
                const int lim = UI_INDEV_SCROLL_LIMIT_PX;
                const uint8_t lim_u8 = (lim < 1) ? 1u : ((lim > 255) ? 255u : (uint8_t)lim);
                lv_indev_set_scroll_limit(indev, lim_u8);
            }
        }
        indev = lv_indev_get_next(indev);
    }
}
