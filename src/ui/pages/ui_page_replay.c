/**
 * @file ui_page_replay.c
 * @brief 回放页：顶栏（标题 / 时长 / 删除）、暂停态（上传 + 播放）、播放态（画面占位 + 进度条）；左滑回主页。
 * 视觉与 **`ui_theme.h`** / 控制中心列表行（**`UI_CC_SETTINGS_ROW_*`**）、**`ui_style_cc_interactive_focus`** 对齐。
 * 视频解码见 **`ui_replay_player_load_request()`** 占位实现。
 * @note 左滑阈值见 **`ui_common.h`** 与 **`.cursor/rules/ui_swipe_gestures.md`**。
 */
#include "ui_page_replay.h"
#include "ui_app_state.h"
#include "ui_common.h"
#include "ui_indev.h"
#include "ui_events.h"
#include "ui_i18n.h"
#include "ui_style.h"
#include "../../logging.h"
#include "lvgl/lvgl.h"

/** 播放态底部进度条区域高度（px） */
#define REPLAY_PROGRESS_ZONE_H 52

static lv_obj_t *s_replay_title_paused_l;
static lv_obj_t *s_replay_title_playing_l;
static lv_obj_t *s_replay_mid_paused;
static lv_obj_t *s_replay_mid_playing;
static lv_obj_t *s_replay_bot_paused;
static lv_obj_t *s_replay_bot_playing;
static lv_obj_t *s_replay_mode_glyph;
static lv_obj_t *s_replay_mode_name_l;
static bool s_replay_playing;

/** `ui_app_shoot_mode_observer_register`：模式变更时刷新回放底栏模式字条。 */
static void replay_shoot_mode_observer_cb(void *user_data)
{
    LV_UNUSED(user_data);
    ui_page_replay_sync_shoot_mode_display();
}

/** 按 `ui_app_get_shoot_mode()` 更新暂停态底栏模式符号与名称。 */
void ui_page_replay_sync_shoot_mode_display(void)
{
    ui_app_shoot_mode_ensure_enabled();
    const ui_shoot_mode_t m = ui_app_get_shoot_mode();
    if(s_replay_mode_glyph != NULL && lv_obj_is_valid(s_replay_mode_glyph)) {
        lv_label_set_text_static(s_replay_mode_glyph, ui_app_shoot_mode_icon_glyph(m));
    }
    if(s_replay_mode_name_l != NULL && lv_obj_is_valid(s_replay_mode_name_l)) {
        lv_label_set_text(s_replay_mode_name_l, ui_i18n_str(ui_i18n_shoot_mode_label_id(m)));
        ui_style_zone_label(s_replay_mode_name_l);
    }
}

/** 按 `s_replay_playing` 更新中部与底部高度、对齐。 */
static void replay_sync_layout(lv_obj_t *top_bar)
{
    const lv_coord_t bot_h = s_replay_playing ? (lv_coord_t)REPLAY_PROGRESS_ZONE_H : (lv_coord_t)UI_BOTTOM_BAR_H;
    const lv_coord_t mid_h = (lv_coord_t)MY_SCREEN_HEIGHT - (lv_coord_t)UI_TOP_BAR_H - bot_h;

    if(s_replay_mid_paused != NULL && lv_obj_is_valid(s_replay_mid_paused)) {
        lv_obj_set_height(s_replay_mid_paused, mid_h);
    }
    if(s_replay_mid_playing != NULL && lv_obj_is_valid(s_replay_mid_playing)) {
        lv_obj_set_height(s_replay_mid_playing, mid_h);
    }
    if(s_replay_bot_paused != NULL && lv_obj_is_valid(s_replay_bot_paused)) {
        lv_obj_set_height(s_replay_bot_paused, bot_h);
    }
    if(s_replay_bot_playing != NULL && lv_obj_is_valid(s_replay_bot_playing)) {
        lv_obj_set_height(s_replay_bot_playing, bot_h);
    }
    LV_UNUSED(top_bar);
}

/** 切换暂停 / 播放 UI，并刷新顶栏标题显隐。 */
static void replay_set_playing(lv_obj_t *top_bar, bool playing)
{
    s_replay_playing = playing;
    replay_sync_layout(top_bar);

    if(s_replay_title_paused_l != NULL && lv_obj_is_valid(s_replay_title_paused_l)) {
        if(playing) {
            lv_obj_add_flag(s_replay_title_paused_l, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_clear_flag(s_replay_title_paused_l, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if(s_replay_title_playing_l != NULL && lv_obj_is_valid(s_replay_title_playing_l)) {
        if(playing) {
            lv_obj_clear_flag(s_replay_title_playing_l, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(s_replay_title_playing_l, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if(s_replay_mid_paused != NULL && lv_obj_is_valid(s_replay_mid_paused)) {
        if(playing) {
            lv_obj_add_flag(s_replay_mid_paused, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_clear_flag(s_replay_mid_paused, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if(s_replay_mid_playing != NULL && lv_obj_is_valid(s_replay_mid_playing)) {
        if(playing) {
            lv_obj_clear_flag(s_replay_mid_playing, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(s_replay_mid_playing, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if(s_replay_bot_paused != NULL && lv_obj_is_valid(s_replay_bot_paused)) {
        if(playing) {
            lv_obj_add_flag(s_replay_bot_paused, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_clear_flag(s_replay_bot_paused, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if(s_replay_bot_playing != NULL && lv_obj_is_valid(s_replay_bot_playing)) {
        if(playing) {
            lv_obj_clear_flag(s_replay_bot_playing, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(s_replay_bot_playing, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/** 文本按钮：与控制中心/蓝牙设置行一致（灰底半透明 + 细边框 + 圆角 + 交互描边缩放）。 */
static lv_obj_t *replay_make_text_button(lv_obj_t *parent, ui_str_id_t sid)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(btn, 44, 0);
    lv_obj_set_style_pad_hor(btn, 12, 0);
    lv_obj_set_style_pad_ver(btn, 10, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_CC_SETTINGS_ROW_BG), 0);
    lv_obj_set_style_bg_opa(btn, UI_CC_SETTINGS_ROW_BG_OPA, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
    lv_obj_set_style_border_width(btn, UI_CC_SETTINGS_ROW_BORDER_W, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    ui_style_cc_interactive_focus(btn);
    lv_obj_t *lb = lv_label_create(btn);
    ui_i18n_bind_label(lb, sid);
    ui_style_zone_label(lb);
    lv_label_set_long_mode(lb, LV_LABEL_LONG_CLIP);
    lv_obj_center(lb);
    return btn;
}

/** 将容器设为与设置列表行相同的底/边/圆角（高度由调用方决定）。 */
static void replay_style_panel_like_cc_row(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_CC_SETTINGS_ROW_BG), 0);
    lv_obj_set_style_bg_opa(obj, UI_CC_SETTINGS_ROW_BG_OPA, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
    lv_obj_set_style_border_width(obj, UI_CC_SETTINGS_ROW_BORDER_W, 0);
    lv_obj_set_style_radius(obj, 8, 0);
}

static void replay_stub_delete_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    printf("[Replay] delete (stub)\n");
    LOG_DEBUG("Replay: delete stub");
}

static void replay_stub_upload_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    printf("[Replay] upload (stub)\n");
    LOG_DEBUG("Replay: upload stub");
}

/** 点中央播放：进入播放态并请求加载（当前 `ui_replay_player_load_request` 为空实现）。 */
static void replay_play_clicked_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    lv_obj_t *top = (lv_obj_t *)lv_event_get_user_data(e);
    if(top == NULL || !lv_obj_is_valid(top)) {
        return;
    }
    ui_replay_player_load_request(NULL);
    replay_set_playing(top, true);
    LOG_DEBUG("Replay: entered playing UI");
}

/** 点播放中画面占位：回到暂停态（不停止解码，当前无解码）。 */
static void replay_video_area_clicked_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    lv_obj_t *top = (lv_obj_t *)lv_event_get_user_data(e);
    if(top == NULL || !lv_obj_is_valid(top)) {
        return;
    }
    replay_set_playing(top, false);
    LOG_DEBUG("Replay: back to paused UI");
}

void ui_replay_player_load_request(const char *uri)
{
    if(uri != NULL && uri[0] != '\0') {
        LOG_DEBUG("Replay: load request (stub) uri=%s", uri);
    }
    else {
        LOG_DEBUG("Replay: load request (stub) uri=NULL");
    }
    /* 后续：打开文件/流、创建 `lv_image`/`lv_canvas`/自定义 draw 目标并挂到此页。 */
}

void ui_page_replay_create(lv_obj_t *scr)
{
    ui_i18n_reset_bindings();
    ui_indev_apply_pointer_profile();

    s_replay_title_paused_l = NULL;
    s_replay_title_playing_l = NULL;
    s_replay_mid_paused = NULL;
    s_replay_mid_playing = NULL;
    s_replay_bot_paused = NULL;
    s_replay_bot_playing = NULL;
    s_replay_mode_glyph = NULL;
    s_replay_mode_name_l = NULL;
    s_replay_playing = false;

    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_THEME_SCREEN_BG), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *huifang_swipe = lv_obj_create(scr);
    lv_obj_set_size(huifang_swipe, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);
    lv_obj_align(huifang_swipe, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_remove_flag(huifang_swipe, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(huifang_swipe, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(huifang_swipe, 0, 0);
    lv_obj_set_style_pad_all(huifang_swipe, 0, 0);

    lv_obj_t *swipe_back = lv_obj_create(huifang_swipe);
    lv_obj_set_size(swipe_back, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);
    lv_obj_align(swipe_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(swipe_back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(swipe_back, 0, 0);
    lv_obj_add_flag(swipe_back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(swipe_back, ui_evt_replay_zuohua_pointer_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(swipe_back, ui_evt_replay_zuohua_pointer_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(swipe_back, ui_evt_replay_zuohua_pointer_cb, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *overlay = lv_obj_create(huifang_swipe);
    lv_obj_set_size(overlay, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);
    lv_obj_align(overlay, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *top_bar = lv_obj_create(overlay);
    lv_obj_set_size(top_bar, MY_SCREEN_WIDTH, UI_TOP_BAR_H);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_hor(top_bar, 8, 0);
    lv_obj_set_style_pad_ver(top_bar, 4, 0);
    lv_obj_remove_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
    ui_region_strip_enable_scroll(top_bar);
    lv_obj_set_layout(top_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title_col = lv_obj_create(top_bar);
    lv_obj_set_flex_grow(title_col, 1);
    lv_obj_set_height(title_col, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(title_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_col, 0, 0);
    lv_obj_remove_flag(title_col, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(title_col, 0, 0);

    s_replay_title_paused_l = lv_label_create(title_col);
    ui_i18n_bind_label(s_replay_title_paused_l, UI_STR_REPLAY_ALBUM_PAUSED);
    ui_label_i18n_wrap(s_replay_title_paused_l, MY_SCREEN_WIDTH / 2);
    ui_style_zone_label(s_replay_title_paused_l);
    lv_label_set_long_mode(s_replay_title_paused_l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_replay_title_paused_l, LV_PCT(100));
    lv_obj_align(s_replay_title_paused_l, LV_ALIGN_LEFT_MID, 0, 0);

    s_replay_title_playing_l = lv_label_create(title_col);
    ui_i18n_bind_label(s_replay_title_playing_l, UI_STR_REPLAY_ALBUM_PLAYING);
    ui_label_i18n_wrap(s_replay_title_playing_l, MY_SCREEN_WIDTH / 2);
    ui_style_zone_label(s_replay_title_playing_l);
    lv_label_set_long_mode(s_replay_title_playing_l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_replay_title_playing_l, LV_PCT(100));
    lv_obj_align(s_replay_title_playing_l, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_flag(s_replay_title_playing_l, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *dur_panel = lv_obj_create(top_bar);
    lv_obj_set_size(dur_panel, 140, 40);
    replay_style_panel_like_cc_row(dur_panel);
    lv_obj_set_style_pad_column(dur_panel, 6, 0);
    lv_obj_remove_flag(dur_panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(dur_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dur_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dur_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *dur_l = lv_label_create(dur_panel);
    ui_i18n_bind_label(dur_l, UI_STR_REPLAY_DURATION);
    ui_style_zone_label(dur_l);
    lv_label_set_long_mode(dur_l, LV_LABEL_LONG_CLIP);
    lv_obj_remove_flag(dur_l, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *dur_v = lv_label_create(dur_panel);
    /* 占位须为 ASCII：Unicode “—”(U+2014) 不在静态 Noto 子集内，会显示为方框；数字/冒号用 Montserrat */
    lv_label_set_text_static(dur_v, "00:00");
    ui_style_zone_label(dur_v);
    lv_obj_set_style_text_font(dur_v, &lv_font_montserrat_16, 0);
    lv_label_set_long_mode(dur_v, LV_LABEL_LONG_CLIP);
    lv_obj_remove_flag(dur_v, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *del_top = replay_make_text_button(top_bar, UI_STR_REPLAY_DELETE);
    lv_obj_add_event_cb(del_top, replay_stub_delete_cb, LV_EVENT_CLICKED, NULL);

    const lv_coord_t mid_h_init =
        (lv_coord_t)MY_SCREEN_HEIGHT - (lv_coord_t)UI_TOP_BAR_H - (lv_coord_t)UI_BOTTOM_BAR_H;

    s_replay_mid_paused = lv_obj_create(overlay);
    lv_obj_set_size(s_replay_mid_paused, MY_SCREEN_WIDTH, mid_h_init);
    lv_obj_align(s_replay_mid_paused, LV_ALIGN_TOP_MID, 0, UI_TOP_BAR_H);
    lv_obj_set_style_bg_opa(s_replay_mid_paused, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_replay_mid_paused, 0, 0);
    lv_obj_remove_flag(s_replay_mid_paused, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *almond = lv_obj_create(s_replay_mid_paused);
    lv_obj_set_size(almond, (lv_coord_t)(MY_SCREEN_WIDTH * 55 / 100), 120);
    lv_obj_set_style_radius(almond, 60, 0);
    lv_obj_set_style_bg_color(almond, lv_color_hex(UI_THEME_CC_PAGE_BG), 0);
    lv_obj_set_style_border_color(almond, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
    lv_obj_set_style_border_width(almond, UI_CC_SETTINGS_ROW_BORDER_W, 0);
    lv_obj_remove_flag(almond, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(almond);

    lv_obj_t *upload_btn = replay_make_text_button(s_replay_mid_paused, UI_STR_REPLAY_UPLOAD);
    lv_obj_add_event_cb(upload_btn, replay_stub_upload_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align_to(upload_btn, almond, LV_ALIGN_OUT_LEFT_MID, -16, 0);

    lv_obj_t *play_disc = lv_obj_create(almond);
    lv_obj_set_size(play_disc, 100, 100);
    lv_obj_set_style_radius(play_disc, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(play_disc, lv_color_hex(UI_CC_SETTINGS_ROW_BG), 0);
    lv_obj_set_style_bg_opa(play_disc, UI_CC_SETTINGS_ROW_BG_OPA, 0);
    lv_obj_set_style_border_color(play_disc, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
    lv_obj_set_style_border_width(play_disc, UI_CC_SETTINGS_ROW_BORDER_W, 0);
    lv_obj_remove_flag(play_disc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(play_disc, LV_OBJ_FLAG_CLICKABLE);
    ui_style_cc_interactive_focus(play_disc);
    lv_obj_add_event_cb(play_disc, replay_play_clicked_cb, LV_EVENT_CLICKED, top_bar);
    lv_obj_center(play_disc);

    lv_obj_t *play_txt = lv_label_create(play_disc);
    ui_i18n_bind_label(play_txt, UI_STR_REPLAY_PLAY);
    ui_style_zone_label(play_txt);
    lv_label_set_long_mode(play_txt, LV_LABEL_LONG_CLIP);
    lv_obj_remove_flag(play_txt, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(play_txt);

    s_replay_mid_playing = lv_obj_create(overlay);
    lv_obj_set_size(s_replay_mid_playing, MY_SCREEN_WIDTH, mid_h_init);
    lv_obj_align(s_replay_mid_playing, LV_ALIGN_TOP_MID, 0, UI_TOP_BAR_H);
    lv_obj_set_style_bg_opa(s_replay_mid_playing, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_replay_mid_playing, 0, 0);
    lv_obj_remove_flag(s_replay_mid_playing, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_replay_mid_playing, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *video_frame = lv_obj_create(s_replay_mid_playing);
    lv_obj_set_size(video_frame, MY_SCREEN_WIDTH - 32, mid_h_init - 40);
    lv_obj_set_style_max_height(video_frame, mid_h_init - 24, 0);
    lv_obj_center(video_frame);
    lv_obj_set_style_bg_color(video_frame, lv_color_hex(UI_THEME_CC_PAGE_BG), 0);
    lv_obj_set_style_border_color(video_frame, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
    lv_obj_set_style_border_width(video_frame, UI_CC_SETTINGS_ROW_BORDER_W, 0);
    lv_obj_set_style_radius(video_frame, 8, 0);
    lv_obj_remove_flag(video_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(video_frame, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(video_frame, replay_video_area_clicked_cb, LV_EVENT_CLICKED, top_bar);

    lv_obj_t *eye = lv_label_create(video_frame);
    lv_label_set_text_static(eye, LV_SYMBOL_EYE_OPEN);
    lv_label_set_long_mode(eye, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(eye, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(eye, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
    lv_obj_remove_flag(eye, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(eye);

    s_replay_bot_paused = lv_obj_create(overlay);
    lv_obj_set_size(s_replay_bot_paused, MY_SCREEN_WIDTH, UI_BOTTOM_BAR_H);
    lv_obj_align(s_replay_bot_paused, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(s_replay_bot_paused, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_replay_bot_paused, 0, 0);
    lv_obj_set_style_pad_hor(s_replay_bot_paused, 8, 0);
    lv_obj_remove_flag(s_replay_bot_paused, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(s_replay_bot_paused, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_replay_bot_paused, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_replay_bot_paused, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *mode_cell = lv_obj_create(s_replay_bot_paused);
    lv_obj_set_flex_grow(mode_cell, 1);
    lv_obj_set_height(mode_cell, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(mode_cell, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mode_cell, 0, 0);
    lv_obj_remove_flag(mode_cell, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(mode_cell, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_cell, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_cell, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(mode_cell, 8, 0);

    s_replay_mode_glyph = lv_label_create(mode_cell);
    lv_label_set_long_mode(s_replay_mode_glyph, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(s_replay_mode_glyph, &lv_font_montserrat_20, 0);
    lv_obj_remove_flag(s_replay_mode_glyph, LV_OBJ_FLAG_CLICKABLE);

    s_replay_mode_name_l = lv_label_create(mode_cell);
    lv_label_set_long_mode(s_replay_mode_name_l, LV_LABEL_LONG_WRAP);
    ui_label_i18n_wrap(s_replay_mode_name_l, MY_SCREEN_WIDTH / 2);
    lv_obj_remove_flag(s_replay_mode_name_l, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *del_bot = replay_make_text_button(s_replay_bot_paused, UI_STR_REPLAY_DELETE);
    lv_obj_add_event_cb(del_bot, replay_stub_delete_cb, LV_EVENT_CLICKED, NULL);

    s_replay_bot_playing = lv_obj_create(overlay);
    lv_obj_set_size(s_replay_bot_playing, MY_SCREEN_WIDTH, REPLAY_PROGRESS_ZONE_H);
    lv_obj_align(s_replay_bot_playing, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_replay_bot_playing, lv_color_hex(UI_THEME_CC_PAGE_BG), 0);
    lv_obj_set_style_border_side(s_replay_bot_playing, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(s_replay_bot_playing, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
    lv_obj_set_style_border_width(s_replay_bot_playing, UI_CC_SETTINGS_ROW_BORDER_W, 0);
    lv_obj_set_style_pad_all(s_replay_bot_playing, 6, 0);
    lv_obj_remove_flag(s_replay_bot_playing, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(s_replay_bot_playing, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_replay_bot_playing, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_replay_bot_playing, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(s_replay_bot_playing, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *prog_l = lv_label_create(s_replay_bot_playing);
    ui_i18n_bind_label(prog_l, UI_STR_REPLAY_PROGRESS);
    ui_style_zone_label(prog_l);
    lv_label_set_long_mode(prog_l, LV_LABEL_LONG_CLIP);
    lv_obj_remove_flag(prog_l, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *bar = lv_bar_create(s_replay_bot_playing);
    lv_obj_set_width(bar, MY_SCREEN_WIDTH - 24);
    lv_obj_set_height(bar, 10);
    lv_bar_set_range(bar, 0, 1000);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(UI_CC_SETTINGS_ROW_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, UI_CC_SETTINGS_ROW_BG_OPA, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);

    lv_obj_move_foreground(overlay);

    replay_sync_layout(top_bar);
    ui_page_replay_sync_shoot_mode_display();
    ui_app_shoot_mode_observer_register(replay_shoot_mode_observer_cb, NULL);

    LOG_DEBUG("回放页面已加载（暂停态 UI）");
}
