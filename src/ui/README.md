# `src/ui` 目录说明

相机 UI 源码按 **职责子目录** 存放；**对外 API 与文件名不变**（仍使用 `#include "ui_xxx.h"`），依赖 CMake 为 `main` 目标配置的 **`target_include_directories`**（各子目录均加入搜索路径）。

## 子目录一览

| 目录 | 内容 |
|------|------|
| **`core/`** | 入口 `my_ui_init`（`ui.c` / `ui.h`）、开机动画（`ui_boot`）、导航（`ui_nav`）、蓝牙进页（`ui_bt_nav`）、跨页事件（`ui_events`） |
| **`platform/`** | 外设占位 HAL（`ui_hw_hal`）、显示旋转封装（`ui_display`）、指针 profile（`ui_indev`）、中文字体（`ui_font`） |
| **`state/`** | 全局应用状态（`ui_app_state`）、模拟器持久化（`ui_sim_settings_persist`）、控制中心记忆（`ui_cc_settings_state`）、蓝牙状态（`ui_bt_state`）、i18n（`ui_i18n`） |
| **`theme/`** | 颜色单源（`ui_theme.h`）、布局与手势常量（`ui_common.h`）、共用样式（`ui_style`）、系统设置行开关（`ui_settings_config.h`） |
| **`widgets/`** | 可复用控件（当前为模式横条 **`ui_mode_carousel`**） |
| **`pages/`** | 全屏页面：`ui_page_main` / `ui_page_replay` / `ui_page_bt_settings` |
| **`diag/`** | 调试与开机参数日志（`ui_boot_debug_log`） |

## 工程外可见头文件

- **`main.c`** 等工程根源码使用 **`#include "ui/core/ui.h"`**、**`#include "ui/platform/ui_font.h"`**（需 `-I src`，由 **`target_include_directories(main … ${PROJECT_SOURCE_DIR}/src)`** 提供）。

## 维护约定

- **新增页面**：在 **`pages/`** 增加 `ui_page_<name>.c/.h`，并在 **`CMakeLists.txt`** 的 **`MAIN_SOURCES`** 中登记 `.c`。
- **新增其它模块**：放入上表对应子目录，并同时把该子目录保持于 **`target_include_directories(main PRIVATE …)`**（若为新子目录则追加一行）。
- **`.c` 内引用 `src/logging.h`**：使用 **`#include "../../logging.h"`**（相对 `src/ui/<subdir>/`）。

详见 **`.cursor/rules/rules.md`** §**一**。
