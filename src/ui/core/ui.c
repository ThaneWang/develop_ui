/**
 * @file ui.c
 * @brief UI 模块入口：实现 `my_ui_init()`，内部转调首个页面（当前为 `ui_page_main_create`）。
 */
#include "ui.h"
#include "../../logging.h"
#include "ui_boot.h"
#include "ui_sim_settings_persist.h"

#if UI_DEVELOP

/** UI 初始化入口：`UI_DEVELOP` 下经开机动画再进入主界面。 */
void my_ui_init(void)
{
    LOG_DEBUG("my_ui_init（UI_DEVELOP）");
    ui_sim_settings_boot_load();
    ui_boot_show_then_main();
    LOG_DEBUG("交互布局 UI 已初始化（含开机动画入口）");
}

#else

/** 非开发构建：空实现。 */
void my_ui_init(void) {}

#endif
