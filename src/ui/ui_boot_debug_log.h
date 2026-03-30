/**
 * @file ui_boot_debug_log.h
 * @brief 开机调试：向控制台打印当前系统参数快照；**进程内仅打印一次**（`ui_page_main_create` 在 `ui_app_state_boot_load` 之后调用）。
 */
#ifndef UI_BOOT_DEBUG_LOG_H
#define UI_BOOT_DEBUG_LOG_H

void ui_boot_debug_log_params(void);

#endif
