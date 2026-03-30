# 项目阶段 Plan（导入路线图）

> **与 `docs/dev-plan.md` 的关系**：本文件描述 **阶段目标与范围**；可执行的 **小步任务** 写在 **`dev-plan.md`** 的 backlog，协作时遵守 **`.cursor/rules/rules.md`**「迭代与变更」。

---

## 1. 导入项目资料

- 收集并归档：需求说明、硬件/SoC 资料、接口定义、与 **`docs/firmware-fw-v1-framework.md`** 一致或可追溯的产品说明。
- 约定版本、目录与引用方式，便于后续 LVGL / RTOS 工程对齐。

---

## 2. 项目导入 LVGL，搭建 UI 框架

### 2.1 基础页面

| 页面 | 说明 |
|------|------|
| 主页面 | 预览、顶栏/底栏、手势入口 |
| 回放 | 机内回放占位与返回 |
| 控制中心 | 下拉全屏、宫格磁贴 |
| 系统设置 | 列表及 **详情子页**（如蓝牙等） |
| 模式切换 | 上滑模式页等 |
| 3A 页面 | 曝光/对焦/白平衡等 **3A** 相关 UI（与固件 §2.6、ISP 能力对照） |

当前 PC 模拟器仓库：部分已在 **`src/ui/ui_page_*.c`** 中占位，其余按 **`docs/ui-fw-v1-mapping.md`** 与固件文档 **增量** 补齐。

#### 2.1.a 各页面已有 UI 改动（按上表行归档，持续追加）

> **维护**：每次合入与界面相关的功能，在 **对应行** 下 **追加一条** bullet（建议格式：`YYYY-MM-DD — 一句话`）；**必须列出改动涉及的源文件路径**（如 `src/ui/ui_page_main.c`），多文件逐项或顿号分隔。与 **`docs/dev-plan.md`**「已完成」可互链；此处保持 **按 Plan 步骤归类**。详见 **`.cursor/rules/rules.md`** §1.3。

- **主页面**
  - 预览区手势：右滑回放、左滑 ISP 侧栏、下拉控制中心、上滑模式页预览/提交；预览区与 `scr` 级 `main_viewport_gesture_cb`（`ui_page_main.c`）。
  - 顶栏：左右 Flex 分栏 — 左为文件符号 + 剩余空间文案（`UI_STR_STORAGE_FREE_FMT` + `ui_app_get_storage_free_gb()`）；右为蓝牙开启字条、当前拍摄模式符号、电量符号与百分比（`ui_app_state` + `main_refresh_status_bar`）；顶栏条带可纵向滚动（`ui_region_strip_enable_scroll`）。
  - 底栏：三格（模式显示区 / 模式参数区 / 视图控制）；**模式显示区点击** 进入模式页。
  - 预览装饰层：眼睛符号 + 四向手势说明（`main_scr_build_preview_decor`、`main_vp_add_hint_column`）。
  - 全局占位：`ui_app_state`（拍摄模式、存储 GB、电量百分比），语种仍用 `ui_i18n`。
- **回放**
  - 全屏回放占位页、顶栏与返回手势/导航（`ui_page_replay.c`、`ui_nav`）。
- **控制中心**
  - 下拉遮罩 + sheet、8 宫格磁贴、进入系统设置子视图；语种下拉与 `ui_i18n_set_lang` / `ui_i18n_refresh_all`（`ui_page_main.c` 内 `main_create_control_center` 等）。
- **系统设置**
  - 列表项文案对齐固件 §6 / §5 分组；行左侧 `LV_SYMBOL_*` 图标、语言行 `LV_SYMBOL_KEYBOARD`；Flex 行布局；列表固定剩余高度与滚动；滚动结束预览日志；列表纵向滚动与控制中心上滑关闭手势区分（`cc_settings_list_scroll_end_cb`、`cc_sync_settings_list_geom`）。
- **模式切换**
  - 全屏模式页：仅 **标题栏** 下滑关闭；**横向滚动 + 居中吸附**；卡片 **图标（Montserrat）+ i18n 名称**、选中描边/底色；**滑动结束** 将 **居中项** 写回 `ui_app_set_shoot_mode`；**点击屏幕侧边模式图标** 仅先滚到中央，**`SCROLL_END` 后再** 刷新选中态；**点击中央模式图标** 确认并关页回主页；顶栏模式符号联动。
  - 2026-03-30 — 模式条仅 **父级横滑**；卡片与 Label 去掉 **内部 SCROLLABLE/弹性滚动**；子项为 **page**（宽 **`UI_MODE_STRIP_PAGE_W`** ≈ 卡宽 + 页内留白，按屏宽分数，`src/ui/ui_common.h`），左右窥见 **邻卡图标** — `src/ui/ui_page_main.c`。
  - 2026-03-30 — 打开模式页时 **`scroll_to_view` + `update_snap`** 对齐 **全局当前模式**（`ui_app` 默认首项）；非法枚举钳位 **第一项** — `src/ui/ui_page_main.c`。
  - 2026-03-30 — **`UI_STR_MODE_BODY`** 避免 **全角标点**（静态字库 `noto_sans_sc_16` 为 U+4E00～U+9FFF），防说明文案方框 — `src/ui/ui_i18n.c`。
  - 入口：底栏左格点击、预览区上滑（沿用原有提交逻辑）。
- **蓝牙设置（系统设置子页）**
  - `ui_page_bt_settings.c`：开关写 `ui_bt_state`；配对/重连动画占位等。
- **3A 页面**
  - （尚无独立页）ISP 侧栏仍为 **占位文案区**，与固件 §2.6 深度对齐待 **`dev-plan.md` backlog** 立项。

#### 2.1.b 模块与工程侧（跨页面）

- **`ui_i18n`**：词条表、`ui_i18n_bind_label` / `ui_i18n_refresh_all`、中英切换。
- **`ui_nav`**：`lv_obj_clean` + 各 `ui_page_*_create` 异步切换。
- **`CMakeLists.txt`**：`ui_page_*`、`ui_i18n`、`ui_bt_state`、`ui_app_state` 等已加入 `main` 目标。

### 2.2 优化布局、交互

- 在 2.1 结构稳定后，迭代 **布局、手势、多语言、滚动与冒泡** 等（见 **`.cursor/rules/ui_swipe_gestures.md`**、**`rules.md`**）。

#### 2.2.a 已落实的优化类改动（持续追加）

> 对应上条 **2.2** 目标，下列为已在模拟器中落地的增量（新改动请 **追加 bullet**，且 **写明 `src/ui/…` 等路径**，见 **`.cursor/rules/rules.md`** §1.3）。

- **多语言**：主界面、控制中心、模式名、存储格式字符串等绑定 `ui_i18n`；语种切换后 `main_refresh_status_bar` 与全局 `lv_obj_update_layout`（`cc_lang_dd_changed_cb`）。
- **手势与事件**：预览区与控制中心、模式页标题栏、设置列表滚动的轴向与阈值约定见 **`.cursor/rules/ui_swipe_gestures.md`**；蓝牙字条置于顶栏右侧子容器、避免与下拉控制中心抢手势（历史说明见 `ui_page_main` 注释）。
- **布局**：顶栏 Flex 双簇、模式页 Flex + 横向滚动条（**page** 宽小于屏宽 + 列间距）、系统设置列表 Flex 行与图标列宽。
- **状态与刷新**：`ui_page_main_refresh_chrome()` 供主屏显式刷新顶栏（仅主屏生命周期内安全调用）；`ui_app_state` 与顶栏/模式页同步。
- **可选功能**：`UI_FEATURE_DISPLAY_ROTATION` 打开时控制中心内旋转面板（见 `ui_page_main.c` 条件编译）。

---

## 3. 项目导入 RTOS 与平台能力

- **RTOS**：任务划分、与 UI 线程/消息机制对接。
- **文件系统**：媒体路径、配置与资源加载（与固件 §3、§6 等对照）。
- **UI 图标**：位图/矢量资源与 LVGL 资源管理。
- **外设接口**：按键、传感器、连接等与 UI 状态同步（占位可先日志）。
- **固件升级**：流程与 UI 提示（与固件 §6.7 等对照）。
- **字体库**：除模拟器静态字体外，目标机上的字库方案与裁剪策略。

---

**维护**：阶段顺序为推荐依赖关系；同一阶段内仍宜 **小步交付**，避免单次铺开过大范围。
