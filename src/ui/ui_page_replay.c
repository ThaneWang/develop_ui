/**
 * @file ui_page_replay.c
 * @brief 回放页布局：顶栏标题与左滑命中区；事件绑定 `ui_evt_replay_zuohua_pointer_cb`。
 * 固定文案使用 `lv_label_set_text_static()`。
 * @note 左滑判定与主屏横向手势共用阈值，见 **`.cursor/rules/ui_swipe_gestures.md`**。
 */
#include "ui_page_replay.h"
#include "ui_common.h"
#include "ui_indev.h"
#include "ui_events.h"
#include "ui_i18n.h"
#include "ui_style.h"
#include "../logging.h"

/** 构建回放屏：顶栏、左滑命中区与正文绑定 i18n。 */
void ui_page_replay_create(lv_obj_t *scr)
{
    ui_i18n_reset_bindings();
    ui_indev_apply_pointer_profile();

    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *top = lv_obj_create(scr);
    lv_obj_set_size(top, MY_SCREEN_WIDTH, UI_TOP_BAR_H);
    lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_pad_hor(top, 10, 0);
    lv_obj_remove_flag(top, LV_OBJ_FLAG_SCROLLABLE);
    ui_region_strip_enable_scroll(top);

    lv_obj_t *title = lv_label_create(top);
    ui_i18n_bind_label(title, UI_STR_REPLAY_TITLE);
    ui_label_i18n_wrap(title, MY_SCREEN_WIDTH - 24);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 4);

    lv_obj_t *huifang_swipe = lv_obj_create(scr);
    lv_obj_set_size(huifang_swipe, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT - UI_TOP_BAR_H);
    lv_obj_align(huifang_swipe, LV_ALIGN_TOP_MID, 0, UI_TOP_BAR_H);
    lv_obj_remove_flag(huifang_swipe, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(huifang_swipe, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(huifang_swipe, 0, 0);
    lv_obj_set_style_pad_all(huifang_swipe, 0, 0);
    lv_obj_add_flag(huifang_swipe, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(huifang_swipe, ui_evt_replay_zuohua_pointer_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(huifang_swipe, ui_evt_replay_zuohua_pointer_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(huifang_swipe, ui_evt_replay_zuohua_pointer_cb, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *body = lv_label_create(huifang_swipe);
    ui_i18n_bind_label(body, UI_STR_REPLAY_BODY);
    ui_label_i18n_wrap(body, 0);
    lv_obj_set_width(body, LV_PCT(92));
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(body);
    lv_obj_add_flag(body, LV_OBJ_FLAG_EVENT_BUBBLE);

    LOG_DEBUG("回放页面已加载");
}
