# 开发任务 Plan

**项目阶段路线图**（导入资料 → LVGL 框架 → RTOS/平台）：见 **`docs/project-plan.md`**。

本文件用于 **你（维护者）** 写下希望 Cursor / AI 协助完成的 **代码开发任务**。实现时按 **`.cursor/rules/rules.md`**「迭代与变更」：**小步、可验证**，一次会话优先完成 **一条** 或紧密相关的一小组条目。

## 使用方式

1. 在下方 **「任务 backlog」** 中用列表追加想做的事（越具体越好：**模块 + 期望行为 + 验收方式**）。**凡 UI 相关条目须写明将改动或已改动的文件路径**（如 `src/ui/ui_page_main.c`），与 **`docs/project-plan.md`** §2.1.a / §2.2.a 一致；详见 **`.cursor/rules/rules.md`** §1.3。
2. 可选：用 `(refs: docs/…, §x)` 指向固件或 UI 对照文档。
3. 开工前可说「按 `docs/dev-plan.md` 做下一项」，或 **@ 本文件** 让助手优先读这里。
4. 完成后把对应条目标为 **已完成**（或移到「已完成」区），避免重复实现。

## 状态约定（可选）

| 标记 | 含义 |
|------|------|
| `[ ]` | 未开始 |
| `[~]` | 进行中（可选，手工维护） |
| `[x]` | 已完成 |

---

## 任务 backlog

<!-- 在下方追加你的开发需求，示例： -->

- [ ] （示例）为某屏增加占位文案并与 `docs/firmware-fw-v1-framework.md` §4.x 对齐
- [ ] （示例）系统设置某一项从打印改为真实状态机占位

---

## 已完成（可选归档）

<!-- 将已交付的条目剪贴到此处；与 **docs/project-plan.md** §2.1.a / §2.2.a 对照，避免只写在一处。 -->

### 对照 `project-plan.md` §2.1（基础页面）

- [x] **主页面**：预览手势、顶栏双分栏（存储 / 蓝牙+模式+电量）、底栏、装饰层、`ui_app_state`（`ui_page_main.c`、`ui_app_state.c`）。
- [x] **回放**：占位页与导航（`ui_page_replay.c`、`ui_nav.c`）。
- [x] **控制中心 + 系统设置列表**：宫格、设置子页、列表图标与滚动行为（`ui_page_main.c`）。
- [x] **模式切换**：横向模式条、选中态、顶栏图标、底栏/上滑入口（`ui_page_main.c`、`ui_i18n` 模式词条）。
- [x] **蓝牙设置子页**：`ui_page_bt_settings.c`、`ui_bt_state.c`。

### 对照 `project-plan.md` §2.2（优化布局、交互）

- [x] **i18n 与顶栏刷新**：`ui_i18n.c/.h`，语种切换与 `main_refresh_status_bar`。
- [x] **手势与冒泡**：与 `ui_swipe_gestures.md` / `rules.md` 一致的预览与控制中心逻辑。
- [x] **顶栏与模式条 Flex/滚动**：横向吸附、`ui_page_main_refresh_chrome` 声明。

### 待 backlog 转化（尚未在 §2.1.a 标「已完成」的）

- [ ] **3A 独立页面**（与固件 §2.6、ISP 能力对照）。
- [ ] 其他：在上方「任务 backlog」追加后，实现完成再移入本区并在 **project-plan §2.1.a / §2.2.a** 追加对应 bullet。
