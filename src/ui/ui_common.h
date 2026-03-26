/**
 * @file ui_common.h
 * @brief UI 内部共用宏：布局尺寸、滑动阈值等；被 `ui_page_*` 与 `ui_events` 引用，main 无需包含。
 *
 * @note 滑动手势的判定公式与约定见 **`.cursor/ui_swipe_gestures.md`**，各页应保持一致。
 */
#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "ui.h"

/** 状态栏/底栏等分区背景色（青色，RGB 0x00FFFF） */
#define UI_ZONE_CYAN          0x00FFFF
/** 顶部状态栏高度（px） */
#define UI_TOP_BAR_H          50
/** 底部工具栏高度（px） */
#define UI_BOTTOM_BAR_H       72
/** 主界面左右侧栏（预留区 / ISP）宽度（px） */
#define UI_SIDE_PANEL_W       120

/**
 * 水平滑动：有效位移下限（px）。
 * 与 `UI_SWIPE_MAX_ABS_DY` 联用：判定为「横向滑」时，要求 |dx| >= 本值。
 */
#define UI_SWIPE_MIN_DX       32
/**
 * 水平滑动：允许的最大纵向偏移（px）。
 * 用于排除斜滑：横向手势要求 |dy| <= 本值，避免与下拉/上滑冲突。
 */
#define UI_SWIPE_MAX_ABS_DY   48

/**
 * 垂直滑动：有效位移下限（px）。
 * 与 `UI_SWIPE_MAX_ABS_DX` 联用：判定为「纵向滑」时，要求 |dy| >= 本值。
 */
#define UI_SWIPE_MIN_DY       40
/**
 * 垂直滑动：允许的最大横向偏移（px）。
 * 用于排除斜滑：纵向手势要求 |dx| <= 本值。
 */
#define UI_SWIPE_MAX_ABS_DX   56

#endif
