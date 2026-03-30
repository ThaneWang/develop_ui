/**
 * @file ui_app_state.h
 * @brief 全局应用选项占位（语种见 ui_i18n；此处为拍摄模式、存储与电量等，后续可接固件/设置）。
 *
 * 拍摄模式 **是否在 UI 中可选** 由 `UI_SHOOT_ENABLE_*` 控制（产品 **P0** 默认仅 3DGS、录像、拍照）。
 * 模拟器调试可设 **`UI_DEBUG_ALL_SHOOT_MODES`** 为 **1**（默认）开放全部模式；设为 **0** 恢复 P0 四项外为关。
 * 编译前可 `#define UI_SHOOT_ENABLE_VIDEO 0` 等逐项覆盖；或与 CMake `-D` 配合。
 */
#ifndef UI_APP_STATE_H
#define UI_APP_STATE_H

#include <stdbool.h>
#include <stdint.h>

/** 产品型号（开机动画等） */
#ifndef UI_FW_PRODUCT_MODEL
#define UI_FW_PRODUCT_MODEL "HP320"
#endif
/** 界面展示用固件版本号 */
#ifndef UI_FW_VERSION_STRING
#define UI_FW_VERSION_STRING "FW00.00.01"
#endif
/** 构建标识（日志 / 与版本串一致时可用于描述当前配置） */
#ifndef UI_FW_BUILD_ID
#define UI_FW_BUILD_ID "P0_FW00.00.01"
#endif

#ifndef UI_DEBUG_ALL_SHOOT_MODES
#define UI_DEBUG_ALL_SHOOT_MODES 1
#endif

#ifndef UI_SHOOT_ENABLE_3DGS
#define UI_SHOOT_ENABLE_3DGS 1
#endif
#ifndef UI_SHOOT_ENABLE_VIDEO
#define UI_SHOOT_ENABLE_VIDEO 1
#endif
#ifndef UI_SHOOT_ENABLE_PHOTO
#define UI_SHOOT_ENABLE_PHOTO 1
#endif
#ifndef UI_SHOOT_ENABLE_AI_DIRECTOR
#if UI_DEBUG_ALL_SHOOT_MODES
#define UI_SHOOT_ENABLE_AI_DIRECTOR 1
#else
#define UI_SHOOT_ENABLE_AI_DIRECTOR 0
#endif
#endif
#ifndef UI_SHOOT_ENABLE_3DGS_VIDEO
#if UI_DEBUG_ALL_SHOOT_MODES
#define UI_SHOOT_ENABLE_3DGS_VIDEO 1
#else
#define UI_SHOOT_ENABLE_3DGS_VIDEO 0
#endif
#endif
#ifndef UI_SHOOT_ENABLE_FREE_RATIO_VIDEO
#if UI_DEBUG_ALL_SHOOT_MODES
#define UI_SHOOT_ENABLE_FREE_RATIO_VIDEO 1
#else
#define UI_SHOOT_ENABLE_FREE_RATIO_VIDEO 0
#endif
#endif
#ifndef UI_SHOOT_ENABLE_DUAL_LENS_VIDEO
#if UI_DEBUG_ALL_SHOOT_MODES
#define UI_SHOOT_ENABLE_DUAL_LENS_VIDEO 1
#else
#define UI_SHOOT_ENABLE_DUAL_LENS_VIDEO 0
#endif
#endif

/** 拍摄模式持久化文件名（单字节 `ui_shoot_mode_t`）；可编译前 `#define` 覆盖。 */
#ifndef UI_APP_SHOOT_MODE_PERSIST_FILE
#define UI_APP_SHOOT_MODE_PERSIST_FILE "ui_shoot_mode.dat"
#endif

typedef enum {
    UI_SHOOT_MODE_3DGS = 0,
    UI_SHOOT_MODE_VIDEO,
    UI_SHOOT_MODE_PHOTO,
    UI_SHOOT_MODE_AI_DIRECTOR,
    UI_SHOOT_MODE_3DGS_VIDEO,
    UI_SHOOT_MODE_FREE_RATIO_VIDEO,
    UI_SHOOT_MODE_DUAL_LENS_VIDEO,
    UI_SHOOT_MODE_COUNT,
} ui_shoot_mode_t;

/** 某拍摄模式是否在模拟器/UI 中开放（由 `UI_SHOOT_ENABLE_*` 决定）。 */
bool ui_shoot_mode_option_enabled(ui_shoot_mode_t m);

/** 第一个已开放的拍摄模式（配置全关时退回 3DGS）。 */
ui_shoot_mode_t ui_shoot_mode_first_enabled(void);

/** 若当前保存的模式未开放或非法，则改为 `ui_shoot_mode_first_enabled()`。 */
void ui_app_shoot_mode_ensure_enabled(void);

/** 恢复默认拍摄模式、存储与电量占位。 */
void ui_app_state_init(void);

/**
 * 进入主界面前调用：尝试从 **`UI_APP_SHOOT_MODE_PERSIST_FILE`**（默认 `ui_shoot_mode.dat`，与可执行工作目录相关）读取上次保存的模式；
 * 无文件或非法/未开放则使用 **`ui_shoot_mode_first_enabled()`**。
 */
void ui_app_state_boot_load(void);

/** 设置当前拍摄模式；非法或未开放的模式将被忽略。成功写入后会 **持久化** 到上述文件（模拟器关机前记忆）。 */
void ui_app_set_shoot_mode(ui_shoot_mode_t m);
/** 查询当前拍摄模式。 */
ui_shoot_mode_t ui_app_get_shoot_mode(void);

/** 拍摄模式变更观察者：`ui_app_set_shoot_mode` 成功写盘、以及 `ensure_enabled` 钳位并写盘时触发。 */
typedef void (*ui_app_shoot_mode_observer_fn)(void *user_data);

/** 登记观察者（去重：同一 fn+user_data 不重复）；满则返回 false。 */
bool ui_app_shoot_mode_observer_register(ui_app_shoot_mode_observer_fn fn, void *user_data);
/** 按 fn+user_data 移除；无匹配则忽略。 */
void ui_app_shoot_mode_observer_unregister(ui_app_shoot_mode_observer_fn fn, void *user_data);
/** 全屏切页前调用：清空所有观察者，避免悬空指针。 */
void ui_app_shoot_mode_observer_unregister_all(void);

/** 状态栏 / 模式卡片用大号符号字体显示 */
const char *ui_app_shoot_mode_icon_glyph(ui_shoot_mode_t m);

/** 占位：剩余存储 GiB。 */
uint32_t ui_app_get_storage_free_gb(void);
/** 占位：设置剩余存储 GiB。 */
void ui_app_set_storage_free_gb(uint32_t gb);

/** 占位：电量 0～100。 */
uint8_t ui_app_get_battery_percent(void);
/** 占位：设置电量百分比。 */
void ui_app_set_battery_percent(uint8_t pct);

#endif
