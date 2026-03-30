# 项目文档索引

本目录为 **相机 LVGL 模拟器工程** 的设计与对齐说明（不含 `lvgl/`、`SDL2/` 等第三方子树内文档）。

## 阅读顺序建议

| 顺序 | 文档 | 用途 |
|------|------|------|
| 1 | 本文 **`README.md`** | 总索引与模块归类 |
| 2 | **`project-plan.md`** | 三阶段路线图（资料 → LVGL UI → RTOS/平台） |
| 3 | **`dev-plan.md`** | 可执行的开发 backlog 与「已完成」归档 |
| 4 | **`firmware-fw-v1-framework.md`** | 产品固件能力大纲（FW-V1） |
| 5 | **`ui-fw-v1-mapping.md`** | 固件章节 ↔ 模拟器 UI 对照表 |
| 6 | **`ui-simulator-status.md`** | 模拟器 **已实现** 的页面与工程细节（持续追加） |
| 7 | **`ui-camera-preview-and-overlay.md`** | 目标机 **预览 Overlay + 录制** 与 LVGL 分层架构 |
| 8 | **`ui-mode-strip-carousel-options.md`** | 循环横向模式列表：**思路一 / 思路二** 对照；**当前** 为 **`ui_mode_carousel.c`**（多段缓冲 + 惯性） |

## 按主题归类

### 路线图与任务

- **`project-plan.md`** — 阶段目标、与 `dev-plan` 分工、阶段 3（RTOS/外设/字体等）条目。
- **`dev-plan.md`** — 维护者 backlog、`[ ]/[x]` 约定、与 §2.1/§2.2 对照的「已完成」列表。

### 产品与固件

- **`firmware-fw-v1-framework.md`** — FW-V1 章节结构（相机、回放、UI、连接、系统管理、配件、AI、声音、默认值、附录等）；文首 **文档导航** 表指向本索引与其它协作文档。
- **`ui-fw-v1-mapping.md`** — 上表各章在 **`src/ui`** 中的对应关系与维护约定（**不**重复粘贴固件正文）。

### 模拟器实现与架构

- **`ui-simulator-status.md`** — 主界面/回放/控制中心/设置/模式/蓝牙/3A 占位、主题与 HAL、手势与布局优化等 **落地状态**（原分散在 `project-plan` §2.1.a / §2.1.b / §2.2.a 的内容已汇总于此）。
- **`ui-camera-preview-and-overlay.md`** — 硬件叠加层、采集/编码与 UI 线程职责、Mermaid 数据流、与本仓库 `ui_hw_hal` 的衔接。
- **`ui-mode-strip-carousel-options.md`** — 循环横向模式条两种实现；**当前** 为 **`ui_mode_carousel.c`**（多段缓冲 + 甩动惯性，非动态重排）。

### Cursor / 协作规则（仓库根下）

- **`.cursor/rules/rules.md`** — 工程结构、字库、编码规范、**§1.0 主题与 HAL**、§1.1～§1.4 UI 约定等。
- **`.cursor/rules/ui_swipe_gestures.md`** — 主屏/控制中心/回放等 **滑动阈值与轴向**。
- **`.cursor/rules/api.md`** — LVGL API 索引、**§6.3 `UI_SETTINGS_SHOW_*`** 等。
- **`.cursor/summaries/YYYY-MM-DD.md`** — 按日会话纪要（实现相关），**不**替代上述正式文档。

## 仓库根目录 `README.md`

- 主要为 **上游 LVGL PC 模拟器** 的英文说明：SDL、CMake、VSCode 调试、可选 FreeRTOS、Demo 切换等。
- **中文相机 UI 与本文档体系**：以 **`docs/README.md`**（本文件）为入口。

---

**维护**：新增独立主题文档时，在本文件 **按主题归类** 表中增加一行，并避免与 `firmware-fw-v1-framework.md` 正文重复 — 产品条文只维护在固件框架文档中。
