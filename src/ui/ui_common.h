/**
 * @file ui_common.h
 * @brief UI 内部共用宏：布局尺寸、滑动阈值等；被 `ui_page_*` 与 `ui_events` 引用，main 无需包含。
 *
 * @note 滑动手势的判定公式与约定见 **`.cursor/ui_swipe_gestures.md`**，各页应保持一致。
 */
#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "ui.h"

/** 控制中心宫格磁贴、系统设置列表等分块模板的填充色（青色）；主屏状态/底栏等不再使用该底色 */
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

/** 按下后超过该位移（px）才锁定滑向，避免抖动误判轴向 */
#define UI_SWIPE_DIR_LOCK_PX  10

/**
 * 主界面「进入下一屏」的主向位移阈值：取屏宽/屏高的 1/5（与 `MY_SCREEN_*` 联动）。
 * 跟手预览进度按 0～1000‰ 映射到此距离；松手时主向达到该值且满足斜率约束则生效。
 */
#define UI_SWIPE_COMMIT_FRAC_NUM  1
#define UI_SWIPE_COMMIT_FRAC_DEN  5
#define UI_SWIPE_COMMIT_DX  ((MY_SCREEN_WIDTH * UI_SWIPE_COMMIT_FRAC_NUM) / UI_SWIPE_COMMIT_FRAC_DEN)
#define UI_SWIPE_COMMIT_DY  ((MY_SCREEN_HEIGHT * UI_SWIPE_COMMIT_FRAC_NUM) / UI_SWIPE_COMMIT_FRAC_DEN)

/**
 * 功能开关（可在 CMake 或本文件前 `#define` 覆盖）：
 * - **UI_FEATURE_LABEL_SCROLL**：为 1 时，经 `ui_label_i18n_wrap(label, w)` 且 `w>0` 的 Label 使用 `LV_LABEL_LONG_MODE_SCROLL_CIRCULAR` 横向循环滚动（字条区划文案）。
 * - **UI_FEATURE_DISPLAY_ROTATION**：为 1 时编译控制中心「旋转方向」子页，并调用 `lv_display_set_rotation()`（与 `lv_display.h` 中 `LV_DISPLAY_ROTATION_*` / 刷新方向一致）；为 0 时磁贴 0 仅占位日志。
 */
#ifndef UI_FEATURE_LABEL_SCROLL
#define UI_FEATURE_LABEL_SCROLL 1
#endif
#ifndef UI_FEATURE_DISPLAY_ROTATION
#define UI_FEATURE_DISPLAY_ROTATION 1
#endif

#endif
