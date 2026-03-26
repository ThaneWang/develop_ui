/**
 * @file ui.c
 * @brief UI 模块入口：实现 `my_ui_init()`，内部转调首个页面（当前为 `ui_page_main_create`）。
 */
#include "ui.h"
#include "../logging.h"
#include "ui_page_main.h"

#if UI_DEVELOP

void my_ui_init(void)
{
    LOG_DEBUG("my_ui_init（UI_DEVELOP）");
    ui_page_main_create(lv_scr_act());
    LOG_DEBUG("交互布局 UI 已初始化");
}

#else

void my_ui_init(void) {}

#endif
