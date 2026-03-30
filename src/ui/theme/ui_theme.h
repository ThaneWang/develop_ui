/**
 * @file ui_theme.h
 * @brief 全局颜色与透明度约定（单源）；布局尺寸仍在 `ui_common.h`。
 * @note 新页面/子页优先使用本头文件宏，避免散落魔法数。
 */
#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl/lvgl.h"

/** 主屏默认底色 */
#define UI_THEME_SCREEN_BG 0xFFFFFF

/** 控制中心下拉 sheet 背景 */
#define UI_THEME_CC_SHEET_BG 0xE8E8E8
/** 控制中心内全屏子页（系统设置、旋转详情等）浅灰底 */
#define UI_THEME_CC_PAGE_BG 0xF0F0F0
/** 与设置子页一致的返回钮灰 */
#define UI_THEME_CC_BACK_BTN_BG 0xDDDDDD

/** 宫格磁贴 / 设置列表行 / 旋转角度行：浅灰半透明 + 细边框 */
#define UI_THEME_CC_ROW_BG 0xC8C8C8
#define UI_THEME_CC_ROW_BG_OPA LV_OPA_50
#define UI_THEME_CC_ROW_BORDER 0x9E9E9E
#define UI_THEME_CC_ROW_BORDER_W 1

/** 语言行等控件块 */
#define UI_THEME_CC_CTRL_BG 0xEAEAEA
#define UI_THEME_CC_CTRL_BORDER 0xA0A0A0

/** 回放等高亮块（与分区说明色区分） */
#define UI_THEME_ZONE_CYAN 0x00FFFF

#endif
