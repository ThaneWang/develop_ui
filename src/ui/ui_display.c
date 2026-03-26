/**
 * @file ui_display.c
 * @brief `lv_display_set_rotation` 封装；旋转后 LVGL 会交换逻辑分辨率并 `LV_EVENT_SIZE_CHANGED` 根屏。
 */
#include "ui_common.h"
#include "ui_display.h"

void ui_display_apply_rotation(lv_display_rotation_t rotation)
{
#if UI_FEATURE_DISPLAY_ROTATION
    lv_display_t *d = lv_display_get_default();
    if(d == NULL) {
        return;
    }
    lv_display_set_rotation(d, rotation);
#else
    (void)rotation;
#endif
}
