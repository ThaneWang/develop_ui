/**
 * @file ui_settings_config.h
 * @brief 控制中心「系统设置」列表项是否参与编译与显示：`1` 显示，`0` 隐藏。可在包含本头文件前 `#define` 覆盖，或通过 CMake `target_compile_definitions` 注入。
 */
#ifndef UI_SETTINGS_CONFIG_H
#define UI_SETTINGS_CONFIG_H

/** 日期与时间（`UI_STR_SETTINGS_DATETIME`）。 */
#ifndef UI_SETTINGS_SHOW_DATETIME
#define UI_SETTINGS_SHOW_DATETIME 1
#endif
/** 电源与睡眠（`UI_STR_SETTINGS_POWER_SLEEP`）。 */
#ifndef UI_SETTINGS_SHOW_POWER_SLEEP
#define UI_SETTINGS_SHOW_POWER_SLEEP 1
#endif
/** 电池与充电（`UI_STR_SETTINGS_BATTERY`）。 */
#ifndef UI_SETTINGS_SHOW_BATTERY
#define UI_SETTINGS_SHOW_BATTERY 1
#endif
/** 温控策略（`UI_STR_SETTINGS_THERMAL`）。 */
#ifndef UI_SETTINGS_SHOW_THERMAL
#define UI_SETTINGS_SHOW_THERMAL 1
#endif
/** 存储卡 / Memory card（`UI_STR_SETTINGS_SDCARD`）。 */
#ifndef UI_SETTINGS_SHOW_SDCARD
#define UI_SETTINGS_SHOW_SDCARD 1
#endif
/** 固件升级（`UI_STR_SETTINGS_FIRMWARE`）。 */
#ifndef UI_SETTINGS_SHOW_FIRMWARE
#define UI_SETTINGS_SHOW_FIRMWARE 1
#endif
/** 日志与反馈（`UI_STR_SETTINGS_LOG`）。 */
#ifndef UI_SETTINGS_SHOW_LOG
#define UI_SETTINGS_SHOW_LOG 1
#endif
/** 安全与权限（`UI_STR_SETTINGS_SECURITY`）。 */
#ifndef UI_SETTINGS_SHOW_SECURITY
#define UI_SETTINGS_SHOW_SECURITY 1
#endif
/** 蓝牙设置入口（`UI_STR_SETTINGS_BT`）。 */
#ifndef UI_SETTINGS_SHOW_BT
#define UI_SETTINGS_SHOW_BT 1
#endif
/** Wi-Fi（`UI_STR_SETTINGS_WIFI`）。 */
#ifndef UI_SETTINGS_SHOW_WIFI
#define UI_SETTINGS_SHOW_WIFI 1
#endif
/** USB 与传输（`UI_STR_SETTINGS_USB`）。 */
#ifndef UI_SETTINGS_SHOW_USB
#define UI_SETTINGS_SHOW_USB 1
#endif
/** 文件导出（`UI_STR_SETTINGS_EXPORT`）。 */
#ifndef UI_SETTINGS_SHOW_EXPORT
#define UI_SETTINGS_SHOW_EXPORT 1
#endif
/** 设备信息（`UI_STR_SETTINGS_DEVICE`）。 */
#ifndef UI_SETTINGS_SHOW_DEVICE
#define UI_SETTINGS_SHOW_DEVICE 1
#endif
/** 恢复出厂（`UI_STR_SETTINGS_FACTORY`）。 */
#ifndef UI_SETTINGS_SHOW_FACTORY
#define UI_SETTINGS_SHOW_FACTORY 1
#endif

#endif
