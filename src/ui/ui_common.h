/**
 * @file ui_common.h
 * @brief UI 内部共用宏：布局尺寸、滑动阈值等；被 `ui_page_*` 与 `ui_events` 引用，main 无需包含。
 *
 * @note 滑动手势的判定公式与约定见 **`.cursor/rules/ui_swipe_gestures.md`**，各页应保持一致。
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
 * 主向「提交」阈值：满行程 = 屏宽或屏高的 NUM/DEN（与 `MY_SCREEN_*` 联动）。
 * 跟手预览 0～1000‰ 映射到本距离；松手时主向 ≥ `UI_SWIPE_COMMIT_DX/DY` 且满足斜率约束则提交。
 */
#define UI_SWIPE_COMMIT_FRAC_NUM  1
#define UI_SWIPE_COMMIT_FRAC_DEN  8
#define UI_SWIPE_COMMIT_DX  ((MY_SCREEN_WIDTH * UI_SWIPE_COMMIT_FRAC_NUM) / UI_SWIPE_COMMIT_FRAC_DEN)
#define UI_SWIPE_COMMIT_DY  ((MY_SCREEN_HEIGHT * UI_SWIPE_COMMIT_FRAC_NUM) / UI_SWIPE_COMMIT_FRAC_DEN)

/**
 * 横向为主手势时：允许的 |dy| 上限 = 屏高 × HNUM/HDEN（抑制斜滑）。
 */
#define UI_SWIPE_CROSS_H_FRAC_NUM  1
#define UI_SWIPE_CROSS_H_FRAC_DEN  12
#define UI_SWIPE_MAX_ABS_DY  ((MY_SCREEN_HEIGHT * UI_SWIPE_CROSS_H_FRAC_NUM) / UI_SWIPE_CROSS_H_FRAC_DEN)

/**
 * 纵向为主手势时：允许的 |dx| 上限 = 屏宽 × VNUM/VDEN（抑制斜滑）。
 */
#define UI_SWIPE_CROSS_V_FRAC_NUM  1
#define UI_SWIPE_CROSS_V_FRAC_DEN  12
#define UI_SWIPE_MAX_ABS_DX  ((MY_SCREEN_WIDTH * UI_SWIPE_CROSS_V_FRAC_NUM) / UI_SWIPE_CROSS_V_FRAC_DEN)

/**
 * 按下后位移超过 屏宽×NUM/DEN（至少 4px）才锁定主滑向，减轻抖动误判。
 */
#define UI_SWIPE_DIR_LOCK_FRAC_NUM  1
#define UI_SWIPE_DIR_LOCK_FRAC_DEN  64
#define UI_SWIPE_DIR_LOCK_PX  LV_MAX(4, (MY_SCREEN_WIDTH * UI_SWIPE_DIR_LOCK_FRAC_NUM) / UI_SWIPE_DIR_LOCK_FRAC_DEN)

/** 文档与回放说明用：有效横向/纵向位移下限，与提交阈值一致。 */
#define UI_SWIPE_MIN_DX  UI_SWIPE_COMMIT_DX
#define UI_SWIPE_MIN_DY  UI_SWIPE_COMMIT_DY

/**
 * 控制中心 sheet 上滑关闭：若设置列表 `scroll_y` 相对按下时变化超过本值，视为用户在滚列表而非关 sheet。
 * 取 屏高 × NUM/DEN，下限 3px。
 */
#define UI_SWIPE_LIST_SCROLL_FRAC_NUM  1
#define UI_SWIPE_LIST_SCROLL_FRAC_DEN  120
#define UI_SWIPE_LIST_SCROLL_EPS  LV_MAX(3, (MY_SCREEN_HEIGHT * UI_SWIPE_LIST_SCROLL_FRAC_NUM) / UI_SWIPE_LIST_SCROLL_FRAC_DEN)

/**
 * 指针设备：超过该像素移动才判为「拖动/滚动」，减轻轻点被 LVGL 判成滚动。
 * 取 min(屏宽,屏高)×NUM/DEN，下限 2px。
 */
#define UI_INDEV_SCROLL_FRAC_NUM  1
#define UI_INDEV_SCROLL_FRAC_DEN  200
#define UI_INDEV_SCROLL_LIMIT_PX  LV_MAX(2, (LV_MIN(MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT) * UI_INDEV_SCROLL_FRAC_NUM) / UI_INDEV_SCROLL_FRAC_DEN)

/** 长按判定时间（ms）：拉大以减少误触长按；普通点击仍走 PRESSED/CLICKED。 */
#ifndef UI_INDEV_LONG_PRESS_MS
#define UI_INDEV_LONG_PRESS_MS  3000
#endif
#ifndef UI_INDEV_LONG_PRESS_REPEAT_MS
#define UI_INDEV_LONG_PRESS_REPEAT_MS  500
#endif

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

/**
 * 模式横条 **卡片** 宽度（px）= 屏宽 × NUM/DEN（下限 180）。
 * 与 `UI_MODE_PAGE_HPAD_*` 联用：单列宽约等于卡宽 + 2×页内留白，窥视邻列时露出的是 **邻卡与图标**，而非宽页内空白。
 */
#define UI_MODE_CARD_W_FRAC_NUM  33
#define UI_MODE_CARD_W_FRAC_DEN  100
#define UI_MODE_CARD_W  LV_MAX(180, (MY_SCREEN_WIDTH * UI_MODE_CARD_W_FRAC_NUM) / UI_MODE_CARD_W_FRAC_DEN)

/** 每列（page）内卡片左右留白（px）= 屏宽 × NUM/DEN（下限 8）。 */
#define UI_MODE_PAGE_HPAD_FRAC_NUM  3
#define UI_MODE_PAGE_HPAD_FRAC_DEN  100
#define UI_MODE_PAGE_HPAD  LV_MAX(8, (MY_SCREEN_WIDTH * UI_MODE_PAGE_HPAD_FRAC_NUM) / UI_MODE_PAGE_HPAD_FRAC_DEN)

/** 横条 Flex 列间距（px）= 屏宽 × NUM/DEN（下限 8）。 */
#define UI_MODE_STRIP_GAP_FRAC_NUM  2
#define UI_MODE_STRIP_GAP_FRAC_DEN  100
#define UI_MODE_STRIP_GAP  LV_MAX(8, (MY_SCREEN_WIDTH * UI_MODE_STRIP_GAP_FRAC_NUM) / UI_MODE_STRIP_GAP_FRAC_DEN)

/** 横条 snap 子项（page）宽度（px）≈ 卡宽 + 两侧页内留白。 */
#define UI_MODE_STRIP_PAGE_W  (UI_MODE_CARD_W + 2 * UI_MODE_PAGE_HPAD)

#endif
