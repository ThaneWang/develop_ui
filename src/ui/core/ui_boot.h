/**
 * @file ui_boot.h
 * @brief 开机动画：在 `my_ui_init()` 中先于主界面展示，结束后 `lv_obj_clean` 再创建主界面。
 */
#ifndef UI_BOOT_H
#define UI_BOOT_H

/** 展示开机动画链，结束后进入 `ui_page_main_create`。 */
void ui_boot_show_then_main(void);

#endif
