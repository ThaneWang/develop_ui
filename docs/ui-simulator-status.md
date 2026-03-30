# LVGL 模拟器：已实现 UI 与工程状态

本文档汇总 **PC 模拟器仓库** 中已落地的界面与跨模块行为，供对照 **`docs/project-plan.md`** 阶段 2 与 **`docs/dev-plan.md`** 任务。  
**产品章节与固件能力大纲**仍以 **`docs/firmware-fw-v1-framework.md`** 为准；**章节 ↔ 页面**速查见 **`docs/ui-fw-v1-mapping.md`**。  
**全部 `docs/` 阅读顺序与归类**：**[`docs/README.md`](README.md)**。

## 目录

- [1. 维护约定](#1-维护约定)
- [2. 按页面](#2-按页面)
- [3. 工程与跨页模块](#3-工程与跨页模块)
- [4. 布局与交互优化（已落实）](#4-布局与交互优化已落实)

---

## 1. 维护约定

- 合入界面相关功能时，在本文件 **对应小节** 追加一条（建议 `YYYY-MM-DD — 一句话`），并 **写明 `src/ui/…` 等路径**；与 **`docs/dev-plan.md`**「已完成」、**`docs/project-plan.md`** §2 可互链。详见 **`.cursor/rules/rules.md`** §**1.3**。

---

## 2. 按页面

### 主页面

- 预览区手势：右滑回放、左滑 ISP 侧栏、下拉控制中心、上滑模式页预览/提交；**右滑无右缘跟手条**（与上/下滑一致，仅松手阈值）；阈值与指针滚动钝感见 **`ui_common.h`**（`UI_SWIPE_*`、`UI_INDEV_SCROLL_*`）；`scr` 级 `main_viewport_gesture_cb`（`ui_page_main.c`）。
- 顶栏：左 — 文件符号 + `UI_STR_STORAGE_FREE_FMT` + `ui_app_get_storage_free_gb()`；右 — 蓝牙字条、当前模式符号、电量；`main_refresh_status_bar`；条带可纵向滚动（`ui_region_strip_enable_scroll`）。
- 底栏：三格；**左格仅展示**当前拍摄模式（符号 + i18n），**不可点**；**控制中心「快切」**（宫格索引 6）进入模式页。模式：`ui_app_state_boot_load` / `ui_app_set_shoot_mode`、**观察者**、`ui_shoot_mode.dat`。
- 预览装饰层：眼睛 + 四向手势说明（`main_scr_build_preview_decor`、`main_vp_add_hint_column`）。
- 全局占位：`ui_app_state`（模式、存储 GB、电量）；语种 `ui_i18n`。

### 回放

- 暂停态底栏与主界面一致展示当前 `ui_app` 模式（`ui_page_replay_sync_shoot_mode_display`）；顶栏、上传/播放、播放态占位与进度条、`ui_replay_player_load_request` 占位；左滑回主页（`ui_page_replay.c`、`ui_nav`、`ui_i18n`）。**视觉**：与 **`ui_theme.h`** / **`UI_CC_SETTINGS_ROW_*`**、**`ui_style_cc_interactive_focus`** 对齐（灰行、细边框、蓝描边缩放），全屏底 **`UI_THEME_SCREEN_BG`**。

### 控制中心

- 遮罩 + sheet、8 宫格、系统设置子视图入口；语种下拉与 `ui_i18n_set_lang` / `ui_i18n_refresh_all`（`ui_page_main.c`）。
- **旋转方向**详情子页：开关 + 四角度，样式同设置行，经 `ui_hw_display_rotation_set` 占位日志。

### 系统设置

- 列表文案对齐固件 §6 / §5；行左 `LV_SYMBOL_*`、语言行 `LV_SYMBOL_KEYBOARD`；Flex 行；列表剩余高度与滚动；滚动结束预览日志；与控制中心上滑关闭手势区分（`cc_settings_list_scroll_end_cb`、`cc_sync_settings_list_geom`）。行显隐：**`src/ui/theme/ui_settings_config.h`** 中 **`UI_SETTINGS_SHOW_*`**（见 **`.cursor/rules/api.md`** §**6.3**）。

### 模式切换

- 全屏模式页：标题栏下滑关闭；横向滚动 + 居中吸附；卡片图标（Montserrat）+ i18n、选中描边/底色；`SCROLL_END` 写回 `ui_app_set_shoot_mode`；侧击先滚中央再选中；中央图标确认关页；顶栏联动。
- 入口：**快切**、预览区上滑。
- 历史迭代：2026-03-30 — 模式条仅父级横滑、page 宽度与 `UI_MODE_STRIP_PAGE_W`（`ui_common.h` / `ui_page_main.c`）；打开时 `scroll_to_view` 对齐当前模式；`UI_STR_MODE_BODY` 避免全角标点（`ui_i18n.c`）。

### 蓝牙设置（系统设置子页）

- `ui_page_bt_settings.c`：`ui_bt_set_enabled` → `ui_hw_bluetooth_set_enabled`；配对/重连动画、低功耗等占位。
- 可记忆设置：**`ui_sim_settings.bin`** **v2**（蓝牙、低功耗、系统设置**语言**下拉、控制中心**亮度/音量**磁贴步进值、**旋转方向**开关与角度）；**`ui_sim_settings_boot_load`** 在 **`my_ui_init()`**、开机动画前；变更经 **`ui_sim_settings_persist_save`**。列表项「时间日期 / 电池…」等仍为点击占位，无独立控件可存。产品侧可替换 **`ui_sim_settings_persist.c`** / **`ui_cc_settings_state.c`**。

### 3A 页面

- 尚无独立页；ISP 侧栏为占位文案，与固件 §2.6 深度对齐待 **`dev-plan.md`** backlog。

---

## 3. 工程与跨页模块

| 模块 | 说明 |
|------|------|
| `ui_i18n` | 词条、`ui_i18n_bind_label` / `ui_i18n_refresh_all`、中英切换 |
| `ui_nav` | `lv_obj_clean` + 各 `ui_page_*_create` 异步切换；切页前 `ui_app_shoot_mode_observer_unregister_all` |
| `ui_theme.h` / `ui_hw_hal.c` | 颜色单源；外设占位 HAL（旋转/蓝牙/亮度/音量/拍摄模式等打印） |
| `ui_display` | `ui_display_set_screen_rotation` → HAL |
| `ui_boot_debug_log` | 进程内首次进入主页后 **`[BOOT]`** 行打印参数快照（屏、FW、持久化路径、模式/存储/电量、语种、蓝牙、亮度音量、旋转、滑动与指针阈值） |
| `CMakeLists.txt` | `ui_page_*`、`ui_i18n`、`ui_cc_settings_state`、`ui_boot_debug_log`、`ui_sim_settings_persist`、`ui_bt_state`、`ui_app_state`、`ui_hw_hal` 等已链入 `main` |

---

## 4. 布局与交互优化（已落实）

- **多语言**：主界面/控制中心/模式名/存储格式等绑定 `ui_i18n`；语种切换后 `main_refresh_status_bar` 与 `lv_obj_update_layout`（`cc_lang_dd_changed_cb`）。
- **手势**：预览、控制中心、模式页标题栏、设置列表 — 见 **`.cursor/rules/ui_swipe_gestures.md`**；顶栏蓝牙与控制中心手势分区见 `ui_page_main` 注释。
- **布局**：顶栏 Flex 双簇；模式条 page 宽与列间距；设置列表 Flex 行与图标列宽。
- **状态**：`ui_page_main_refresh_chrome()`（仅主屏生命周期内安全）；拍摄模式 **观察者** 同步顶栏/底栏/快切磁贴/回放底栏。
- **主题与 HAL**：见 **`.cursor/rules/rules.md`** §**1.0**。
- **屏幕旋转**：控制中心详情页 + `ui_hw_display_rotation_set`；导航进回放/蓝牙前 `ui_hw_display_rotation_set(false, 0°)`。

---

**相关**：[`docs/README.md`](README.md) · [`project-plan.md`](project-plan.md) · [`ui-fw-v1-mapping.md`](ui-fw-v1-mapping.md) · [`ui-camera-preview-and-overlay.md`](ui-camera-preview-and-overlay.md)
