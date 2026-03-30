/**
 * @file ui_indev.h
 * @brief 指针类输入设备参数（与 `ui_common.h` 中 `UI_INDEV_*` 宏一致），在各页 `create` 入口调用一次即可。
 *
 * 应用内容：`scroll_limit`（`UI_INDEV_SCROLL_LIMIT_PX`）、`scroll_throw`（`UI_INDEV_SCROLL_THROW_PCT`）、
 * 长按时间。**`scroll_throw` 越大惯性越弱**（LVGL 语义）。PC **SDL** 为 **事件模式**，由 SDL 驱动 `lv_indev_read`；
 * **`continue_reading`** 在事件模式下忽略。
 */
#ifndef UI_INDEV_H
#define UI_INDEV_H

/** 对全部指针 indev 应用 `UI_INDEV_*`（滚动门槛、甩动钝化、长按）。 */
void ui_indev_apply_pointer_profile(void);

#endif
