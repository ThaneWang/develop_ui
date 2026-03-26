/**
 * @file ui_style.c
 * @brief 共用标签样式（字体/颜色），供各页面复用，避免重复样式代码。
 */
#include "ui_style.h"
#include "ui_font.h"

void ui_style_zone_label(lv_obj_t *label)
{
    const lv_font_t *f = ui_font_cjk();
    lv_obj_set_style_text_font(label, f != NULL ? f : LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
}
