/**
 * @file ui_common.h
 * @brief UI 内部共用宏：布局尺寸、滑动阈值等；被 `ui_page_*` 与 `ui_events` 引用，main 无需包含。
 *
 * @note 滑动手势的判定公式与约定见 **`.cursor/rules/ui_swipe_gestures.md`**，各页应保持一致。
 */
#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "ui.h"
#include "ui_theme.h"

/** 高亮青（回放页按钮/时长条等）；控制中心分块色见 **`UI_THEME_*` / `UI_CC_SETTINGS_ROW_*`** */
#define UI_ZONE_CYAN UI_THEME_ZONE_CYAN
/** 控制中心宫格磁贴、旋转角度行、系统设置列表行（含语言行）：与 `ui_theme.h` 一致 */
#define UI_CC_SETTINGS_ROW_BG UI_THEME_CC_ROW_BG
#define UI_CC_SETTINGS_ROW_BG_OPA UI_THEME_CC_ROW_BG_OPA
#define UI_CC_SETTINGS_ROW_BORDER UI_THEME_CC_ROW_BORDER
#define UI_CC_SETTINGS_ROW_BORDER_W UI_THEME_CC_ROW_BORDER_W
/** 语言下拉与行内控件 */
#define UI_CC_SETTINGS_CTRL_BG UI_THEME_CC_CTRL_BG
#define UI_CC_SETTINGS_CTRL_BORDER UI_THEME_CC_CTRL_BORDER
/** 顶部状态栏高度（px） */
#define UI_TOP_BAR_H          50
/**
 * 顶栏右侧容器（蓝牙字条 / 模式符号 / 电量）最小宽度（px）。
 * 与 `flex_grow` 配合占满「状态栏剩余宽度」；勿过小，否则中文词条与后续多图标易裁切。
 */
#ifndef UI_STATUS_BAR_RIGHT_MIN_W
#define UI_STATUS_BAR_RIGHT_MIN_W  220
#endif
/** 底部工具栏高度（px） */
#define UI_BOTTOM_BAR_H       72
/** 主界面左右侧栏（预留区 / ISP）宽度（px） */
#define UI_SIDE_PANEL_W       120

/**
 * 主向「提交」阈值：满行程 = 屏宽或屏高的 NUM/DEN（与 `MY_SCREEN_*` 联动）。
 * 跟手预览 0～1000‰ 映射到本距离；松手时主向 ≥ `UI_SWIPE_COMMIT_DX/DY` 且满足斜率约束则提交。
 * **阻抗**：DEN 越小需拖得越远（原 8 偏敏）；默认 **5** ≈ 五分之一屏，减轻轻触即出页。
 */
#define UI_SWIPE_COMMIT_FRAC_NUM  1
#define UI_SWIPE_COMMIT_FRAC_DEN  5
#define UI_SWIPE_COMMIT_DX  ((MY_SCREEN_WIDTH * UI_SWIPE_COMMIT_FRAC_NUM) / UI_SWIPE_COMMIT_FRAC_DEN)
#define UI_SWIPE_COMMIT_DY  ((MY_SCREEN_HEIGHT * UI_SWIPE_COMMIT_FRAC_NUM) / UI_SWIPE_COMMIT_FRAC_DEN)

/**
 * 横向为主手势时：允许的 |dy| 上限 = 屏高 × HNUM/HDEN（抑制斜滑）。
 * **略收紧**（相对 12）使斜向更难误触竖向页。
 */
#define UI_SWIPE_CROSS_H_FRAC_NUM  1
#define UI_SWIPE_CROSS_H_FRAC_DEN  14
#define UI_SWIPE_MAX_ABS_DY  ((MY_SCREEN_HEIGHT * UI_SWIPE_CROSS_H_FRAC_NUM) / UI_SWIPE_CROSS_H_FRAC_DEN)

/**
 * 纵向为主手势时：允许的 |dx| 上限 = 屏宽 × VNUM/VDEN（抑制斜滑）。
 */
#define UI_SWIPE_CROSS_V_FRAC_NUM  1
#define UI_SWIPE_CROSS_V_FRAC_DEN  14
#define UI_SWIPE_MAX_ABS_DX  ((MY_SCREEN_WIDTH * UI_SWIPE_CROSS_V_FRAC_NUM) / UI_SWIPE_CROSS_V_FRAC_DEN)

/**
 * 按下后位移超过 屏宽×NUM/DEN（至少 10px）才锁定主滑向，减轻抖动误判；**略增**可体感「晚一点才跟手」。
 */
#define UI_SWIPE_DIR_LOCK_FRAC_NUM  1
#define UI_SWIPE_DIR_LOCK_FRAC_DEN  40
#define UI_SWIPE_DIR_LOCK_PX  LV_MAX(10, (MY_SCREEN_WIDTH * UI_SWIPE_DIR_LOCK_FRAC_NUM) / UI_SWIPE_DIR_LOCK_FRAC_DEN)

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
 * 下拉控制中心 / 上滑模式页：松手时若跟手露出 ≥ 本千分比则 **ease_out** 吸附完全展开，否则缓动收回（避免须拉满 `UI_SWIPE_COMMIT_*` 才打开）。
 */
#ifndef UI_PANEL_SNAP_OPEN_PERMILLE
#define UI_PANEL_SNAP_OPEN_PERMILLE 500
#endif
/** 面板边缘吸附动画：按 **全行程** 计的基准 ms；实际时长 = 本值 ×（剩余像素 / 参考行程），再钳到 MIN/MAX。 */
#ifndef UI_PANEL_EDGE_ANIM_MS
#define UI_PANEL_EDGE_ANIM_MS 260
#endif
#ifndef UI_PANEL_EDGE_ANIM_MS_MIN
#define UI_PANEL_EDGE_ANIM_MS_MIN 120
#endif
#ifndef UI_PANEL_EDGE_ANIM_MS_MAX
#define UI_PANEL_EDGE_ANIM_MS_MAX 340
#endif

/**
 * 指针设备滚动触发门槛（px，`lv_indev_set_scroll_limit`）：手指移动超过该距离后 LVGL 才从「点击」转为「拖动/滚动」。
 * **钝化/延迟感**：提高到 **22～28** 可减少轻移即滚列表/模式条；与全屏手势 `UI_SWIPE_DIR_LOCK_*` 分工不同（后者在 `ui_page_main`）。
 */
#ifndef UI_INDEV_SCROLL_LIMIT_PX
#define UI_INDEV_SCROLL_LIMIT_PX  24
#endif

/**
 * 指针甩动衰减（0～99，`lv_indev_set_scroll_throw`）。**LVGL 语义**（见 `lv_indev.c` 注释）：
 * **数值越大 → 减速越快 → 惯性越弱、滑行越短**；**数值越小 → 松手后越能「飘」**。
 * 网传「throw 越大惯性越大」与 LVGL 9 实现相反，请勿混淆。
 * 默认 **12**：比 **16** 略长滑行，贴近「有惯性」；**≥22** 则几乎立刻停。
 */
#ifndef UI_INDEV_SCROLL_THROW_PCT
#define UI_INDEV_SCROLL_THROW_PCT  12
#endif

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
 */
#ifndef UI_FEATURE_LABEL_SCROLL
#define UI_FEATURE_LABEL_SCROLL 1
#endif

/**
 * 模式横条 **卡片** 宽度（px）= 屏宽 × NUM/DEN（下限 100）。
 * 目标约 **一屏可见 5 个模式**（800 宽时 `page_w`≈152、`gap`≈8 → 5 列 + 间距 ≈ 792px）；与 `UI_MODE_PAGE_HPAD_*` 联用得 `UI_MODE_STRIP_PAGE_W`。
 */
#define UI_MODE_CARD_W_FRAC_NUM  17
#define UI_MODE_CARD_W_FRAC_DEN  100
#define UI_MODE_CARD_W  LV_MAX(100, (MY_SCREEN_WIDTH * UI_MODE_CARD_W_FRAC_NUM) / UI_MODE_CARD_W_FRAC_DEN)

/** 每列（page）内卡片左右留白（px）= 屏宽 × NUM/DEN（下限 6）。 */
#define UI_MODE_PAGE_HPAD_FRAC_NUM  1
#define UI_MODE_PAGE_HPAD_FRAC_DEN  100
#define UI_MODE_PAGE_HPAD  LV_MAX(6, (MY_SCREEN_WIDTH * UI_MODE_PAGE_HPAD_FRAC_NUM) / UI_MODE_PAGE_HPAD_FRAC_DEN)

/** 横条 Flex 列间距（px）= 屏宽 × NUM/DEN（下限 4）。 */
#define UI_MODE_STRIP_GAP_FRAC_NUM  1
#define UI_MODE_STRIP_GAP_FRAC_DEN  100
#define UI_MODE_STRIP_GAP  LV_MAX(4, (MY_SCREEN_WIDTH * UI_MODE_STRIP_GAP_FRAC_NUM) / UI_MODE_STRIP_GAP_FRAC_DEN)

/** 横条 snap 子项（page）宽度（px）≈ 卡宽 + 两侧页内留白。 */
#define UI_MODE_STRIP_PAGE_W  (UI_MODE_CARD_W + 2 * UI_MODE_PAGE_HPAD)

/** 横条单列总高度（px）：缩小图标框后与卡宽配套（图标 `montserrat_28` + 两行文案）。 */
#define UI_MODE_STRIP_ROW_H  LV_MAX(118, (MY_SCREEN_WIDTH * 16) / 100)

/** 模式页顶栏（标题+关闭）高度（px），与 `ui_page_main.c` 中 `hdr` 一致；其下为宽触区竖向范围。 */
#ifndef UI_MODE_PANEL_HEADER_H
#define UI_MODE_PANEL_HEADER_H  52
#endif
/**
 * 模式页宽触区与横条 **单列几何** 对齐：左右各 **`2 × (UI_MODE_STRIP_PAGE_W + UI_MODE_STRIP_GAP)`**（对应图示 **左/右各两列**），
 * 左 **第一列** → 中心槽 −2、**第二列** → −1；右对称为 +1 / +2。左区 **纵贯整屏高**；`main_create_mode_panel` 末尾 **`lv_obj_move_foreground(hdr)`** 使顶栏盖在左区上，保留标题栏下滑关闭。
 */

/**
 * 模式卡片选中态（边框/底色）样式过渡时长（ms，`lv_style_transition_dsc_init`）。
 * **与甩动惯性区分**：`ui_app_set_shoot_mode` 在 `SCROLL_END` **立即**生效并刷新顶栏/底栏；卡片高亮靠本过渡 **渐显**，形成「数据立即、视觉约本时长」。
 */
#ifndef UI_MODE_CARD_SELECT_TRANSITION_MS
#define UI_MODE_CARD_SELECT_TRANSITION_MS  200
#endif

/**
 * 模式横条 **程序化滚动**（snap 补全、`scroll_to_view`）在 `LV_EVENT_SCROLL_BEGIN` 里对 `lv_anim` 做 clamp：
 * 略长于 LVGL 默认 `SCROLL_ANIM_TIME_MIN/MAX`（200～400ms），使吸附有 **阻尼感**；与 `UI_MODE_CARD_SELECT_TRANSITION_MS` 分工不同（卡片样式 vs 滚动位移）。
 */
#ifndef UI_MODE_SNAP_SCROLL_MIN_MS
#define UI_MODE_SNAP_SCROLL_MIN_MS  260
#endif
#ifndef UI_MODE_SNAP_SCROLL_MAX_MS
#define UI_MODE_SNAP_SCROLL_MAX_MS  520
#endif
/** 在 LVGL 计算出的 `scroll_by` 时长基础上乘 NUM/DEN（例如 115/100 ≈ +15%） */
#ifndef UI_MODE_SNAP_SCROLL_DURATION_PCT_NUM
#define UI_MODE_SNAP_SCROLL_DURATION_PCT_NUM  115
#endif
#ifndef UI_MODE_SNAP_SCROLL_DURATION_PCT_DEN
#define UI_MODE_SNAP_SCROLL_DURATION_PCT_DEN  100
#endif

#endif
