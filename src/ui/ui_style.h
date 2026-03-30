/**
 * @file ui_style.h
 * @brief 声明 `ui_style_zone_label` 等共用样式接口。
 */
#ifndef UI_STYLE_H
#define UI_STYLE_H

#include "lvgl/lvgl.h"

/** 区域说明 Label 的基础字体与颜色。 */
void ui_style_zone_label(lv_obj_t *label);
/**
 * 可 i18n 的区域说明文字：`ui_style_zone_label` + 行距 + `LV_LABEL_LONG_WRAP` + 固定折行宽度。
 * 用于状态栏/底栏/侧栏等；父容器应固定高度并配合 `ui_region_strip_enable_scroll()` 保证超长文案可滚动读完。
 */
void ui_label_i18n_wrap(lv_obj_t *label, lv_coord_t wrap_width);
/** 固定条带内文字可能超高时：纵向滚动 + 自动隐藏滚动条（溢出时出现） */
void ui_region_strip_enable_scroll(lv_obj_t *strip);

#endif
