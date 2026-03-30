# 循环横向模式列表：两种实现思路

本文档记录 **拍摄模式横条**（或同类「横向循环图标」）的两种实现路径，便于选型、评审与后续重构。**产品逻辑**（`ui_app` 单一数据源、居中吸附、惯性、`SCROLL_END` 写回）见 **`.cursor/rules/rules.md`** §**1.4** 与 **`src/ui/ui_page_main.c`** 中 `s_mode_strip` 相关实现。

## 与本仓库当前状态的关系

- **本仓库** 所携带的 **`lvgl/`** 树中 **未检索到** `lv_carousel` 组件（多为 NXP GUI Guider 等扩展）。
- **当前实现** 为 **思路二**「**多段缓冲**」的 **工程内等价物**（**非** 动态重排，以便与 **`SCROLL_SNAP_CENTER`**、对称 **pad** 稳定配合）：**`src/ui/ui_mode_carousel.c`** + **`ui_page_main.c`** 创建 **`s_mode_strip`**；**开放项 ≥2** 时把 **同一开放顺序** 连续铺 **三份**（左/中/右缓冲带），**`SCROLL_END`** 落在左/右带时 **无动画** 对齐到 **中间带** 等效槽，**`SCROLL_SNAP_CENTER`** 与 **甩动惯性**（`SCROLL_MOMENTUM` + 指针 `scroll_throw` 调参）保留；与 **`ui_app`** 在 **`SCROLL_END`/点击** 时同步。
- 若将来 LVGL 或第三方提供 **`lv_carousel`**，可评估 **替换** **`ui_mode_carousel`** 内部实现，**保留** 对 **`ui_app`** 的回调约定。

---

## 思路一：`lv_carousel`（推荐）

**适用场景**：图标数量适中（如十几个以内），希望快速实现循环、手势切换，且每个图标为独立页面。

**实现要点**：

1. 创建 `lv_carousel`，方向 **`LV_CAROUSEL_DIR_HOR`**（若 API 与版本一致），并启用 **`lv_carousel_set_loop`** 循环模式。
2. 每个模式用 **`lv_carousel_add`** 增加一页，页内放置 **`lv_img`** 或 **`lv_obj`** 组合（图标 + 文案）。
3. 开启触摸后，横向滑动由控件处理，单页级切换；需 **一次跳多页** 时可多次调用 **`lv_carousel_next` / `lv_carousel_prev`**，或按滑动距离/速度自定义跳转。

**优点**：代码量少；原生循环、动画与手势；常带 **指示点（dots）**，便于感知位置。

**缺点**：每页独立对象，数量多时内存占用较大。

**与模式切换逻辑对齐**：以 **`lv_carousel` 当前页** 对应** `ui_shoot_mode_t`**，在 **`VALUE_CHANGED`** 或等价事件中调用 **`ui_app_set_shoot_mode`**；打开模式页时用 **`lv_carousel_set_active`**（或等价 API）对齐到当前全局模式。

---

## 思路二：滚动容器 + 循环策略

**适用场景**：图标较多（如几十个）、需省内存，或希望 **一次滑动** 经过 **多格**（惯性、长划）。

**实现要点**：

1. **`lv_obj`** 作为滚动容器，**`lv_obj_set_scroll_dir(..., LV_DIR_HOR)`**，横向滚动条按需关闭。
2. 子项为等宽图标（或 `page + 卡`），**Flex** 或手动布局成一排。
3. **循环策略**（二选一或组合）：
   - **动态重排**：监听 **`LV_EVENT_SCROLL`** / **`SCROLL_END`**，当滚动越过边界时，把 **最后一个子对象移到最前**（或 **第一个移到最后**），并 **修正 `scroll_x`**，视觉无限循环。
   - **多段缓冲**（可选）：**不**移动子对象，而是 **预先铺多段相同开放顺序**（本仓库为 **三份并列**），**`SCROLL_END`** 时 **无动画** 跳到中间带等效槽；**本仓库当前采用** 与 **`ui_mode_carousel.c`** 一致。

**优点**：子项可共享样式；惯性、多格甩动与 **LVGL** 原生滚动一致；图标数量可扩展。

**缺点**：需自维护循环与边界（或缓冲段），代码量与测试成本高于 **思路一**。

---

## 相关源码与规则

| 内容 | 位置 |
|------|------|
| 当前横条实现（思路二·多段缓冲等价） | `src/ui/ui_mode_carousel.c`、`src/ui/ui_page_main.c`（`main_create_mode_panel`、`main_mode_strip_scroll_to_current`） |
| 手势与滚动约定 | `.cursor/rules/ui_swipe_gestures.md` §**2** |
| 模式横条与 `ui_app` 约定 | `.cursor/rules/rules.md` §**1.4** |

## 维护

新增或替换 **LVGL** 版本后，若 **`lv_carousel`** 可用，应在本文件 **文首** 更新「与本仓库当前状态」一小节，并视情况在 **`docs/ui-simulator-status.md`** 记一条实现变更。
