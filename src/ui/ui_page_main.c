/**
 * @file ui_page_main.c
 * @brief 相机主界面：状态栏、预览区、底栏；预览区手势(右滑回放/左滑 ISP/下拉全屏控制中心/上滑模式页)；控制中心 8 宫格与系统设置子页。
 * 可翻译文案经 `ui_i18n_bind_label()` 绑定；符号与 Montserrat 专用字体仍用 static 文本。
 * 主屏区划文案无底色、换行与滚动见 `ui_label_i18n_wrap` / `ui_region_strip_enable_scroll` 及 `.cursor/rules.md`「1.1」。
 * @note 滑动手势与阈值约定见 **`.cursor/rules/ui_swipe_gestures.md`**；固件能力章节与 UI 对照见 **`docs/firmware-fw-v1-framework.md`**（版本见文首）、**`docs/ui-fw-v1-mapping.md`**。
 */
#include "ui_page_main.h"
#include "ui_common.h"
#include "ui_display.h"
#include "ui_events.h"
#include "ui_i18n.h"
#include "ui_nav.h"
#include "ui_bt_state.h"
#include "ui_style.h"
#include "ui_font.h"
#include "../logging.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/layouts/grid/lv_grid.h"

#if UI_FEATURE_DISPLAY_ROTATION
static void cc_show_rotation_view(void);
#endif

static void cc_sync_settings_list_geom(void);
static void cc_settings_list_scroll_end_cb(lv_event_t *e);

static lv_obj_t *s_isp;
static lv_obj_t *s_cc_dim;
static lv_obj_t *s_cc_sheet;
static lv_obj_t *s_mode;
static lv_obj_t *s_cc_lang_dd;
static lv_obj_t *s_cc_grid;
static lv_obj_t *s_cc_settings;
/** 系统设置页可滚动列表（用于高度同步、滚动预览、与上滑关闭手势区分） */
static lv_obj_t *s_cc_set_list;
/** 控制中心 `PRESSED` 时记录列表 `scroll_y`，用于判断本次手势是否为列表滚动 */
static int32_t s_cc_setlist_scroll_y_at_press;
#if UI_FEATURE_DISPLAY_ROTATION
static lv_obj_t *s_cc_rotation_panel;
static lv_obj_t *s_cc_rot_switch;
static lv_obj_t *s_cc_rot_angle_list;
static lv_obj_t *s_cc_rot_angle_btns[4];
static bool s_cc_rot_enabled;
static uint8_t s_cc_rot_sel_idx;
#endif
static bool s_isp_open;
static bool s_cc_open;
static bool s_mode_open;
static int32_t s_cc_sheet_h;

/** 主预览区手势：按下起点 */
static lv_point_t s_vp_press;
/** 主预览区：是否跟踪到有效 RELEASED（与 PRESS_LOST 配对） */
static bool s_vp_tracking;

typedef enum {
    MAIN_VP_DIR_NONE = 0,
    MAIN_VP_DIR_RIGHT,
    MAIN_VP_DIR_LEFT,
    MAIN_VP_DIR_DOWN,
    MAIN_VP_DIR_UP,
} main_vp_dir_t;

/** 当前手势锁定的方向（全屏手势，PRESSING 跟手预览） */
static main_vp_dir_t s_vp_dir;
/** 下拉控制中心跟手预览中（尚未 `s_cc_open`） */
static bool s_vp_cc_drag;
/** 上滑模式页跟手预览 */
static bool s_vp_mode_preview;
/** 左滑 ISP 跟手预览（仅从未展开状态拖出） */
static bool s_vp_isp_preview;
/** 右滑回放预显条（无点击，不挡触摸） */
static lv_obj_t *s_vp_replay_peek;
/** 全屏宽、仅中间带高的透明层：眼睛与手势提示相对「主内容区」几何中心固定，不随 ISP 列宽变化 */
static lv_obj_t *s_vp_decor_layer;

static const lv_style_prop_t s_cc_lang_hit_tr_props[] = {
    LV_STYLE_TRANSFORM_SCALE_X,
    LV_STYLE_TRANSFORM_SCALE_Y,
    LV_STYLE_OUTLINE_WIDTH,
    LV_STYLE_OUTLINE_OPA,
    LV_STYLE_PROP_INV,
};

static lv_style_transition_dsc_t s_cc_lang_hit_tr;
static bool s_cc_lang_hit_tr_inited;

static void cc_lang_ctrl_install_transition(lv_obj_t *ctrl)
{
    if(!s_cc_lang_hit_tr_inited) {
        lv_style_transition_dsc_init(&s_cc_lang_hit_tr, s_cc_lang_hit_tr_props, lv_anim_path_ease_out, 120, 0, NULL);
        s_cc_lang_hit_tr_inited = true;
    }
    lv_obj_set_style_transition(ctrl, &s_cc_lang_hit_tr, LV_PART_MAIN);
}

/** 悬停/聚焦/展开：略放大 + 描边（用于控制中心语种下拉） */
static void cc_lang_ctrl_apply_focus_visual(lv_obj_t *ctrl)
{
    lv_obj_set_style_transform_pivot_x(ctrl, lv_pct(50), LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(ctrl, lv_pct(50), LV_PART_MAIN);
    lv_obj_set_style_transform_scale_x(ctrl, LV_SCALE_NONE, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_y(ctrl, LV_SCALE_NONE, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_x(ctrl, 280, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_transform_scale_y(ctrl, 280, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_transform_scale_x(ctrl, 280, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_transform_scale_y(ctrl, 280, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_transform_scale_x(ctrl, 280, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_transform_scale_y(ctrl, 280, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_outline_width(ctrl, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(ctrl, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_outline_width(ctrl, 2, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_outline_opa(ctrl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_outline_color(ctrl, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_outline_pad(ctrl, 2, LV_PART_MAIN | LV_STATE_HOVERED);
    lv_obj_set_style_outline_width(ctrl, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_opa(ctrl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(ctrl, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(ctrl, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(ctrl, 2, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_outline_opa(ctrl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_outline_color(ctrl, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_outline_pad(ctrl, 2, LV_PART_MAIN | LV_STATE_CHECKED);
    cc_lang_ctrl_install_transition(ctrl);
}

static void cc_lang_dd_sync_from_i18n(void)
{
    if(s_cc_lang_dd == NULL || !lv_obj_is_valid(s_cc_lang_dd)) {
        return;
    }
    const uint32_t want = (ui_i18n_get_lang() == UI_LANG_ZH) ? 0u : 1u;
    if(lv_dropdown_get_selected(s_cc_lang_dd) != want) {
        lv_dropdown_set_selected(s_cc_lang_dd, want);
    }
}

static void cc_lang_dd_changed_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    const uint32_t sel = lv_dropdown_get_selected(dd);
    const ui_lang_t lang = (sel == 0u) ? UI_LANG_ZH : UI_LANG_EN;
    if(ui_i18n_get_lang() == lang) {
        return;
    }
    ui_i18n_set_lang(lang);
    ui_i18n_refresh_all();
    LOG_DEBUG("语种切换为 %s", lang == UI_LANG_ZH ? "ZH" : "EN");
}

static const ui_str_id_t s_cc_tile_str_ids[8] = {
    UI_STR_CC_TILE_ROTATION,
    UI_STR_CC_TILE_LOCK_SCREEN,
    UI_STR_CC_TILE_VOICE,
    UI_STR_CC_TILE_SYSTEM,
    UI_STR_CC_TILE_BRIGHTNESS,
    UI_STR_CC_TILE_VOLUME,
    UI_STR_CC_TILE_QUICK,
    UI_STR_CC_TILE_THEME,
};

static const int32_t s_cc_grid_col_dsc[] = {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
};
static const int32_t s_cc_grid_row_dsc[] = {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
};

/** 控制中心：显示 8 宫格，隐藏系统设置子页 */
static void cc_show_grid_view(void)
{
#if UI_FEATURE_DISPLAY_ROTATION
    if(s_cc_rotation_panel != NULL && lv_obj_is_valid(s_cc_rotation_panel)) {
        lv_obj_add_flag(s_cc_rotation_panel, LV_OBJ_FLAG_HIDDEN);
    }
#endif
    if(s_cc_grid != NULL && lv_obj_is_valid(s_cc_grid)) {
        lv_obj_clear_flag(s_cc_grid, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_cc_settings != NULL && lv_obj_is_valid(s_cc_settings)) {
        lv_obj_add_flag(s_cc_settings, LV_OBJ_FLAG_HIDDEN);
    }
}

/** 控制中心：显示系统设置子页，隐藏宫格 */
static void cc_show_settings_view(void)
{
#if UI_FEATURE_DISPLAY_ROTATION
    if(s_cc_rotation_panel != NULL && lv_obj_is_valid(s_cc_rotation_panel)) {
        lv_obj_add_flag(s_cc_rotation_panel, LV_OBJ_FLAG_HIDDEN);
    }
#endif
    if(s_cc_grid != NULL && lv_obj_is_valid(s_cc_grid)) {
        lv_obj_add_flag(s_cc_grid, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_cc_settings != NULL && lv_obj_is_valid(s_cc_settings)) {
        lv_obj_clear_flag(s_cc_settings, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_cc_settings);
        cc_sync_settings_list_geom();
    }
}

/** 系统设置子页：返回宫格；若语种下拉展开则先关闭 */
static void cc_settings_back_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(s_cc_lang_dd != NULL && lv_obj_is_valid(s_cc_lang_dd)) {
        lv_dropdown_close(s_cc_lang_dd);
    }
    cc_show_grid_view();
}

/** 系统设置列表项点击：占位，仅打印（对齐 `docs/firmware-fw-v1-framework.md` §5/§6）；蓝牙进入独立页 */
static void cc_settings_item_clicked_cb(lv_event_t *e)
{
    const ui_str_id_t id = (ui_str_id_t)(uintptr_t)lv_event_get_user_data(e);
    if(id == UI_STR_SETTINGS_BT) {
        printf("[Settings] navigate -> Bluetooth settings\n");
        LOG_DEBUG("Settings: open Bluetooth page");
        lv_async_call(ui_nav_replace_with_bt_settings_async, NULL);
        return;
    }
    printf("[Settings] tap id=%d \"%s\"\n", (int)id, ui_i18n_str(id));
    LOG_DEBUG("Settings tap id=%d %s", (int)id, ui_i18n_str(id));
}

/** 根据 `s_cc_settings` 子控件高度，为列表分配剩余高度，使内容可纵向滚动 */
static void cc_sync_settings_list_geom(void)
{
    if(s_cc_settings == NULL || s_cc_set_list == NULL) {
        return;
    }
    if(!lv_obj_is_valid(s_cc_settings) || !lv_obj_is_valid(s_cc_set_list)) {
        return;
    }
    lv_obj_update_layout(s_cc_settings);
    lv_obj_t *hdr = lv_obj_get_child(s_cc_settings, 0);
    if(hdr == NULL) {
        return;
    }
    const lv_coord_t h_set = lv_obj_get_height(s_cc_settings);
    const lv_coord_t pad_top = lv_obj_get_style_pad_top(s_cc_settings, 0);
    const lv_coord_t pad_bot = lv_obj_get_style_pad_bottom(s_cc_settings, 0);
    const lv_coord_t pad_row = lv_obj_get_style_pad_row(s_cc_settings, 0);
    const lv_coord_t hdr_h = lv_obj_get_height(hdr);
    lv_coord_t list_h = h_set - pad_top - pad_bot - hdr_h - pad_row;
    if(list_h < 64) {
        list_h = 64;
    }
    lv_obj_set_height(s_cc_set_list, list_h);
    lv_obj_update_layout(s_cc_set_list);
}

/** 滚动结束：根据视口中心落在哪一行，打印当前「预览」项（便于下拉浏览选择） */
static void cc_settings_list_scroll_end_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(s_cc_set_list == NULL || !lv_obj_is_valid(s_cc_set_list)) {
        return;
    }
    lv_obj_update_layout(s_cc_set_list);
    const lv_coord_t scroll_y = lv_obj_get_scroll_y(s_cc_set_list);
    const lv_coord_t view_h = lv_obj_get_height(s_cc_set_list);
    if(view_h < 1) {
        return;
    }
    const lv_coord_t mid_y = scroll_y + view_h / 2;
    const uint32_t n = lv_obj_get_child_cnt(s_cc_set_list);
    for(uint32_t i = 0; i < n; i++) {
        lv_obj_t *row = lv_obj_get_child(s_cc_set_list, i);
        if(row == NULL) {
            continue;
        }
        const lv_coord_t y1 = lv_obj_get_y(row);
        const lv_coord_t y2 = y1 + lv_obj_get_height(row);
        if(mid_y >= y1 && mid_y < y2) {
            lv_obj_t *first = lv_obj_get_child(row, 0);
            if(first != NULL && lv_obj_check_type(first, &lv_label_class)) {
                const char *txt = lv_label_get_text(first);
                printf("[Settings] preview (center): \"%s\"\n", txt != NULL ? txt : "");
                LOG_DEBUG("Settings preview: %s", txt != NULL ? txt : "");
            }
            break;
        }
    }
}

/** 控制中心宫格：点击磁贴；索引 3 进入系统设置 */
static void cc_tile_clicked_cb(lv_event_t *e)
{
    const unsigned idx = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(idx >= 8) {
        return;
    }
    if(idx == 3u) {
        cc_show_settings_view();
        printf("[CC] %s -> enter system settings\n", ui_i18n_str(s_cc_tile_str_ids[idx]));
        LOG_DEBUG("CC: system settings opened");
        return;
    }
#if UI_FEATURE_DISPLAY_ROTATION
    if(idx == 0u) {
        cc_show_rotation_view();
        printf("[CC] %s -> rotation panel\n", ui_i18n_str(s_cc_tile_str_ids[idx]));
        LOG_DEBUG("CC: rotation panel opened");
        return;
    }
#endif
    printf("[CC] %s effect triggered\n", ui_i18n_str(s_cc_tile_str_ids[idx]));
    LOG_DEBUG("CC tile: %s triggered", ui_i18n_str(s_cc_tile_str_ids[idx]));
}

static void main_show_control_center(bool show);
static void main_show_mode_panel(bool show);

/** ISP 侧栏宽度动画执行回调 */
static void isp_width_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = var;
    lv_obj_set_width(obj, v);
    lv_obj_t *mid = lv_obj_get_parent(obj);
    if(mid) {
        lv_obj_update_layout(mid);
    }
}

/** ISP 收起动画结束：隐藏对象 */
static void isp_close_done_cb(lv_anim_t *a)
{
    LV_UNUSED(a);
    if(s_isp != NULL) {
        lv_obj_add_flag(s_isp, LV_OBJ_FLAG_HIDDEN);
    }
}

/** 展开/收起右侧 ISP 面板（宽度动画） */
static void main_set_isp_open(bool open)
{
    if(s_isp == NULL) {
        return;
    }
    lv_anim_delete(s_isp, isp_width_anim_cb);

    if(open) {
        if(s_isp_open) {
            return;
        }
        s_isp_open = true;
        lv_obj_clear_flag(s_isp, LV_OBJ_FLAG_HIDDEN);
        lv_coord_t w0 = lv_obj_get_width(s_isp);
        if(w0 < 1) {
            w0 = 0;
            lv_obj_set_width(s_isp, 0);
        }
        lv_obj_t *mid = lv_obj_get_parent(s_isp);
        if(mid) {
            lv_obj_update_layout(mid);
        }
        if(w0 >= UI_SIDE_PANEL_W) {
            lv_obj_set_width(s_isp, UI_SIDE_PANEL_W);
            if(mid) {
                lv_obj_update_layout(mid);
            }
            return;
        }

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, s_isp);
        lv_anim_set_values(&anim, w0, UI_SIDE_PANEL_W);
        lv_anim_set_exec_cb(&anim, isp_width_anim_cb);
        lv_anim_set_duration(&anim, w0 > 0 ? 120 : 220);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
        lv_anim_start(&anim);
    }
    else {
        if(!s_isp_open) {
            return;
        }
        s_isp_open = false;
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, s_isp);
        lv_anim_set_values(&anim, lv_obj_get_width(s_isp), 0);
        lv_anim_set_exec_cb(&anim, isp_width_anim_cb);
        lv_anim_set_duration(&anim, 200);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
        lv_anim_set_completed_cb(&anim, isp_close_done_cb);
        lv_anim_start(&anim);
    }
}

/** 控制中心 sheet 垂直位移动画 */
static void cc_sheet_y_anim_cb(void *var, int32_t v)
{
    lv_obj_set_y(var, v);
}

/** 控制中心收起动画结束：隐藏遮罩与 sheet，清 `s_cc_open` */
static void cc_hide_done_cb(lv_anim_t *a)
{
    LV_UNUSED(a);
    if(s_cc_dim != NULL) {
        lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_cc_sheet != NULL) {
        lv_obj_add_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
    }
    s_cc_open = false;
}

/** 点击半透明遮罩：关闭控制中心 */
static void cc_dim_clicked_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    main_show_control_center(false);
}

/**
 * @brief 打开/关闭全屏控制中心（遮罩 + 自顶向下滑入的 sheet）。
 * @param show true 打开并复位到宫格视图；false 播放收起动画。
 */
static void main_show_control_center(bool show)
{
    if(s_cc_dim == NULL || s_cc_sheet == NULL || s_cc_sheet_h <= 0) {
        return;
    }
    lv_anim_delete(s_cc_sheet, cc_sheet_y_anim_cb);

    if(show) {
        if(s_cc_open) {
            return;
        }
        s_cc_open = true;
        cc_show_grid_view();
        lv_obj_clear_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_cc_dim);
        lv_obj_move_foreground(s_cc_sheet);
        lv_obj_set_y(s_cc_sheet, -s_cc_sheet_h);

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, s_cc_sheet);
        lv_anim_set_values(&anim, -s_cc_sheet_h, 0);
        lv_anim_set_exec_cb(&anim, cc_sheet_y_anim_cb);
        lv_anim_set_duration(&anim, 240);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
        lv_anim_start(&anim);
        cc_lang_dd_sync_from_i18n();
    }
    else {
        if(!s_cc_open) {
            return;
        }
        if(s_cc_lang_dd != NULL && lv_obj_is_valid(s_cc_lang_dd)) {
            lv_dropdown_close(s_cc_lang_dd);
        }
        cc_show_grid_view();
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, s_cc_sheet);
        lv_anim_set_values(&anim, lv_obj_get_y(s_cc_sheet), -s_cc_sheet_h);
        lv_anim_set_exec_cb(&anim, cc_sheet_y_anim_cb);
        lv_anim_set_duration(&anim, 200);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
        lv_anim_set_completed_cb(&anim, cc_hide_done_cb);
        lv_anim_start(&anim);
    }
}

/** 控制中心上滑关闭：按下起点 */
static lv_point_t s_cc_swipe_press;
/** 控制中心上滑：是否跟踪手势 */
static bool s_cc_swipe_track;

/**
 * @brief 控制中心内上滑收起（与主屏「下拉打开」方向相反）。
 *
 * 判定：-dy >= UI_SWIPE_MIN_DY，|dx| <= UI_SWIPE_MAX_ABS_DX，且 |dy| > |dx|（纵向占优）。
 * 子控件（磁贴、设置项等）须对指针事件使用 `LV_EVENT_BUBBLE`，否则事件无法到达 `s_cc_sheet`。
 */
static void cc_sheet_swipe_dismiss_cb(lv_event_t *e)
{
    if(s_cc_sheet == NULL || !lv_obj_is_valid(s_cc_sheet)) {
        return;
    }
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    if(indev == NULL) {
        return;
    }

    if(code == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &s_cc_swipe_press);
        s_cc_swipe_track = true;
        if(s_cc_set_list != NULL && lv_obj_is_valid(s_cc_set_list)) {
            s_cc_setlist_scroll_y_at_press = lv_obj_get_scroll_y(s_cc_set_list);
        }
        else {
            s_cc_setlist_scroll_y_at_press = 0;
        }
        return;
    }
    if(code == LV_EVENT_PRESS_LOST) {
        s_cc_swipe_track = false;
        return;
    }
    if(code != LV_EVENT_RELEASED || !s_cc_swipe_track) {
        return;
    }
    s_cc_swipe_track = false;

    lv_point_t rel;
    lv_indev_get_point(indev, &rel);
    const int dx = rel.x - s_cc_swipe_press.x;
    const int dy = rel.y - s_cc_swipe_press.y;
    /* 系统设置列表发生纵向滚动时，不将手势当作「控制中心上滑关闭」 */
    if(s_cc_set_list != NULL && lv_obj_is_valid(s_cc_set_list)) {
        const int32_t sy = lv_obj_get_scroll_y(s_cc_set_list);
        if(LV_ABS(sy - s_cc_setlist_scroll_y_at_press) > 5) {
            return;
        }
    }
    if(-dy >= UI_SWIPE_COMMIT_DY && LV_ABS(dx) <= UI_SWIPE_MAX_ABS_DX && LV_ABS(dy) > LV_ABS(dx)) {
        LOG_DEBUG("控制中心上滑关闭");
        main_show_control_center(false);
    }
}

/** 模式页淡出结束：隐藏并恢复不透明 */
static void mode_hide_done_cb(lv_anim_t *a)
{
    LV_UNUSED(a);
    if(s_mode != NULL) {
        lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_add_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
    }
}

/** 模式页透明度动画 */
static void mode_opa_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_MAIN);
}

/**
 * @brief 模式参数页：下滑关闭（与「上滑打开」相反）。
 *
 * 判定：dy >= UI_SWIPE_MIN_DY，|dx| <= UI_SWIPE_MAX_ABS_DY，且 dy > |dx|（纵向占优）。
 */
static void mode_panel_gesture_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    if(indev == NULL) {
        return;
    }

    static lv_point_t press;
    static bool track;

    if(code == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &press);
        track = true;
        return;
    }
    if(code == LV_EVENT_PRESS_LOST) {
        track = false;
        return;
    }
    if(code != LV_EVENT_RELEASED || !track) {
        return;
    }
    track = false;

    lv_point_t rel;
    lv_indev_get_point(indev, &rel);
    const int dx = rel.x - press.x;
    const int dy = rel.y - press.y;
    if(dy >= UI_SWIPE_COMMIT_DY && LV_ABS(dx) <= UI_SWIPE_MAX_ABS_DX && dy > LV_ABS(dx)) {
        main_show_mode_panel(false);
    }
}

/** 模式页关闭按钮 */
static void mode_close_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    main_show_mode_panel(false);
}

/** 显示/隐藏模式参数全屏层（淡入淡出） */
static void main_show_mode_panel(bool show)
{
    if(s_mode == NULL) {
        return;
    }
    lv_anim_delete(s_mode, mode_opa_anim_cb);

    if(show) {
        if(s_mode_open) {
            return;
        }
        s_mode_open = true;
        lv_obj_clear_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(s_mode, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_move_foreground(s_mode);

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, s_mode);
        lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_anim_set_exec_cb(&anim, mode_opa_anim_cb);
        lv_anim_set_duration(&anim, 200);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
        lv_anim_start(&anim);
    }
    else {
        if(!s_mode_open) {
            return;
        }
        s_mode_open = false;
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, s_mode);
        lv_anim_set_values(&anim, lv_obj_get_style_opa(s_mode, LV_PART_MAIN), LV_OPA_TRANSP);
        lv_anim_set_exec_cb(&anim, mode_opa_anim_cb);
        lv_anim_set_duration(&anim, 180);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
        lv_anim_set_completed_cb(&anim, mode_hide_done_cb);
        lv_anim_start(&anim);
    }
}

static void main_replay_peek_ensure(lv_obj_t *scr)
{
    if(s_vp_replay_peek != NULL && lv_obj_is_valid(s_vp_replay_peek)) {
        return;
    }
    s_vp_replay_peek = lv_obj_create(scr);
    lv_obj_remove_flag(s_vp_replay_peek, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_vp_replay_peek, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_vp_replay_peek, 0, MY_SCREEN_HEIGHT);
    lv_obj_align(s_vp_replay_peek, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(s_vp_replay_peek, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_vp_replay_peek, LV_OPA_40, 0);
    lv_obj_set_style_border_width(s_vp_replay_peek, 0, 0);
    lv_obj_add_flag(s_vp_replay_peek, LV_OBJ_FLAG_HIDDEN);
}

static void main_replay_peek_set(int32_t permille)
{
    if(s_vp_replay_peek == NULL || !lv_obj_is_valid(s_vp_replay_peek)) {
        return;
    }
    if(permille <= 0) {
        lv_obj_add_flag(s_vp_replay_peek, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_width(s_vp_replay_peek, 0);
        return;
    }
    lv_coord_t w = (lv_coord_t)((int32_t)UI_SWIPE_COMMIT_DX * LV_MIN(permille, 1000) / 1000);
    if(w < 1) {
        w = 1;
    }
    lv_obj_set_width(s_vp_replay_peek, w);
    lv_obj_clear_flag(s_vp_replay_peek, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_vp_replay_peek);
}

static void main_cc_apply_drag_permille(int32_t permille)
{
    if(s_cc_dim == NULL || s_cc_sheet == NULL || s_cc_sheet_h <= 0 || s_cc_open) {
        return;
    }
    if(permille <= 0) {
        s_vp_cc_drag = false;
        lv_anim_delete(s_cc_sheet, cc_sheet_y_anim_cb);
        lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(s_cc_sheet, -s_cc_sheet_h);
        return;
    }
    s_vp_cc_drag = true;
    lv_anim_delete(s_cc_sheet, cc_sheet_y_anim_cb);
    lv_obj_clear_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_cc_dim);
    lv_obj_move_foreground(s_cc_sheet);
    const int32_t y = -s_cc_sheet_h + (permille * (int32_t)s_cc_sheet_h) / 1000;
    lv_obj_set_y(s_cc_sheet, (lv_coord_t)y);
    lv_obj_set_style_bg_opa(s_cc_dim, (lv_opa_t)((int32_t)LV_OPA_50 * LV_MIN(permille, 1000) / 1000), 0);
}

static void main_cc_commit_from_drag(void)
{
    if(s_cc_dim == NULL || s_cc_sheet == NULL || s_cc_sheet_h <= 0) {
        return;
    }
    lv_anim_delete(s_cc_sheet, cc_sheet_y_anim_cb);
    s_vp_cc_drag = false;
    s_cc_open = true;
    cc_show_grid_view();
    lv_obj_clear_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_cc_dim);
    lv_obj_move_foreground(s_cc_sheet);
    lv_obj_set_y(s_cc_sheet, 0);
    lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
    cc_lang_dd_sync_from_i18n();
}

static void vp_cc_drag_cancel_done_cb(lv_anim_t *a)
{
    LV_UNUSED(a);
    s_vp_cc_drag = false;
    if(!s_cc_open) {
        if(s_cc_dim != NULL && lv_obj_is_valid(s_cc_dim)) {
            lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
            lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
        }
        if(s_cc_sheet != NULL && lv_obj_is_valid(s_cc_sheet)) {
            lv_obj_add_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void main_cc_cancel_drag_anim(void)
{
    if(!s_vp_cc_drag || s_cc_open || s_cc_sheet == NULL) {
        s_vp_cc_drag = false;
        return;
    }
    lv_anim_delete(s_cc_sheet, cc_sheet_y_anim_cb);
    lv_coord_t y0 = lv_obj_get_y(s_cc_sheet);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, s_cc_sheet);
    lv_anim_set_values(&anim, y0, -s_cc_sheet_h);
    lv_anim_set_exec_cb(&anim, cc_sheet_y_anim_cb);
    lv_anim_set_duration(&anim, 160);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&anim, vp_cc_drag_cancel_done_cb);
    lv_anim_start(&anim);
}

static void main_mode_apply_drag_permille(int32_t permille)
{
    if(s_mode == NULL || s_mode_open) {
        return;
    }
    if(permille <= 0) {
        s_vp_mode_preview = false;
        lv_anim_delete(s_mode, mode_opa_anim_cb);
        lv_obj_add_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
        return;
    }
    s_vp_mode_preview = true;
    lv_anim_delete(s_mode, mode_opa_anim_cb);
    lv_obj_clear_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(s_mode, (lv_opa_t)((int32_t)LV_OPA_COVER * LV_MIN(permille, 1000) / 1000), LV_PART_MAIN);
    lv_obj_move_foreground(s_mode);
}

static void main_mode_commit_from_drag(void)
{
    if(s_mode == NULL) {
        return;
    }
    lv_anim_delete(s_mode, mode_opa_anim_cb);
    s_vp_mode_preview = false;
    s_mode_open = true;
    lv_obj_clear_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_move_foreground(s_mode);
}

static void main_isp_apply_drag_permille(int32_t permille)
{
    if(s_isp == NULL || s_isp_open) {
        return;
    }
    if(permille <= 0) {
        s_vp_isp_preview = false;
        lv_anim_delete(s_isp, isp_width_anim_cb);
        lv_obj_set_width(s_isp, 0);
        lv_obj_add_flag(s_isp, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t *mid = lv_obj_get_parent(s_isp);
        if(mid) {
            lv_obj_update_layout(mid);
        }
        return;
    }
    s_vp_isp_preview = true;
    lv_anim_delete(s_isp, isp_width_anim_cb);
    lv_obj_clear_flag(s_isp, LV_OBJ_FLAG_HIDDEN);
    lv_coord_t w = (lv_coord_t)((int32_t)UI_SIDE_PANEL_W * LV_MIN(permille, 1000) / 1000);
    lv_obj_set_width(s_isp, w);
    lv_obj_t *mid = lv_obj_get_parent(s_isp);
    if(mid) {
        lv_obj_update_layout(mid);
    }
}

static int32_t main_vp_permille_for_dir(main_vp_dir_t dir, int dx, int dy)
{
    switch(dir) {
        case MAIN_VP_DIR_RIGHT:
            return (dx > 0) ? (int32_t)dx * 1000 / UI_SWIPE_COMMIT_DX : 0;
        case MAIN_VP_DIR_LEFT:
            return (dx < 0) ? (int32_t)(-dx) * 1000 / UI_SWIPE_COMMIT_DX : 0;
        case MAIN_VP_DIR_DOWN:
            return (dy > 0) ? (int32_t)dy * 1000 / UI_SWIPE_COMMIT_DY : 0;
        case MAIN_VP_DIR_UP:
            return (dy < 0) ? (int32_t)(-dy) * 1000 / UI_SWIPE_COMMIT_DY : 0;
        default:
            return 0;
    }
}

static bool main_vp_cross_axis_bad(main_vp_dir_t dir, int adx, int ady)
{
    switch(dir) {
        case MAIN_VP_DIR_RIGHT:
        case MAIN_VP_DIR_LEFT:
            return ady > UI_SWIPE_MAX_ABS_DY;
        case MAIN_VP_DIR_DOWN:
        case MAIN_VP_DIR_UP:
            return adx > UI_SWIPE_MAX_ABS_DX;
        default:
            return false;
    }
}

static bool main_vp_release_commit_ok(main_vp_dir_t dir, int dx, int dy)
{
    const int adx = LV_ABS(dx);
    const int ady = LV_ABS(dy);
    if(dir == MAIN_VP_DIR_NONE) {
        return false;
    }
    if(main_vp_cross_axis_bad(dir, adx, ady)) {
        return false;
    }
    switch(dir) {
        case MAIN_VP_DIR_RIGHT:
            return dx >= UI_SWIPE_COMMIT_DX && ady <= UI_SWIPE_MAX_ABS_DY && dx > ady;
        case MAIN_VP_DIR_LEFT:
            return (-dx) >= UI_SWIPE_COMMIT_DX && ady <= UI_SWIPE_MAX_ABS_DY && (-dx) > ady;
        case MAIN_VP_DIR_DOWN:
            return dy >= UI_SWIPE_COMMIT_DY && adx <= UI_SWIPE_MAX_ABS_DX && dy > adx;
        case MAIN_VP_DIR_UP:
            return (-dy) >= UI_SWIPE_COMMIT_DY && adx <= UI_SWIPE_MAX_ABS_DX && (-dy) > adx;
        default:
            return false;
    }
}

static void main_vp_cancel_drag(bool instant)
{
    s_vp_dir = MAIN_VP_DIR_NONE;

    main_replay_peek_set(0);
    main_mode_apply_drag_permille(0);
    main_isp_apply_drag_permille(0);

    if(s_vp_cc_drag && !s_cc_open && s_cc_sheet != NULL) {
        if(instant) {
            lv_anim_delete(s_cc_sheet, cc_sheet_y_anim_cb);
            s_vp_cc_drag = false;
            lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_y(s_cc_sheet, -s_cc_sheet_h);
            lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
        }
        else {
            main_cc_cancel_drag_anim();
        }
    }
    else {
        s_vp_cc_drag = false;
    }
}

/**
 * @brief 主界面全屏四向滑动手势（绑定在 `scr` + 子控件 EVENT_BUBBLE）。
 *
 * PRESSED 记点；PRESSING 按锁定方向跟手预览（位移达屏宽/高 1/5 为满行程）；RELEASED 达到提交条件则进入对应页。
 * 控制中心/模式层打开时不处理，避免与覆盖层冲突。
 */
static void main_viewport_gesture_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    if(indev == NULL) {
        return;
    }

    if(s_cc_open || s_mode_open) {
        if(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESS_LOST) {
            s_vp_tracking = false;
            s_vp_dir = MAIN_VP_DIR_NONE;
        }
        return;
    }

    if(code == LV_EVENT_PRESSED) {
        main_vp_cancel_drag(true);
        lv_indev_get_point(indev, &s_vp_press);
        s_vp_tracking = true;
        s_vp_dir = MAIN_VP_DIR_NONE;
        return;
    }

    if(code == LV_EVENT_PRESS_LOST) {
        s_vp_tracking = false;
        main_vp_cancel_drag(false);
        s_vp_dir = MAIN_VP_DIR_NONE;
        return;
    }

    if(code == LV_EVENT_PRESSING && s_vp_tracking) {
        lv_point_t cur;
        lv_indev_get_point(indev, &cur);
        const int dx = cur.x - s_vp_press.x;
        const int dy = cur.y - s_vp_press.y;
        const int adx = LV_ABS(dx);
        const int ady = LV_ABS(dy);

        if(s_vp_dir == MAIN_VP_DIR_NONE) {
            if(adx < UI_SWIPE_DIR_LOCK_PX && ady < UI_SWIPE_DIR_LOCK_PX) {
                return;
            }
            if(adx >= ady) {
                s_vp_dir = (dx > 0) ? MAIN_VP_DIR_RIGHT : MAIN_VP_DIR_LEFT;
            }
            else {
                s_vp_dir = (dy > 0) ? MAIN_VP_DIR_DOWN : MAIN_VP_DIR_UP;
            }
        }

        if(main_vp_cross_axis_bad(s_vp_dir, adx, ady)) {
            s_vp_tracking = false;
            main_vp_cancel_drag(false);
            s_vp_dir = MAIN_VP_DIR_NONE;
            return;
        }

        int32_t pm = main_vp_permille_for_dir(s_vp_dir, dx, dy);
        if(pm > 1000) {
            pm = 1000;
        }

        switch(s_vp_dir) {
            case MAIN_VP_DIR_RIGHT:
                if(scr != NULL) {
                    main_replay_peek_ensure(scr);
                }
                main_replay_peek_set(pm);
                main_cc_apply_drag_permille(0);
                main_mode_apply_drag_permille(0);
                main_isp_apply_drag_permille(0);
                break;
            case MAIN_VP_DIR_LEFT:
                main_replay_peek_set(0);
                main_cc_apply_drag_permille(0);
                main_mode_apply_drag_permille(0);
                if(!s_isp_open) {
                    main_isp_apply_drag_permille(pm);
                }
                break;
            case MAIN_VP_DIR_DOWN:
                main_replay_peek_set(0);
                main_mode_apply_drag_permille(0);
                main_isp_apply_drag_permille(0);
                main_cc_apply_drag_permille(pm);
                break;
            case MAIN_VP_DIR_UP:
                main_replay_peek_set(0);
                main_cc_apply_drag_permille(0);
                main_isp_apply_drag_permille(0);
                main_mode_apply_drag_permille(pm);
                break;
            default:
                break;
        }
        return;
    }

    if(code != LV_EVENT_RELEASED || !s_vp_tracking) {
        return;
    }
    s_vp_tracking = false;

    lv_point_t rel;
    lv_indev_get_point(indev, &rel);
    const int dx = rel.x - s_vp_press.x;
    const int dy = rel.y - s_vp_press.y;
    const int adxr = LV_ABS(dx);
    const int adyr = LV_ABS(dy);

    if(s_vp_dir == MAIN_VP_DIR_NONE) {
        if(adxr < UI_SWIPE_DIR_LOCK_PX && adyr < UI_SWIPE_DIR_LOCK_PX) {
            main_vp_cancel_drag(false);
            s_vp_dir = MAIN_VP_DIR_NONE;
            return;
        }
        if(adxr >= adyr) {
            s_vp_dir = (dx > 0) ? MAIN_VP_DIR_RIGHT : MAIN_VP_DIR_LEFT;
        }
        else {
            s_vp_dir = (dy > 0) ? MAIN_VP_DIR_DOWN : MAIN_VP_DIR_UP;
        }
    }

    if(main_vp_cross_axis_bad(s_vp_dir, adxr, adyr)) {
        main_vp_cancel_drag(false);
        s_vp_dir = MAIN_VP_DIR_NONE;
        return;
    }

    const bool commit = main_vp_release_commit_ok(s_vp_dir, dx, dy);

    if(commit) {
        switch(s_vp_dir) {
            case MAIN_VP_DIR_RIGHT:
                LOG_DEBUG("用户右滑");
                main_replay_peek_set(0);
                main_cc_apply_drag_permille(0);
                main_mode_apply_drag_permille(0);
                main_isp_apply_drag_permille(0);
                s_vp_cc_drag = false;
                lv_async_call(ui_nav_replace_with_replay_async, NULL);
                break;
            case MAIN_VP_DIR_LEFT:
                LOG_DEBUG("左滑 ISP");
                main_replay_peek_set(0);
                main_cc_apply_drag_permille(0);
                main_mode_apply_drag_permille(0);
                s_vp_isp_preview = false;
                main_set_isp_open(!s_isp_open);
                break;
            case MAIN_VP_DIR_DOWN:
                LOG_DEBUG("下拉控制中心");
                main_replay_peek_set(0);
                main_mode_apply_drag_permille(0);
                main_isp_apply_drag_permille(0);
                if(!s_cc_open) {
                    main_cc_commit_from_drag();
                }
                break;
            case MAIN_VP_DIR_UP:
                LOG_DEBUG("上滑模式参数");
                main_replay_peek_set(0);
                main_cc_apply_drag_permille(0);
                main_isp_apply_drag_permille(0);
                if(!s_mode_open) {
                    main_mode_commit_from_drag();
                }
                break;
            default:
                main_vp_cancel_drag(false);
                break;
        }
    }
    else {
        main_vp_cancel_drag(false);
    }
    s_vp_dir = MAIN_VP_DIR_NONE;
}

#if UI_FEATURE_DISPLAY_ROTATION

/*
 * 用户语义是“顺时针 0/90/180/270”，而 LVGL 显示旋转方向与绘制角度定义相反。
 * 这里做映射：CW90 -> LV_DISPLAY_ROTATION_270，CW270 -> LV_DISPLAY_ROTATION_90。
 */
static const lv_display_rotation_t s_cc_rot_map[4] = {
    LV_DISPLAY_ROTATION_0,
    LV_DISPLAY_ROTATION_270,
    LV_DISPLAY_ROTATION_180,
    LV_DISPLAY_ROTATION_90,
};

static void cc_rot_sync_from_display(void)
{
    lv_display_t *d = lv_display_get_default();
    if(d == NULL) {
        return;
    }
    const lv_display_rotation_t r = lv_display_get_rotation(d);
    s_cc_rot_enabled = (r != LV_DISPLAY_ROTATION_0);
    switch(r) {
        case LV_DISPLAY_ROTATION_0:
            s_cc_rot_sel_idx = 0;
            break;
        case LV_DISPLAY_ROTATION_270: /* CW 90 */
            s_cc_rot_sel_idx = 1;
            break;
        case LV_DISPLAY_ROTATION_180:
            s_cc_rot_sel_idx = 2;
            break;
        case LV_DISPLAY_ROTATION_90: /* CW 270 */
            s_cc_rot_sel_idx = 3;
            break;
        default:
            s_cc_rot_sel_idx = 0;
            break;
    }
}

static void cc_rot_refresh_angle_focus(void)
{
    for(unsigned i = 0; i < 4; i++) {
        lv_obj_t *b = s_cc_rot_angle_btns[i];
        if(b == NULL || !lv_obj_is_valid(b)) {
            continue;
        }
        if(i == s_cc_rot_sel_idx) {
            lv_obj_set_style_outline_width(b, 3, 0);
            lv_obj_set_style_outline_opa(b, LV_OPA_COVER, 0);
            lv_obj_set_style_outline_color(b, lv_palette_main(LV_PALETTE_BLUE), 0);
            lv_obj_set_style_outline_pad(b, 2, 0);
        }
        else {
            lv_obj_set_style_outline_width(b, 0, 0);
            lv_obj_set_style_outline_opa(b, LV_OPA_TRANSP, 0);
        }
    }
}

static void cc_rot_apply_from_ui(void)
{
    if(!s_cc_rot_enabled) {
        ui_display_apply_rotation(LV_DISPLAY_ROTATION_0);
        return;
    }
    ui_display_apply_rotation(s_cc_rot_map[s_cc_rot_sel_idx]);
    lv_obj_t *scr = lv_scr_act();
    if(scr != NULL && lv_obj_is_valid(scr)) {
        lv_obj_update_layout(scr);
    }
}

static void cc_rot_angle_clicked_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    const unsigned i = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(i >= 4) {
        return;
    }
    s_cc_rot_sel_idx = (uint8_t)i;
    cc_rot_refresh_angle_focus();
    cc_rot_apply_from_ui();
}

static void cc_rot_switch_changed_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    lv_obj_t *sw = lv_event_get_target(e);
    s_cc_rot_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if(s_cc_rot_angle_list != NULL && lv_obj_is_valid(s_cc_rot_angle_list)) {
        if(s_cc_rot_enabled) {
            lv_obj_remove_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
        }
    }
    cc_rot_apply_from_ui();
}

static void cc_rotation_back_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    cc_show_grid_view();
}

static void cc_show_rotation_view(void)
{
    if(s_cc_rotation_panel == NULL || !lv_obj_is_valid(s_cc_rotation_panel)) {
        return;
    }
    cc_rot_sync_from_display();
    if(s_cc_grid != NULL && lv_obj_is_valid(s_cc_grid)) {
        lv_obj_add_flag(s_cc_grid, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_cc_settings != NULL && lv_obj_is_valid(s_cc_settings)) {
        lv_obj_add_flag(s_cc_settings, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_remove_flag(s_cc_rotation_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_cc_rotation_panel);
    if(s_cc_rot_switch != NULL && lv_obj_is_valid(s_cc_rot_switch)) {
        if(s_cc_rot_enabled) {
            lv_obj_add_state(s_cc_rot_switch, LV_STATE_CHECKED);
        }
        else {
            lv_obj_remove_state(s_cc_rot_switch, LV_STATE_CHECKED);
        }
    }
    if(s_cc_rot_angle_list != NULL && lv_obj_is_valid(s_cc_rot_angle_list)) {
        if(s_cc_rot_enabled) {
            lv_obj_remove_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
        }
    }
    cc_rot_refresh_angle_focus();
}

static void main_cc_create_rotation_panel(lv_obj_t *cc_body)
{
    s_cc_rotation_panel = lv_obj_create(cc_body);
    lv_obj_set_size(s_cc_rotation_panel, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_cc_rotation_panel, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(s_cc_rotation_panel, 0, 0);
    lv_obj_set_style_pad_all(s_cc_rotation_panel, 8, 0);
    lv_obj_remove_flag(s_cc_rotation_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_cc_rotation_panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_cc_rotation_panel, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(s_cc_rotation_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_layout(s_cc_rotation_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_cc_rotation_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_cc_rotation_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_cc_rotation_panel, 10, 0);

    lv_obj_t *rh = lv_obj_create(s_cc_rotation_panel);
    lv_obj_set_width(rh, LV_PCT(100));
    lv_obj_set_height(rh, 48);
    lv_obj_set_style_bg_opa(rh, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rh, 0, 0);
    lv_obj_remove_flag(rh, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(rh, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(rh, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_layout(rh, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rh, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rh, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(rh, 12, 0);

    lv_obj_t *rb = lv_obj_create(rh);
    lv_obj_set_size(rb, 72, 40);
    lv_obj_set_style_bg_color(rb, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_border_width(rb, 0, 0);
    lv_obj_remove_flag(rb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(rb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(rb, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(rb, cc_rotation_back_cb, LV_EVENT_CLICKED, NULL);
    cc_lang_ctrl_apply_focus_visual(rb);
    lv_obj_t *rbl = lv_label_create(rb);
    ui_i18n_bind_label(rbl, UI_STR_SETTINGS_BACK);
    lv_label_set_long_mode(rbl, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(rbl);
    lv_obj_center(rbl);

    lv_obj_t *rt = lv_label_create(rh);
    ui_i18n_bind_label(rt, UI_STR_CC_ROT_TITLE);
    lv_label_set_long_mode(rt, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(rt);
    lv_obj_set_flex_grow(rt, 1);

    lv_obj_t *en_row = lv_obj_create(s_cc_rotation_panel);
    lv_obj_set_width(en_row, LV_PCT(100));
    lv_obj_set_height(en_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(en_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(en_row, 0, 0);
    lv_obj_remove_flag(en_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(en_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(en_row, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_layout(en_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(en_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(en_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(en_row, 12, 0);

    lv_obj_t *en_lbl = lv_label_create(en_row);
    ui_i18n_bind_label(en_lbl, UI_STR_CC_ROT_ENABLE);
    ui_label_i18n_wrap(en_lbl, MY_SCREEN_WIDTH - 120);
    lv_obj_set_flex_grow(en_lbl, 1);

    s_cc_rot_switch = lv_switch_create(en_row);
    lv_obj_add_flag(s_cc_rot_switch, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(s_cc_rot_switch, cc_rot_switch_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    cc_lang_ctrl_apply_focus_visual(s_cc_rot_switch);

    s_cc_rot_angle_list = lv_obj_create(s_cc_rotation_panel);
    lv_obj_set_width(s_cc_rot_angle_list, LV_PCT(100));
    lv_obj_set_flex_grow(s_cc_rot_angle_list, 1);
    lv_obj_set_style_bg_opa(s_cc_rot_angle_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_cc_rot_angle_list, 0, 0);
    lv_obj_remove_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_layout(s_cc_rot_angle_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_cc_rot_angle_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_cc_rot_angle_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_cc_rot_angle_list, 8, 0);
    lv_obj_add_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);

    static const ui_str_id_t angle_ids[4] = {
        UI_STR_CC_ROT_ANGLE_0,
        UI_STR_CC_ROT_ANGLE_90,
        UI_STR_CC_ROT_ANGLE_180,
        UI_STR_CC_ROT_ANGLE_270,
    };
    for(unsigned i = 0; i < 4; i++) {
        lv_obj_t *b = lv_obj_create(s_cc_rot_angle_list);
        s_cc_rot_angle_btns[i] = b;
        lv_obj_set_width(b, LV_PCT(100));
        lv_obj_set_height(b, 44);
        lv_obj_set_style_bg_color(b, lv_color_hex(UI_ZONE_CYAN), 0);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);
        cc_lang_ctrl_apply_focus_visual(b);
        lv_obj_add_event_cb(b, cc_rot_angle_clicked_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *bl = lv_label_create(b);
        ui_i18n_bind_label(bl, angle_ids[i]);
        lv_label_set_long_mode(bl, LV_LABEL_LONG_CLIP);
        ui_style_zone_label(bl);
        lv_obj_center(bl);
    }
    cc_rot_refresh_angle_focus();
}

#endif /* UI_FEATURE_DISPLAY_ROTATION */

/**
 * @brief 创建全屏控制中心：遮罩、sheet、宫格与设置子页。
 * @note 为让 `cc_sheet_swipe_dismiss_cb` 收到从磁贴/列表起始的滑动，相关容器与子项均设置 `LV_OBJ_FLAG_EVENT_BUBBLE`。
 */
static void main_create_control_center(lv_obj_t *scr)
{
    s_cc_sheet_h = MY_SCREEN_HEIGHT;

    s_cc_dim = lv_obj_create(scr);
    lv_obj_set_size(s_cc_dim, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);
    lv_obj_set_pos(s_cc_dim, 0, 0);
    lv_obj_set_style_bg_color(s_cc_dim, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
    lv_obj_set_style_border_width(s_cc_dim, 0, 0);
    lv_obj_remove_flag(s_cc_dim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_cc_dim, cc_dim_clicked_cb, LV_EVENT_CLICKED, NULL);

    s_cc_sheet = lv_obj_create(scr);
    lv_obj_set_size(s_cc_sheet, MY_SCREEN_WIDTH, s_cc_sheet_h);
    lv_obj_set_pos(s_cc_sheet, 0, -s_cc_sheet_h);
    lv_obj_set_style_bg_color(s_cc_sheet, lv_color_hex(0xE8E8E8), 0);
    lv_obj_set_style_border_width(s_cc_sheet, 0, 0);
    lv_obj_set_style_pad_left(s_cc_sheet, 12, 0);
    lv_obj_set_style_pad_right(s_cc_sheet, 12, 0);
    lv_obj_set_style_pad_top(s_cc_sheet, 10, 0);
    lv_obj_set_style_pad_bottom(s_cc_sheet, 12, 0);
    lv_obj_remove_flag(s_cc_sheet, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_layout(s_cc_sheet, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_cc_sheet, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_cc_sheet, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_cc_sheet, 8, 0);

    lv_obj_t *cc_title = lv_label_create(s_cc_sheet);
    ui_i18n_bind_label(cc_title, UI_STR_CC_TITLE);
    lv_label_set_long_mode(cc_title, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(cc_title);

    lv_obj_t *cc_body = lv_obj_create(s_cc_sheet);
    lv_obj_set_width(cc_body, LV_PCT(100));
    lv_obj_set_flex_grow(cc_body, 1);
    lv_obj_set_style_bg_opa(cc_body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cc_body, 0, 0);
    lv_obj_remove_flag(cc_body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(cc_body, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cc_body, LV_OBJ_FLAG_EVENT_BUBBLE);

    s_cc_grid = lv_obj_create(cc_body);
    lv_obj_set_size(s_cc_grid, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(s_cc_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_cc_grid, 0, 0);
    lv_obj_set_style_pad_row(s_cc_grid, 8, 0);
    lv_obj_set_style_pad_column(s_cc_grid, 8, 0);
    lv_obj_remove_flag(s_cc_grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_cc_grid, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_cc_grid, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_layout(s_cc_grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(s_cc_grid, s_cc_grid_col_dsc, s_cc_grid_row_dsc);

    for(unsigned i = 0; i < 8; i++) {
        lv_obj_t *cell = lv_obj_create(s_cc_grid);
        const int c = (int)(i % 4u);
        const int r = (int)(i / 4u);
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, c, 1, LV_GRID_ALIGN_STRETCH, r, 1);
        lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(cell, lv_color_hex(UI_ZONE_CYAN), 0);
        lv_obj_set_style_radius(cell, 10, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_layout(cell, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        cc_lang_ctrl_apply_focus_visual(cell);
        lv_obj_add_event_cb(cell, cc_tile_clicked_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_add_flag(cell, LV_OBJ_FLAG_EVENT_BUBBLE);

        lv_obj_t *lb = lv_label_create(cell);
        ui_i18n_bind_label(lb, s_cc_tile_str_ids[i]);
        lv_label_set_long_mode(lb, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lb, LV_PCT(92));
        lv_obj_set_style_text_align(lb, LV_TEXT_ALIGN_CENTER, 0);
        ui_style_zone_label(lb);
    }

    s_cc_settings = lv_obj_create(cc_body);
    lv_obj_set_size(s_cc_settings, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_cc_settings, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(s_cc_settings, 0, 0);
    lv_obj_set_style_pad_all(s_cc_settings, 8, 0);
    lv_obj_remove_flag(s_cc_settings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_cc_settings, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_cc_settings, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(s_cc_settings, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_layout(s_cc_settings, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_cc_settings, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_cc_settings, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_cc_settings, 10, 0);

    lv_obj_t *set_hdr = lv_obj_create(s_cc_settings);
    lv_obj_set_width(set_hdr, LV_PCT(100));
    lv_obj_set_height(set_hdr, 48);
    lv_obj_set_style_bg_opa(set_hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(set_hdr, 0, 0);
    lv_obj_remove_flag(set_hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(set_hdr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(set_hdr, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_layout(set_hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(set_hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(set_hdr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(set_hdr, 12, 0);

    lv_obj_t *back_btn = lv_obj_create(set_hdr);
    lv_obj_set_size(back_btn, 72, 40);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_remove_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(back_btn, cc_settings_back_cb, LV_EVENT_CLICKED, NULL);
    cc_lang_ctrl_apply_focus_visual(back_btn);
    lv_obj_t *back_l = lv_label_create(back_btn);
    ui_i18n_bind_label(back_l, UI_STR_SETTINGS_BACK);
    lv_label_set_long_mode(back_l, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(back_l);
    lv_obj_center(back_l);

    lv_obj_t *set_title = lv_label_create(set_hdr);
    ui_i18n_bind_label(set_title, UI_STR_SETTINGS_TITLE);
    lv_label_set_long_mode(set_title, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(set_title);
    lv_obj_set_flex_grow(set_title, 1);

    lv_obj_t *set_list = lv_obj_create(s_cc_settings);
    lv_obj_set_width(set_list, LV_PCT(100));
    /* 高度由 `cc_sync_settings_list_geom()` 按剩余空间计算，避免 flex 将列表撑满内容导致无法滚动 */
    lv_obj_set_style_bg_opa(set_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(set_list, 0, 0);
    s_cc_set_list = set_list;
    lv_obj_add_flag(set_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(set_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(set_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(set_list, LV_OBJ_FLAG_CLICKABLE);
    /* 列表内滚动/点击勿冒泡到 s_cc_sheet，避免与上滑关闭控制中心冲突 */
    lv_obj_remove_flag(set_list, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(set_list, cc_settings_list_scroll_end_cb, LV_EVENT_SCROLL_END, NULL);
    lv_obj_set_layout(set_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(set_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(set_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(set_list, 8, 0);

    lv_obj_t *lang_row = lv_obj_create(set_list);
    lv_obj_set_width(lang_row, LV_PCT(100));
    lv_obj_set_height(lang_row, 48);
    lv_obj_set_style_bg_opa(lang_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lang_row, 0, 0);
    lv_obj_remove_flag(lang_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(lang_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(lang_row, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_layout(lang_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lang_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lang_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lang_cap = lv_label_create(lang_row);
    ui_i18n_bind_label(lang_cap, UI_STR_CC_LANG_LABEL);
    lv_label_set_long_mode(lang_cap, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(lang_cap);

    s_cc_lang_dd = lv_dropdown_create(lang_row);
    lv_dropdown_set_options_static(s_cc_lang_dd, "中文\nEnglish");
    lv_dropdown_set_selected(s_cc_lang_dd, ui_i18n_get_lang() == UI_LANG_ZH ? 0u : 1u);
    lv_obj_set_style_min_width(s_cc_lang_dd, 140, LV_PART_MAIN);
    lv_obj_set_height(s_cc_lang_dd, 40);
    lv_obj_set_style_bg_color(s_cc_lang_dd, lv_color_hex(UI_ZONE_CYAN), LV_PART_MAIN);
    lv_obj_set_style_radius(s_cc_lang_dd, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s_cc_lang_dd, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s_cc_lang_dd, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_cc_lang_dd, 0, LV_PART_MAIN);
    {
        const lv_font_t *f = ui_font_cjk();
        lv_obj_set_style_text_font(s_cc_lang_dd, f != NULL ? f : LV_FONT_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_cc_lang_dd, lv_color_black(), LV_PART_MAIN);
    }
    lv_obj_add_flag(s_cc_lang_dd, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(s_cc_lang_dd, cc_lang_dd_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    cc_lang_ctrl_apply_focus_visual(s_cc_lang_dd);

    lv_obj_t *dd_list = lv_dropdown_get_list(s_cc_lang_dd);
    if(dd_list != NULL) {
        const lv_font_t *f = ui_font_cjk();
        lv_obj_set_style_text_font(dd_list, f != NULL ? f : LV_FONT_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_text_font(dd_list, f != NULL ? f : LV_FONT_DEFAULT, LV_PART_SELECTED);
    }

    static const ui_str_id_t set_item_ids[] = {
        UI_STR_SETTINGS_DATETIME,
        UI_STR_SETTINGS_POWER_SLEEP,
        UI_STR_SETTINGS_BATTERY,
        UI_STR_SETTINGS_THERMAL,
        UI_STR_SETTINGS_SDCARD,
        UI_STR_SETTINGS_FIRMWARE,
        UI_STR_SETTINGS_LOG,
        UI_STR_SETTINGS_SECURITY,
        UI_STR_SETTINGS_BT,
        UI_STR_SETTINGS_WIFI,
        UI_STR_SETTINGS_USB,
        UI_STR_SETTINGS_EXPORT,
        UI_STR_SETTINGS_DEVICE,
        UI_STR_SETTINGS_FACTORY,
    };
    for(unsigned j = 0; j < (unsigned)(sizeof(set_item_ids) / sizeof(set_item_ids[0])); j++) {
        lv_obj_t *row = lv_obj_create(set_list);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_height(row, 48);
        lv_obj_set_style_bg_color(row, lv_color_hex(UI_ZONE_CYAN), 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(row, cc_settings_item_clicked_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)set_item_ids[j]);
        cc_lang_ctrl_apply_focus_visual(row);
        lv_obj_t *rl = lv_label_create(row);
        ui_i18n_bind_label(rl, set_item_ids[j]);
        lv_label_set_long_mode(rl, LV_LABEL_LONG_CLIP);
        ui_style_zone_label(rl);
        lv_obj_align(rl, LV_ALIGN_LEFT_MID, 12, 0);
    }

#if UI_FEATURE_DISPLAY_ROTATION
    main_cc_create_rotation_panel(cc_body);
#endif

    cc_sync_settings_list_geom();

    lv_obj_add_event_cb(s_cc_sheet, cc_sheet_swipe_dismiss_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_cc_sheet, cc_sheet_swipe_dismiss_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_cc_sheet, cc_sheet_swipe_dismiss_cb, LV_EVENT_PRESS_LOST, NULL);

    s_cc_open = false;
}

static void main_create_mode_panel(lv_obj_t *scr)
{
    s_mode = lv_obj_create(scr);
    lv_obj_set_size(s_mode, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);
    lv_obj_set_pos(s_mode, 0, 0);
    lv_obj_set_style_bg_color(s_mode, lv_color_white(), 0);
    lv_obj_set_style_border_width(s_mode, 0, 0);
    lv_obj_remove_flag(s_mode, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_mode, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_mode, mode_panel_gesture_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_mode, mode_panel_gesture_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_mode, mode_panel_gesture_cb, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *title = lv_label_create(s_mode);
    ui_i18n_bind_label(title, UI_STR_MODE_TITLE);
    ui_label_i18n_wrap(title, MY_SCREEN_WIDTH - 96);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *body = lv_label_create(s_mode);
    ui_i18n_bind_label(body, UI_STR_MODE_BODY);
    ui_label_i18n_wrap(body, MY_SCREEN_WIDTH - 32);
    lv_obj_align_to(body, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);

    lv_obj_t *close_btn = lv_obj_create(s_mode);
    lv_obj_set_size(close_btn, 72, 40);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -8, 8);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_border_width(close_btn, 0, 0);
    lv_obj_remove_flag(close_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(close_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(close_btn, mode_close_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_l = lv_label_create(close_btn);
    lv_label_set_text_static(close_l, "X");
    lv_label_set_long_mode(close_l, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(close_l, &lv_font_montserrat_16, 0);
    lv_obj_center(close_l);

    s_mode_open = false;
}

/** 预览区手势说明一行：纯文案 Label + 箭头符号，不参与触摸/冒泡（字条展示） */
static void main_vp_hint_row(lv_obj_t *col, ui_str_id_t gest_id, const char *sym, ui_str_id_t act_id)
{
    lv_obj_t *row = lv_obj_create(col);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_column(row, 10, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);

    lv_obj_t *lg = lv_label_create(row);
    ui_i18n_bind_label(lg, gest_id);
    ui_style_zone_label(lg);
    lv_label_set_long_mode(lg, LV_LABEL_LONG_WRAP);
    lv_obj_remove_flag(lg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(lg, LV_PCT(40));
    lv_obj_set_style_text_align(lg, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_flex_grow(lg, 1);

    lv_obj_t *ic = lv_label_create(row);
    lv_label_set_text_static(ic, sym);
    lv_obj_set_style_text_font(ic, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(ic, lv_color_hex(0x4480e8), 0);
    lv_obj_set_style_text_align(ic, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_long_mode(ic, LV_LABEL_LONG_CLIP);
    lv_obj_remove_flag(ic, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(ic, 28);

    lv_obj_t *la = lv_label_create(row);
    ui_i18n_bind_label(la, act_id);
    ui_style_zone_label(la);
    lv_label_set_long_mode(la, LV_LABEL_LONG_WRAP);
    lv_obj_remove_flag(la, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(la, LV_PCT(40));
    lv_obj_set_style_text_align(la, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_flex_grow(la, 1);
}

/** 在 parent 下添加四行手势提示列（纯展示字条，无滚动、无触摸） */
static lv_obj_t *main_vp_add_hint_column(lv_obj_t *parent)
{
    lv_obj_t *gcol = lv_obj_create(parent);
    lv_obj_remove_flag(gcol, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(gcol, LV_PCT(100));
    lv_obj_set_height(gcol, LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(gcol, 260, 0);
    lv_obj_set_style_bg_opa(gcol, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gcol, 0, 0);
    lv_obj_set_style_pad_row(gcol, 6, 0);
    lv_obj_set_layout(gcol, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(gcol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(gcol, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    main_vp_hint_row(gcol, UI_STR_VP_GEST_RIGHT, LV_SYMBOL_RIGHT, UI_STR_VP_ACT_REPLAY);
    main_vp_hint_row(gcol, UI_STR_VP_GEST_LEFT, LV_SYMBOL_LEFT, UI_STR_VP_ACT_ISP);
    main_vp_hint_row(gcol, UI_STR_VP_GEST_DOWN, LV_SYMBOL_DOWN, UI_STR_VP_ACT_CC);
    main_vp_hint_row(gcol, UI_STR_VP_GEST_UP, LV_SYMBOL_UP, UI_STR_VP_ACT_MODE);
    return gcol;
}

/**
 * 在 scr 上建透明层（仅顶栏与底栏之间的全屏宽条带），眼睛与提示相对**该条带**几何中心对齐。
 * 与 `mid`/ISP 列宽无关，左滑展开 ISP 时装饰不随中间列平移；层不接收点击，手势仍落到下层。
 */
static void main_scr_build_preview_decor(lv_obj_t *scr, lv_coord_t mid_h)
{
    s_vp_decor_layer = lv_obj_create(scr);
    lv_obj_set_size(s_vp_decor_layer, MY_SCREEN_WIDTH, mid_h);
    lv_obj_align(s_vp_decor_layer, LV_ALIGN_TOP_MID, 0, UI_TOP_BAR_H);
    lv_obj_set_style_bg_opa(s_vp_decor_layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_vp_decor_layer, 0, 0);
    lv_obj_remove_flag(s_vp_decor_layer, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *eye = lv_label_create(s_vp_decor_layer);
    lv_label_set_text_static(eye, LV_SYMBOL_EYE_OPEN);
    lv_label_set_long_mode(eye, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(eye, lv_color_hex(0x7dce9a), 0);
    lv_obj_set_style_text_font(eye, &lv_font_montserrat_40, 0);
    lv_obj_remove_flag(eye, LV_OBJ_FLAG_CLICKABLE);
    /* 整体上移，避免四行提示在条带下半部被裁切或贴底栏 */
    lv_obj_align(eye, LV_ALIGN_CENTER, 0, -110);

    lv_obj_t *gcol = main_vp_add_hint_column(s_vp_decor_layer);
    lv_obj_set_width(gcol, MY_SCREEN_WIDTH - 40);
    lv_obj_update_layout(gcol);
    lv_obj_align_to(gcol, eye, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void ui_page_main_create(lv_obj_t *scr)
{
    ui_i18n_reset_bindings();

#if UI_FEATURE_DISPLAY_ROTATION
    ui_display_apply_rotation(LV_DISPLAY_ROTATION_0);
#endif

    s_isp = NULL;
    s_cc_dim = NULL;
    s_cc_sheet = NULL;
    s_mode = NULL;
    s_cc_lang_dd = NULL;
    s_cc_grid = NULL;
    s_cc_settings = NULL;
    s_cc_set_list = NULL;
#if UI_FEATURE_DISPLAY_ROTATION
    s_cc_rotation_panel = NULL;
    s_cc_rot_switch = NULL;
    s_cc_rot_angle_list = NULL;
    for(unsigned ri = 0; ri < 4; ri++) {
        s_cc_rot_angle_btns[ri] = NULL;
    }
    s_cc_rot_enabled = false;
    s_cc_rot_sel_idx = 0;
#endif
    s_isp_open = false;
    s_cc_open = false;
    s_mode_open = false;
    s_cc_sheet_h = 0;
    s_vp_dir = MAIN_VP_DIR_NONE;
    s_vp_cc_drag = false;
    s_vp_mode_preview = false;
    s_vp_isp_preview = false;
    s_vp_replay_peek = NULL;
    s_vp_decor_layer = NULL;

    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    const int mid_h = MY_SCREEN_HEIGHT - UI_TOP_BAR_H - UI_BOTTOM_BAR_H;

    lv_obj_t *status = lv_obj_create(scr);
    lv_obj_add_flag(status, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(status, MY_SCREEN_WIDTH, UI_TOP_BAR_H);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(status, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status, 0, 0);
    lv_obj_set_style_pad_hor(status, 8, 0);
    lv_obj_set_style_pad_ver(status, 4, 0);
    lv_obj_remove_flag(status, LV_OBJ_FLAG_SCROLLABLE);
    ui_region_strip_enable_scroll(status);
    lv_obj_t *status_l = lv_label_create(status);
    ui_i18n_bind_label(status_l, UI_STR_STATUS_ZONE);
    ui_label_i18n_wrap(status_l, MY_SCREEN_WIDTH - 16);
    lv_obj_set_style_text_align(status_l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(status_l, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_add_event_cb(status, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)"状态区");

    lv_obj_t *mid = lv_obj_create(scr);
    lv_obj_add_flag(mid, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(mid, MY_SCREEN_WIDTH, mid_h);
    lv_obj_align(mid, LV_ALIGN_TOP_MID, 0, UI_TOP_BAR_H);
    lv_obj_set_style_bg_opa(mid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mid, 0, 0);
    lv_obj_set_style_pad_all(mid, 0, 0);
    lv_obj_remove_flag(mid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(mid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *reserved = lv_obj_create(mid);
    lv_obj_add_flag(reserved, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(reserved, UI_SIDE_PANEL_W, LV_PCT(100));
    lv_obj_set_style_bg_opa(reserved, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(reserved, 0, 0);
    lv_obj_set_style_pad_ver(reserved, 6, 0);
    lv_obj_remove_flag(reserved, LV_OBJ_FLAG_SCROLLABLE);
    ui_region_strip_enable_scroll(reserved);
    lv_obj_t *reserved_l = lv_label_create(reserved);
    ui_i18n_bind_label(reserved_l, UI_STR_RESERVED_ZONE);
    ui_label_i18n_wrap(reserved_l, UI_SIDE_PANEL_W - 8);
    lv_obj_align(reserved_l, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_add_event_cb(reserved, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)"预留区");

    lv_obj_t *viewport = lv_obj_create(mid);
    lv_obj_add_flag(viewport, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_flex_grow(viewport, 1);
    lv_obj_set_style_bg_opa(viewport, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(viewport, 0, 0);
    lv_obj_remove_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(viewport, LV_OBJ_FLAG_CLICKABLE);

    s_isp = lv_obj_create(mid);
    lv_obj_add_flag(s_isp, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(s_isp, 0, LV_PCT(100));
    lv_obj_set_style_bg_opa(s_isp, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_isp, 0, 0);
    lv_obj_set_style_pad_ver(s_isp, 6, 0);
    lv_obj_remove_flag(s_isp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_isp, LV_OBJ_FLAG_HIDDEN);
    ui_region_strip_enable_scroll(s_isp);
    lv_obj_set_layout(s_isp, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_isp, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_isp, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *isp_l = lv_label_create(s_isp);
    ui_i18n_bind_label(isp_l, UI_STR_ISP_ZONE);
    ui_label_i18n_wrap(isp_l, UI_SIDE_PANEL_W - 8);
    lv_obj_add_event_cb(s_isp, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)"ISP参数显示区域");

    lv_obj_t *bottom = lv_obj_create(scr);
    lv_obj_add_flag(bottom, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(bottom, MY_SCREEN_WIDTH, UI_BOTTOM_BAR_H);
    lv_obj_align(bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(bottom, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottom, 0, 0);
    lv_obj_set_style_pad_all(bottom, 0, 0);
    lv_obj_remove_flag(bottom, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(bottom, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    static const ui_str_id_t bottom_ids[] = {
        UI_STR_BOTTOM_MODE_DISP,
        UI_STR_BOTTOM_MODE_PARAM,
        UI_STR_BOTTOM_VIEW_CTRL,
    };
    static const char *const bottom_dbg[] = {
        "模式显示区",
        "模式参数区",
        "视图控制",
    };

    for(size_t i = 0; i < 3; i++) {
        lv_obj_t *cell = lv_obj_create(bottom);
        lv_obj_add_flag(cell, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_flex_grow(cell, 1);
        lv_obj_set_height(cell, LV_PCT(100));
        lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_style_pad_hor(cell, 6, 0);
        lv_obj_set_style_pad_ver(cell, 4, 0);
        lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        ui_region_strip_enable_scroll(cell);
        lv_obj_t *cell_l = lv_label_create(cell);
        ui_i18n_bind_label(cell_l, bottom_ids[i]);
        ui_label_i18n_wrap(cell_l, MY_SCREEN_WIDTH / 3 - 20);
        lv_obj_align(cell_l, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_text_align(cell_l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_add_event_cb(cell, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)bottom_dbg[i]);
    }

    main_scr_build_preview_decor(scr, mid_h);
    lv_obj_move_foreground(s_vp_decor_layer);

    /**
     * 蓝牙状态字条：挂在 scr 上、置于顶栏视觉区，且不可点击。
     * 若放在可滚动的 status 内，纵向拖动易被顶栏当成滚动，干扰「下拉控制中心」跟手（main_viewport_gesture_cb）。
     */
    lv_obj_t *bt_hint = lv_label_create(scr);
    ui_i18n_bind_label(bt_hint, UI_STR_BT_STATUS_ON);
    ui_style_zone_label(bt_hint);
    lv_label_set_long_mode(bt_hint, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(bt_hint, 168);
    lv_obj_set_style_text_align(bt_hint, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_align(bt_hint, LV_ALIGN_TOP_RIGHT, -6, 4);
    lv_obj_remove_flag(bt_hint, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(bt_hint, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(bt_hint, LV_OBJ_FLAG_EVENT_BUBBLE);
    if(!ui_bt_is_enabled()) {
        lv_obj_add_flag(bt_hint, LV_OBJ_FLAG_HIDDEN);
    }

    main_create_control_center(scr);
    main_create_mode_panel(scr);

    lv_obj_add_event_cb(scr, main_viewport_gesture_cb, LV_EVENT_PRESSED, scr);
    lv_obj_add_event_cb(scr, main_viewport_gesture_cb, LV_EVENT_PRESSING, scr);
    lv_obj_add_event_cb(scr, main_viewport_gesture_cb, LV_EVENT_RELEASED, scr);
    lv_obj_add_event_cb(scr, main_viewport_gesture_cb, LV_EVENT_PRESS_LOST, scr);
}
