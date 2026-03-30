/**
 * @file ui_sim_settings_persist.h
 * @brief 模拟器将可记忆设置写入 **`ui_sim_settings.bin`**（v2：蓝牙、低功耗、语种、亮度、音量、屏幕旋转等）。
 * 应在 **`my_ui_init()`** 内、开机动画前调用 **`ui_sim_settings_boot_load()`**。产品工程可整文件替换为 NVM。
 */
#ifndef UI_SIM_SETTINGS_PERSIST_H
#define UI_SIM_SETTINGS_PERSIST_H

/** 默认与可执行文件工作目录同级的二进制档；编译前可 `#define` 覆盖。 */
#ifndef UI_SIM_SETTINGS_PERSIST_FILE
#define UI_SIM_SETTINGS_PERSIST_FILE "ui_sim_settings.bin"
#endif

/** 进程启动时调用一次：恢复蓝牙/语种/控制中心记忆项（无文件或损坏则各模块默认）。 */
void ui_sim_settings_boot_load(void);

/** 将当前可持久化状态写入文件（`ui_bt_set_*`、`ui_i18n_set_lang`、`ui_cc_settings_set_*` 等会触发）。 */
void ui_sim_settings_persist_save(void);

#endif
