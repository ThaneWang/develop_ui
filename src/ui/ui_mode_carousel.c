/**
 * @file ui_mode_carousel.c
 * @brief 水平循环模式条：`docs/ui-mode-strip-carousel-options.md` **思路二**「多段缓冲」（重复开放顺序 + `SCROLL_END` 归位中段）。
 */
#include "ui_mode_carousel.h"
#include "ui_common.h"
#include "ui_i18n.h"
#include "ui_style.h"
#include "ui_font.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/indev/lv_indev_scroll.h"
#include <string.h>

typedef struct {
    uint32_t phys_n;
    uint32_t real_n;
    bool loop;
    lv_coord_t screen_w;
    lv_coord_t card_w;
    lv_coord_t page_w;
    lv_coord_t col_gap;
    lv_obj_t *pages[UI_MODE_CAROUSEL_SLOT_MAX];
    lv_obj_t *cards[UI_MODE_CAROUSEL_SLOT_MAX];
    ui_shoot_mode_t mode_at[UI_MODE_CAROUSEL_SLOT_MAX];
    uint8_t scroll_end_suppress;
    void (*on_visual_scroll)(void *user_data);
    void (*on_mode_committed)(ui_shoot_mode_t m, void *user_data);
    void *callback_user_data;
    /** 构建时传入的卡片过渡；指针拖动横条时临时置 `NULL` 使选中边框跟手、无 200ms 滞后 */
    const lv_style_transition_dsc_t *card_tr;
    bool strip_instant_border;
} ui_mode_carousel_dsc_t;

static ui_mode_carousel_dsc_t *carousel_get_dsc(lv_obj_t *carousel)
{
    if(carousel == NULL || !lv_obj_is_valid(carousel)) {
        return NULL;
    }
    return (ui_mode_carousel_dsc_t *)lv_obj_get_user_data(carousel);
}

static void carousel_cards_set_transition(ui_mode_carousel_dsc_t *dsc, const lv_style_transition_dsc_t *tr)
{
    if(dsc == NULL) {
        return;
    }
    for(uint32_t s = 0u; s < dsc->phys_n; s++) {
        lv_obj_t *c = dsc->cards[s];
        if(c == NULL || !lv_obj_is_valid(c)) {
            continue;
        }
        lv_obj_set_style_transition(c, tr, LV_PART_MAIN);
    }
}

/** 停稳或异常路径：恢复卡片过渡，使松手后选中态仍可有 `UI_MODE_CARD_SELECT_TRANSITION_MS` 渐显。 */
static void carousel_restore_card_transition(ui_mode_carousel_dsc_t *dsc)
{
    if(dsc == NULL) {
        return;
    }
    dsc->strip_instant_border = false;
    carousel_cards_set_transition(dsc, dsc->card_tr);
}

static void carousel_delete_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_DELETE) {
        return;
    }
    lv_obj_t *carousel = lv_event_get_target(e);
    ui_mode_carousel_dsc_t *dsc = (ui_mode_carousel_dsc_t *)lv_obj_get_user_data(carousel);
    if(dsc != NULL) {
        lv_free(dsc);
        lv_obj_set_user_data(carousel, NULL);
    }
}

static void carousel_add_page(lv_obj_t *carousel, ui_mode_carousel_dsc_t *dsc, ui_shoot_mode_t m, uint32_t phys_idx,
                              const lv_style_transition_dsc_t *card_tr, lv_event_cb_t icon_clicked_cb)
{
    if(phys_idx >= UI_MODE_CAROUSEL_SLOT_MAX) {
        return;
    }
    dsc->mode_at[phys_idx] = m;

    lv_obj_t *page = lv_obj_create(carousel);
    dsc->pages[phys_idx] = page;
    lv_obj_set_size(page, dsc->page_w, (lv_coord_t)UI_MODE_STRIP_ROW_H);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLL_ELASTIC |
                                LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_flag(page, LV_OBJ_FLAG_SNAPPABLE);
    lv_obj_set_layout(page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *card = lv_obj_create(page);
    dsc->cards[phys_idx] = card;
    lv_obj_set_size(card, dsc->card_w, (lv_coord_t)UI_MODE_STRIP_ROW_H);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC |
                             LV_OBJ_FLAG_SCROLL_MOMENTUM);
    if(card_tr != NULL) {
        lv_obj_set_style_transition(card, card_tr, LV_PART_MAIN);
    }

    lv_obj_t *ic = lv_label_create(card);
    lv_label_set_text_static(ic, ui_app_shoot_mode_icon_glyph(m));
    lv_label_set_long_mode(ic, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(ic, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(ic, lv_color_hex(0x2a6ae9), 0);
    lv_obj_set_style_text_align(ic, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_remove_flag(ic, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_flag(ic, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(ic, 8);
    lv_obj_add_event_cb(ic, icon_clicked_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)phys_idx);

    lv_obj_t *nm = lv_label_create(card);
    ui_i18n_bind_label(nm, ui_i18n_shoot_mode_label_id(m));
    ui_style_zone_label(nm);
    lv_label_set_long_mode(nm, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(nm, dsc->card_w - 12);
    lv_obj_set_style_text_align(nm, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_remove_flag(nm, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
}

/**
 * 横向 snap 补全动画：在 `lv_obj_scroll_by` 默认基础上略延长时长，并用 **ease_in_out**，松手吸附更顺、有「阻尼」感。
 */
static void carousel_scroll_begin_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_SCROLL_BEGIN) {
        return;
    }
    lv_anim_t *a = lv_event_get_param(e);
    if(a == NULL) {
        return;
    }
    uint32_t t = lv_anim_get_time(a);
    t = (t * (uint32_t)UI_MODE_SNAP_SCROLL_DURATION_PCT_NUM) / (uint32_t)UI_MODE_SNAP_SCROLL_DURATION_PCT_DEN;
    if(t < (uint32_t)UI_MODE_SNAP_SCROLL_MIN_MS) {
        t = (uint32_t)UI_MODE_SNAP_SCROLL_MIN_MS;
    }
    if(t > (uint32_t)UI_MODE_SNAP_SCROLL_MAX_MS) {
        t = (uint32_t)UI_MODE_SNAP_SCROLL_MAX_MS;
    }
    lv_anim_set_duration(a, t);
    lv_anim_set_path_cb(a, lv_anim_path_ease_in_out);
}

static void carousel_scroll_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_SCROLL) {
        return;
    }
    lv_obj_t *carousel = lv_event_get_target(e);
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc == NULL || dsc->scroll_end_suppress > 0u) {
        return;
    }
    /* 指针/触摸驱动本横条滚动时去掉样式过渡，否则 `UI_MODE_CARD_SELECT_TRANSITION_MS` 会让边框滞后于视口中心 */
    lv_indev_t *indev = lv_indev_active();
    if(indev != NULL && lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER &&
       lv_indev_get_scroll_obj(indev) == carousel) {
        if(!dsc->strip_instant_border) {
            dsc->strip_instant_border = true;
            carousel_cards_set_transition(dsc, NULL);
        }
    }
    if(dsc->on_visual_scroll != NULL) {
        dsc->on_visual_scroll(dsc->callback_user_data);
    }
}

static void carousel_scroll_end_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_SCROLL_END) {
        return;
    }
    lv_obj_t *carousel = lv_event_get_target(e);
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc == NULL || dsc->scroll_end_suppress > 0u) {
        return;
    }

    /* `lv_indev_scroll_throw_handler` 在甩动衰减结束时就发 SCROLL_END，但带 snap 时 `lv_obj_scroll_by(..., ANIM_ON)` 可能仍在跑。
     * 若立刻 `lv_obj_update_snap(OFF)` 会删掉滚动动画并瞬间对齐，手感发「硬」。等滚动动画结束后再归位/提交。 */
    if(lv_event_get_param(e) != NULL && lv_obj_is_scrolling(carousel)) {
        if(dsc->on_visual_scroll != NULL) {
            dsc->on_visual_scroll(dsc->callback_user_data);
        }
        return;
    }

    lv_obj_update_layout(carousel);

    lv_point_t snap_rem;
    lv_indev_scroll_get_snap_dist(carousel, &snap_rem);
    if(snap_rem.x == LV_COORD_MAX || snap_rem.x == LV_COORD_MIN) {
        snap_rem.x = 0;
    }
    if(snap_rem.y == LV_COORD_MAX || snap_rem.y == LV_COORD_MIN) {
        snap_rem.y = 0;
    }

    if(snap_rem.x != 0 || snap_rem.y != 0) {
        dsc->scroll_end_suppress++;
        lv_obj_scroll_by(carousel, snap_rem.x, snap_rem.y, LV_ANIM_ON);
        dsc->scroll_end_suppress--;
        if(dsc->on_visual_scroll != NULL) {
            dsc->on_visual_scroll(dsc->callback_user_data);
        }
        return;
    }

    dsc->scroll_end_suppress++;
    lv_obj_update_snap(carousel, LV_ANIM_OFF);
    dsc->scroll_end_suppress--;
    lv_obj_update_layout(carousel);

    const uint32_t phys_n = dsc->phys_n;
    const uint32_t real_n = dsc->real_n;
    uint32_t si = ui_mode_carousel_nearest_index(carousel);
    if(si >= phys_n) {
        carousel_restore_card_transition(dsc);
        return;
    }

    /* 多段缓冲：左/中/右三块等宽逻辑带；停稳在左右带时无动画对齐到中间带等效槽 */
    if(dsc->loop && real_n >= 2u && phys_n >= real_n * 3u) {
        if(si < real_n && dsc->pages[si + real_n] != NULL && lv_obj_is_valid(dsc->pages[si + real_n])) {
            dsc->scroll_end_suppress++;
            lv_obj_scroll_to_view(dsc->pages[si + real_n], LV_ANIM_OFF);
            lv_obj_update_snap(carousel, LV_ANIM_OFF);
            dsc->scroll_end_suppress--;
            lv_obj_update_layout(carousel);
            si = ui_mode_carousel_nearest_index(carousel);
        }
        else if(si >= 2u * real_n && dsc->pages[si - real_n] != NULL && lv_obj_is_valid(dsc->pages[si - real_n])) {
            dsc->scroll_end_suppress++;
            lv_obj_scroll_to_view(dsc->pages[si - real_n], LV_ANIM_OFF);
            lv_obj_update_snap(carousel, LV_ANIM_OFF);
            dsc->scroll_end_suppress--;
            lv_obj_update_layout(carousel);
            si = ui_mode_carousel_nearest_index(carousel);
        }
    }

    carousel_restore_card_transition(dsc);

    if(dsc->on_mode_committed != NULL) {
        dsc->on_mode_committed(dsc->mode_at[si], dsc->callback_user_data);
    }
    if(dsc->on_visual_scroll != NULL) {
        dsc->on_visual_scroll(dsc->callback_user_data);
    }
}

lv_obj_t *ui_mode_carousel_create(lv_obj_t *parent, lv_coord_t screen_width)
{
    ui_mode_carousel_dsc_t *dsc = lv_malloc_zeroed(sizeof(ui_mode_carousel_dsc_t));
    if(dsc == NULL) {
        return NULL;
    }
    dsc->screen_w = screen_width;

    lv_obj_t *carousel = lv_obj_create(parent);
    lv_obj_set_user_data(carousel, dsc);
    lv_obj_add_event_cb(carousel, carousel_delete_cb, LV_EVENT_DELETE, NULL);

    lv_obj_set_width(carousel, screen_width);
    lv_obj_set_flex_grow(carousel, 1);
    lv_obj_set_style_min_height(carousel, (lv_coord_t)(UI_MODE_STRIP_ROW_H + 24), 0);
    lv_obj_set_style_bg_opa(carousel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(carousel, 0, 0);
    lv_obj_set_style_pad_ver(carousel, 12, 0);

    lv_obj_set_scroll_dir(carousel, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(carousel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(carousel, LV_SCROLL_SNAP_CENTER);
    lv_obj_remove_flag(carousel, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_add_flag(carousel, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_event_cb(carousel, carousel_scroll_begin_cb, LV_EVENT_SCROLL_BEGIN, NULL);
    lv_obj_add_event_cb(carousel, carousel_scroll_cb, LV_EVENT_SCROLL, NULL);
    lv_obj_add_event_cb(carousel, carousel_scroll_end_cb, LV_EVENT_SCROLL_END, NULL);

    lv_obj_set_layout(carousel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(carousel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(carousel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    return carousel;
}

void ui_mode_carousel_set_callbacks(lv_obj_t *carousel, void (*on_visual_scroll)(void *user_data),
                                    void (*on_mode_committed)(ui_shoot_mode_t m, void *user_data), void *user_data)
{
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc == NULL) {
        return;
    }
    dsc->on_visual_scroll = on_visual_scroll;
    dsc->on_mode_committed = on_mode_committed;
    dsc->callback_user_data = user_data;
}

void ui_mode_carousel_configure_strip(lv_obj_t *carousel, lv_coord_t card_w, lv_coord_t page_w, lv_coord_t col_gap)
{
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc == NULL) {
        return;
    }
    dsc->card_w = card_w;
    dsc->page_w = page_w;
    dsc->col_gap = col_gap;
    lv_obj_set_style_pad_column(carousel, col_gap, 0);
    lv_coord_t edge = (lv_coord_t)((dsc->screen_w - page_w) / 2);
    if(edge < 0) {
        edge = 0;
    }
    lv_obj_set_style_pad_left(carousel, edge, 0);
    lv_obj_set_style_pad_right(carousel, edge, 0);
}

void ui_mode_carousel_build(lv_obj_t *carousel, const ui_shoot_mode_t *modes, uint32_t n, bool loop,
                            const lv_style_transition_dsc_t *card_transition, lv_event_cb_t icon_clicked_cb)
{
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc == NULL || icon_clicked_cb == NULL) {
        return;
    }

    while(lv_obj_get_child_count(carousel) > 0) {
        lv_obj_delete(lv_obj_get_child(carousel, 0));
    }
    memset(dsc->pages, 0, sizeof(dsc->pages));
    memset(dsc->cards, 0, sizeof(dsc->cards));
    dsc->phys_n = 0u;
    dsc->real_n = n;
    dsc->loop = loop && (n >= 2u);
    dsc->card_tr = card_transition;
    dsc->strip_instant_border = false;

    if(n == 0u) {
        return;
    }

    uint32_t phys = 0u;
    if(dsc->loop) {
        for(uint32_t rep = 0u; rep < 3u; rep++) {
            for(uint32_t i = 0u; i < n; i++) {
                carousel_add_page(carousel, dsc, modes[i], phys++, card_transition, icon_clicked_cb);
            }
        }
    }
    else {
        for(uint32_t i = 0u; i < n; i++) {
            carousel_add_page(carousel, dsc, modes[i], phys++, card_transition, icon_clicked_cb);
        }
    }
    dsc->phys_n = phys;
}

uint32_t ui_mode_carousel_phys_count(const lv_obj_t *carousel)
{
    const ui_mode_carousel_dsc_t *dsc = carousel_get_dsc((lv_obj_t *)carousel);
    return dsc != NULL ? dsc->phys_n : 0u;
}

uint32_t ui_mode_carousel_real_count(const lv_obj_t *carousel)
{
    const ui_mode_carousel_dsc_t *dsc = carousel_get_dsc((lv_obj_t *)carousel);
    return dsc != NULL ? dsc->real_n : 0u;
}

uint32_t ui_mode_carousel_nearest_index(lv_obj_t *carousel)
{
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc == NULL || dsc->phys_n == 0u) {
        return 0u;
    }
    lv_obj_update_layout(carousel);

    lv_area_t content;
    lv_obj_get_content_coords(carousel, &content);
    const int32_t vp_cx = (content.x1 + content.x2) / 2;

    uint32_t best = 0u;
    lv_coord_t best_d = LV_COORD_MAX;
    for(uint32_t i = 0u; i < dsc->phys_n; i++) {
        lv_obj_t *p = dsc->pages[i];
        if(p == NULL || !lv_obj_is_valid(p)) {
            continue;
        }
        lv_area_t a;
        lv_obj_get_coords(p, &a);
        const int32_t ccx = (a.x1 + a.x2) / 2;
        const lv_coord_t d = LV_ABS(ccx - vp_cx);
        if(d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

void ui_mode_carousel_highlight_slot(lv_obj_t *carousel, uint32_t si)
{
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc == NULL || dsc->phys_n == 0u) {
        return;
    }
    if(si >= dsc->phys_n) {
        si = 0u;
    }
    for(uint32_t s = 0u; s < dsc->phys_n; s++) {
        lv_obj_t *c = dsc->cards[s];
        if(c == NULL || !lv_obj_is_valid(c)) {
            continue;
        }
        if(s == si) {
            lv_obj_set_style_border_width(c, 3, 0);
            lv_obj_set_style_border_color(c, lv_palette_main(LV_PALETTE_BLUE), 0);
            lv_obj_set_style_bg_color(c, lv_color_hex(0xE8F4FF), 0);
        }
        else {
            lv_obj_set_style_border_width(c, 0, 0);
            lv_obj_set_style_border_color(c, lv_color_hex(0xDDDDDD), 0);
            lv_obj_set_style_bg_color(c, lv_color_hex(0xF7F7F7), 0);
        }
    }
}

void ui_mode_carousel_scroll_to_mode(lv_obj_t *carousel, ui_shoot_mode_t want, lv_anim_enable_t anim)
{
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc == NULL || dsc->phys_n == 0u) {
        return;
    }

    uint32_t slot = UINT32_MAX;
    const uint32_t rn = dsc->real_n;
    if(dsc->loop && rn >= 2u && dsc->phys_n >= rn * 3u) {
        for(uint32_t p = rn; p < 2u * rn; p++) {
            if(dsc->mode_at[p] == want) {
                slot = p;
                break;
            }
        }
    }
    if(slot == UINT32_MAX) {
        for(uint32_t p = 0u; p < dsc->phys_n; p++) {
            if(dsc->mode_at[p] == want) {
                slot = p;
                break;
            }
        }
    }

    if(slot == UINT32_MAX || dsc->pages[slot] == NULL || !lv_obj_is_valid(dsc->pages[slot])) {
        return;
    }

    lv_obj_update_layout(carousel);

    if(anim) {
        lv_obj_scroll_to_view(dsc->pages[slot], LV_ANIM_ON);
        ui_mode_carousel_highlight_slot(carousel, slot);
    }
    else {
        dsc->scroll_end_suppress++;
        lv_obj_scroll_to_view(dsc->pages[slot], LV_ANIM_OFF);
        lv_obj_update_snap(carousel, LV_ANIM_OFF);
        dsc->scroll_end_suppress--;
        ui_mode_carousel_highlight_slot(carousel, ui_mode_carousel_nearest_index(carousel));
    }
}

void ui_mode_carousel_program_scroll_begin(lv_obj_t *carousel)
{
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc != NULL) {
        dsc->scroll_end_suppress++;
    }
}

void ui_mode_carousel_program_scroll_end(lv_obj_t *carousel)
{
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc != NULL && dsc->scroll_end_suppress > 0u) {
        dsc->scroll_end_suppress--;
    }
}

lv_obj_t *ui_mode_carousel_page(lv_obj_t *carousel, uint32_t slot)
{
    ui_mode_carousel_dsc_t *dsc = carousel_get_dsc(carousel);
    if(dsc == NULL || slot >= dsc->phys_n) {
        return NULL;
    }
    return dsc->pages[slot];
}

ui_shoot_mode_t ui_mode_carousel_mode_at(const lv_obj_t *carousel, uint32_t slot)
{
    const ui_mode_carousel_dsc_t *dsc = carousel_get_dsc((lv_obj_t *)carousel);
    if(dsc == NULL || slot >= dsc->phys_n) {
        return (ui_shoot_mode_t)0;
    }
    return dsc->mode_at[slot];
}
