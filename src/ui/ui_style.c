/**
 * @file ui_style.c
 * @brief 共用标签样式（字体/颜色），供各页面复用，避免重复样式代码。
 */
#include "ui_style.h"
#include "ui_common.h"
#include "ui_font.h"

void ui_style_zone_label(lv_obj_t *label)
{
    const lv_font_t *f = ui_font_cjk();
    lv_obj_set_style_text_font(label, f != NULL ? f : LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
}

void ui_label_i18n_wrap(lv_obj_t *label, lv_coord_t wrap_width)
{
    ui_style_zone_label(label);
#if UI_FEATURE_LABEL_SCROLL
    if(wrap_width > 0) {
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
        lv_obj_set_width(label, wrap_width);
        return;
    }
#endif
    lv_obj_set_style_text_line_space(label, 4, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    if(wrap_width > 0) {
        lv_obj_set_width(label, wrap_width);
    }
}

void ui_region_strip_enable_scroll(lv_obj_t *strip)
{
    lv_obj_add_flag(strip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(strip, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(strip, LV_SCROLLBAR_MODE_AUTO);
}
