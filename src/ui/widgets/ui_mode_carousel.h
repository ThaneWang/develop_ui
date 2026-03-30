/**
 * @file ui_mode_carousel.h
 * @brief 水平循环模式条（思路二「多段缓冲」：重复开放顺序的横向缓冲 + 滚动 + snap）。官方 LVGL 无 `lv_carousel`。
 */
#ifndef UI_MODE_CAROUSEL_H
#define UI_MODE_CAROUSEL_H

#include "ui_app_state.h"
#include "lvgl/lvgl.h"

/** 物理槽上限：循环时为 3×开放项（左/中/右各一份开放顺序），否则不超过 `UI_SHOOT_MODE_COUNT` */
#define UI_MODE_CAROUSEL_SLOT_MAX (UI_SHOOT_MODE_COUNT * 3)

/**
 * 创建横向 carousel 容器（内部为 `lv_obj` 滚动 + flex 行）。
 * @param parent 父对象（如模式全屏层）
 * @param screen_width 用于左右对称 padding，通常 `MY_SCREEN_WIDTH`
 */
lv_obj_t *ui_mode_carousel_create(lv_obj_t *parent, lv_coord_t screen_width);

/**
 * 滚动跟手时刷新高亮、`SCROLL_END` 提交后写回 `ui_app` 等（由 `ui_page_main` 注入）。
 */
void ui_mode_carousel_set_callbacks(lv_obj_t *carousel, void (*on_visual_scroll)(void *user_data),
                                    void (*on_mode_committed)(ui_shoot_mode_t m, void *user_data), void *user_data);

/** 配置卡片几何与列间距（须在 `build` 前调用） */
void ui_mode_carousel_configure_strip(lv_obj_t *carousel, lv_coord_t card_w, lv_coord_t page_w, lv_coord_t col_gap);

/**
 * 按开放列表构建子页。
 * @param loop `true` 且 n≥2 时将开放顺序重复铺满左/中/右缓冲；`n==1` 时仅一页
 * @param card_transition 卡片选中过渡，可为 NULL
 * @param icon_clicked_cb 图标 `CLICKED`，`user_data` 为 `(void*)(uintptr_t)phys_idx`
 */
void ui_mode_carousel_build(lv_obj_t *carousel, const ui_shoot_mode_t *modes, uint32_t n, bool loop,
                            const lv_style_transition_dsc_t *card_transition, lv_event_cb_t icon_clicked_cb);

uint32_t ui_mode_carousel_phys_count(const lv_obj_t *carousel);
uint32_t ui_mode_carousel_real_count(const lv_obj_t *carousel);

uint32_t ui_mode_carousel_nearest_index(lv_obj_t *carousel);
void ui_mode_carousel_highlight_slot(lv_obj_t *carousel, uint32_t slot);

/** 将 `want` 对齐到视口中部；循环开启时在真项段 `1…n` 选槽。动画开启时立即高亮目标槽。 */
void ui_mode_carousel_scroll_to_mode(lv_obj_t *carousel, ui_shoot_mode_t want, lv_anim_enable_t anim);

void ui_mode_carousel_program_scroll_begin(lv_obj_t *carousel);
void ui_mode_carousel_program_scroll_end(lv_obj_t *carousel);

lv_obj_t *ui_mode_carousel_page(lv_obj_t *carousel, uint32_t slot);
ui_shoot_mode_t ui_mode_carousel_mode_at(const lv_obj_t *carousel, uint32_t slot);

#endif
