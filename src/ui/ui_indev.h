/**
 * @file ui_indev.h
 * @brief 指针类输入设备参数（与 `ui_common.h` 中 `UI_INDEV_*` 宏一致），在各页 `create` 入口调用一次即可。
 */
#ifndef UI_INDEV_H
#define UI_INDEV_H

/** 按工程约定应用 `UI_INDEV_*`：减轻轻点被误判为拖动、弱化长按。 */
void ui_indev_apply_pointer_profile(void);

#endif
