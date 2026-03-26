/**
 * @file ui_font.h
 * @brief 中文字体入口：静态 `noto_sans_sc_16`；供 `ui_style` 与各页面使用。
 */
#ifndef UI_FONT_H
#define UI_FONT_H

#include "lvgl/lvgl.h"

/** 在 `lv_init()` 之后调用 */
lv_result_t ui_font_init(void);

/** 静态字库无动态资源时可不调用；保留接口便于对称释放 */
void ui_font_uninit(void);

/** 用于中文标签的 `lv_font_t`；未启用 `LV_USE_NOTO_SANS_SC_16_STATIC` 时返回 NULL */
const lv_font_t *ui_font_cjk(void);

#endif
