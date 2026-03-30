/**
 * @file ui_style.c
 * @brief 共用标签样式（字体/颜色），供各页面复用，避免重复样式代码。
 */
#include "ui_style.h"
#include "ui_common.h"
#include "ui_font.h"

static const lv_style_prop_t s_cc_hit_tr_props[] = {
    LV_STYLE_TRANSFORM_SCALE_X,
    LV_STYLE_TRANSFORM_SCALE_Y,
    LV_STYLE_OUTLINE_WIDTH,
    LV_STYLE_OUTLINE_OPA,
    LV_STYLE_PROP_INV,
};

static lv_style_transition_dsc_t s_cc_hit_tr;
static bool s_cc_hit_tr_inited;

static void ui_style_cc_hit_install_transition(lv_obj_t *ctrl)
{
    if(!s_cc_hit_tr_inited) {
        lv_style_transition_dsc_init(&s_cc_hit_tr, s_cc_hit_tr_props, lv_anim_path_ease_out, 120, 0, NULL);
        s_cc_hit_tr_inited = true;
    }
    lv_obj_set_style_transition(ctrl, &s_cc_hit_tr, LV_PART_MAIN);
}

void ui_style_cc_settings_row_apply(lv_obj_t *row)
{
    lv_obj_set_height(row, 48);
    lv_obj_set_style_bg_color(row, lv_color_hex(UI_CC_SETTINGS_ROW_BG), 0);
    lv_obj_set_style_bg_opa(row, UI_CC_SETTINGS_ROW_BG_OPA, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
    lv_obj_set_style_border_width(row, UI_CC_SETTINGS_ROW_BORDER_W, 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_pad_left(row, 8, 0);
    lv_obj_set_style_pad_right(row, 10, 0);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
}

void ui_style_cc_interactive_focus(lv_obj_t *ctrl)
{
    lv_obj_set_style_transform_pivot_x(ctrl, lv_pct(50), LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(ctrl, lv_pct(50), LV_PART_MAIN);
    lv_obj_set_style_transform_scale_x(ctrl, LV_SCALE_NONE, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_y(ctrl, LV_SCALE_NONE, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_x(ctrl, 280, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_transform_scale_y(ctrl, 280, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_transform_scale_x(ctrl, 280, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_transform_scale_y(ctrl, 280, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_transform_scale_x(ctrl, 280, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_transform_scale_y(ctrl, 280, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_outline_width(ctrl, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(ctrl, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_outline_width(ctrl, 2, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_outline_opa(ctrl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_outline_color(ctrl, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_outline_pad(ctrl, 2, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_outline_width(ctrl, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_opa(ctrl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(ctrl, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(ctrl, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(ctrl, 2, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_outline_opa(ctrl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_outline_color(ctrl, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_outline_pad(ctrl, 2, LV_PART_MAIN | LV_STATE_CHECKED);
    ui_style_cc_hit_install_transition(ctrl);
}

/** 区域说明类 Label：中文用 `ui_font_cjk()`，黑色正文。 */
void ui_style_zone_label(lv_obj_t *label)
{
    const lv_font_t *f = ui_font_cjk();
    lv_obj_set_style_text_font(label, f != NULL ? f : LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
}

/** 在 `ui_style_zone_label` 基础上设换行/行距；`wrap_width>0` 时限制折行宽度（或条件编译下循环滚动）。 */
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

/** 条带容器纵向可滚、自动滚动条，避免固定高度内长文被裁切。 */
void ui_region_strip_enable_scroll(lv_obj_t *strip)
{
    lv_obj_add_flag(strip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(strip, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(strip, LV_SCROLLBAR_MODE_AUTO);
}
