# 开发任务 Plan

## 目录

- [文档关系](#文档关系)
- [使用方式](#使用方式)
- [状态约定](#状态约定)
- [任务 backlog](#任务-backlog)
- [已完成（归档）](#已完成归档)

---

## 文档关系

| 文档 | 角色 |
|------|------|
| **`docs/project-plan.md`** | 阶段 **目标与顺序** |
| **本文 `dev-plan.md`** | **可执行** 任务与勾选归档 |
| **`docs/ui-simulator-status.md`** | 合入后的 **实现细节** 追加处（须写清 `src/ui/…` 路径） |
| **`docs/README.md`** | 全部文档 **索引** |

实现时遵守 **`.cursor/rules/rules.md`**「迭代与变更」：**小步、可验证**。

---

## 使用方式

1. 在 **[任务 backlog](#任务-backlog)** 追加条目（**模块 + 期望行为 + 验收**）。**UI 相关须写文件路径**（如 `src/ui/ui_page_main.c`），与 **`docs/ui-simulator-status.md`**、**`project-plan.md`** §2 一致；规则见 **`.cursor/rules/rules.md`** §**1.3**。
2. 可注明 `(refs: docs/firmware-fw-v1-framework.md §x)` 等。
3. 开工前可说「按 `dev-plan.md` 做下一项」。
4. 完成后将条目标为 **[x]** 或移至 **[已完成](#已完成归档)**，并在 **`ui-simulator-status.md`** 对应小节 **追加一行**（若涉及界面行为）。

---

## 状态约定

| 标记 | 含义 |
|------|------|
| `[ ]` | 未开始 |
| `[~]` | 进行中（可选） |
| `[x]` | 已完成 |

---

## 任务 backlog

- [ ] （示例）为某屏增加占位文案并与 `docs/firmware-fw-v1-framework.md` §4.x 对齐
- [ ] （示例）系统设置某一项从打印改为状态机占位
- [ ] **3A 独立页面**（与固件 §2.6、ISP 能力对照；refs: `ui-fw-v1-mapping.md`）

---

## 已完成（归档）

### 对照 `project-plan.md` §2（基础页面）

- [x] **主页面**：预览手势、顶栏、底栏、装饰层、`ui_app_state`（`ui_page_main.c`、`ui_app_state.c`）。
- [x] **回放**：占位与导航（`ui_page_replay.c`、`ui_nav.c`）。
- [x] **控制中心 + 系统设置**：宫格、设置子页、旋转详情、列表与滚动（`ui_page_main.c`）。
- [x] **模式切换**：模式条、选中态、观察者、快切/上滑入口（`ui_page_main.c`、`ui_i18n`）。
- [x] **蓝牙设置子页**：`ui_page_bt_settings.c`、`ui_bt_state.c`、`ui_hw_hal.c`。

### 对照 §2（优化与工程）

- [x] **i18n 与顶栏刷新**：`ui_i18n`、`main_refresh_status_bar`。
- [x] **手势与冒泡**：与 `ui_swipe_gestures.md` / `rules.md` 一致。
- [x] **主题与 HAL**：`ui_theme.h`、`ui_hw_hal.c`、`rules.md` §1.0。

> **细目与路径** 已迁移至 **`docs/ui-simulator-status.md`**，此处仅保留勾选归档。

---

**索引**：[`docs/README.md`](README.md)
