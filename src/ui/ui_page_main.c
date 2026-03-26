/**
 * @file ui_page_main.c
 * @brief 相机主界面：状态栏、预览区、底栏；预览区手势(右滑回放/左滑 ISP/下拉全屏控制中心/上滑模式页)；控制中心 8 宫格与系统设置子页。
 * 可翻译文案经 `ui_i18n_bind_label()` 绑定；符号与 Montserrat 专用字体仍用 static 文本。
 * @note 滑动手势与阈值约定见 **`.cursor/ui_swipe_gestures.md`**。
 */
#include "ui_page_main.h"
#include "ui_common.h"
#include "ui_events.h"
#include "ui_i18n.h"
#include "ui_nav.h"
#include "ui_style.h"
#include "ui_font.h"
#include "../logging.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/layouts/grid/lv_grid.h"

static lv_obj_t *s_isp;
static lv_obj_t *s_cc_dim;
static lv_obj_t *s_cc_sheet;
static lv_obj_t *s_mode;
static lv_obj_t *s_cc_lang_dd;
static lv_obj_t *s_cc_grid;
static lv_obj_t *s_cc_settings;
static bool s_isp_open;
static bool s_cc_open;
static bool s_mode_open;
static int32_t s_cc_sheet_h;

/** 主预览区手势：按下起点 */
static lv_point_t s_vp_press;
/** 主预览区：是否跟踪到有效 RELEASED（与 PRESS_LOST 配对） */
static bool s_vp_tracking;

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
    if(s_cc_grid != NULL && lv_obj_is_valid(s_cc_grid)) {
        lv_obj_add_flag(s_cc_grid, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_cc_settings != NULL && lv_obj_is_valid(s_cc_settings)) {
        lv_obj_clear_flag(s_cc_settings, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_cc_settings);
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

/** 系统设置列表项点击（占位日志） */
static void cc_settings_item_clicked_cb(lv_event_t *e)
{
    const ui_str_id_t id = (ui_str_id_t)(uintptr_t)lv_event_get_user_data(e);
    printf("[Settings] %s event triggered\n", ui_i18n_str(id));
    LOG_DEBUG("Settings: %s triggered", ui_i18n_str(id));
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
        lv_obj_set_width(s_isp, 0);
        lv_obj_update_layout(lv_obj_get_parent(s_isp));

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, s_isp);
        lv_anim_set_values(&anim, 0, UI_SIDE_PANEL_W);
        lv_anim_set_exec_cb(&anim, isp_width_anim_cb);
        lv_anim_set_duration(&anim, 220);
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
    if(-dy >= UI_SWIPE_MIN_DY && LV_ABS(dx) <= UI_SWIPE_MAX_ABS_DX && LV_ABS(dy) > LV_ABS(dx)) {
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
    if(dy >= UI_SWIPE_MIN_DY && LV_ABS(dx) <= UI_SWIPE_MAX_ABS_DY && dy > LV_ABS(dx)) {
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

/**
 * @brief 主预览区四向滑动手势。
 *
 * 流程：PRESSED 记点 → PRESS_LOST 放弃 → RELEASED 算 dx/dy。
 * 先滤除微小移动（|dx|、|dy| 均小于各自 MIN 则忽略）。
 * 再按主方向分支：|dx| >= |dy| 为横向（右滑回放、左滑 ISP），否则为纵向（下拉控制中心、上滑模式页）。
 * 各分支内再校验对应 MIN 与正交轴 MAX，并要求主方向位移严格大于另一轴（与回放左滑、控制中心上滑公式一致）。
 */
static void main_viewport_gesture_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    if(indev == NULL) {
        return;
    }

    if(code == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &s_vp_press);
        s_vp_tracking = true;
        return;
    }
    if(code == LV_EVENT_PRESS_LOST) {
        s_vp_tracking = false;
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
    const int adx = LV_ABS(dx);
    const int ady = LV_ABS(dy);

    if(adx < UI_SWIPE_MIN_DX && ady < UI_SWIPE_MIN_DY) {
        return;
    }

    if(adx >= ady) {
        if(dx >= UI_SWIPE_MIN_DX && ady <= UI_SWIPE_MAX_ABS_DY && dx > ady) {
            LOG_DEBUG("用户右滑");
            lv_async_call(ui_nav_replace_with_replay_async, NULL);
        }
        else if(-dx >= UI_SWIPE_MIN_DX && ady <= UI_SWIPE_MAX_ABS_DY && -dx > ady) {
            LOG_DEBUG("左滑 ISP");
            main_set_isp_open(!s_isp_open);
        }
    }
    else {
        if(dy >= UI_SWIPE_MIN_DY && adx <= UI_SWIPE_MAX_ABS_DX && dy > adx) {
            LOG_DEBUG("下拉控制中心");
            main_show_control_center(true);
        }
        else if(-dy >= UI_SWIPE_MIN_DY && adx <= UI_SWIPE_MAX_ABS_DX && -dy > adx) {
            LOG_DEBUG("上滑模式参数");
            main_show_mode_panel(true);
        }
    }
}

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
    lv_obj_set_flex_grow(set_list, 1);
    lv_obj_set_style_bg_opa(set_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(set_list, 0, 0);
    lv_obj_remove_flag(set_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(set_list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(set_list, LV_OBJ_FLAG_EVENT_BUBBLE);
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
        UI_STR_SETTINGS_FACTORY,
        UI_STR_SETTINGS_DEVICE,
    };
    for(unsigned j = 0; j < 3; j++) {
        lv_obj_t *row = lv_obj_create(set_list);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_height(row, 48);
        lv_obj_set_style_bg_color(row, lv_color_hex(UI_ZONE_CYAN), 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(row, cc_settings_item_clicked_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)set_item_ids[j]);
        cc_lang_ctrl_apply_focus_visual(row);
        lv_obj_t *rl = lv_label_create(row);
        ui_i18n_bind_label(rl, set_item_ids[j]);
        lv_label_set_long_mode(rl, LV_LABEL_LONG_CLIP);
        ui_style_zone_label(rl);
        lv_obj_align(rl, LV_ALIGN_LEFT_MID, 12, 0);
    }

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
    lv_label_set_long_mode(title, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *body = lv_label_create(s_mode);
    ui_i18n_bind_label(body, UI_STR_MODE_BODY);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(body, MY_SCREEN_WIDTH - 32);
    ui_style_zone_label(body);
    lv_obj_align(body, LV_ALIGN_CENTER, 0, 0);

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

void ui_page_main_create(lv_obj_t *scr)
{
    ui_i18n_reset_bindings();

    s_isp = NULL;
    s_cc_dim = NULL;
    s_cc_sheet = NULL;
    s_mode = NULL;
    s_cc_lang_dd = NULL;
    s_cc_grid = NULL;
    s_cc_settings = NULL;
    s_isp_open = false;
    s_cc_open = false;
    s_mode_open = false;
    s_cc_sheet_h = 0;

    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    lv_obj_t *note = lv_obj_create(scr);
    lv_obj_set_size(note, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(note, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_set_style_bg_color(note, lv_color_white(), 0);
    lv_obj_set_style_border_color(note, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_border_width(note, 1, 0);
    lv_obj_set_style_pad_all(note, 6, 0);
    lv_obj_t *note_l = lv_label_create(note);
    ui_i18n_bind_label(note_l, UI_STR_NOTE);
    lv_label_set_long_mode(note_l, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(note_l);

    const int mid_h = MY_SCREEN_HEIGHT - UI_TOP_BAR_H - UI_BOTTOM_BAR_H;

    lv_obj_t *status = lv_obj_create(scr);
    lv_obj_set_size(status, MY_SCREEN_WIDTH, UI_TOP_BAR_H);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(status, lv_color_hex(UI_ZONE_CYAN), 0);
    lv_obj_set_style_border_width(status, 0, 0);
    lv_obj_remove_flag(status, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *status_l = lv_label_create(status);
    ui_i18n_bind_label(status_l, UI_STR_STATUS_ZONE);
    lv_label_set_long_mode(status_l, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(status_l);
    lv_obj_center(status_l);
    lv_obj_add_event_cb(status, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)"状态区");

    lv_obj_t *mid = lv_obj_create(scr);
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
    lv_obj_set_size(reserved, UI_SIDE_PANEL_W, LV_PCT(100));
    lv_obj_set_style_bg_color(reserved, lv_color_hex(UI_ZONE_CYAN), 0);
    lv_obj_set_style_border_width(reserved, 0, 0);
    lv_obj_remove_flag(reserved, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *reserved_l = lv_label_create(reserved);
    ui_i18n_bind_label(reserved_l, UI_STR_RESERVED_ZONE);
    lv_label_set_long_mode(reserved_l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(reserved_l, UI_SIDE_PANEL_W - 8);
    ui_style_zone_label(reserved_l);
    lv_obj_center(reserved_l);
    lv_obj_add_event_cb(reserved, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)"预留区");

    lv_obj_t *viewport = lv_obj_create(mid);
    lv_obj_set_flex_grow(viewport, 1);
    lv_obj_set_style_bg_color(viewport, lv_color_white(), 0);
    lv_obj_set_style_border_width(viewport, 1, 0);
    lv_obj_set_style_border_color(viewport, lv_color_hex(0xDDDDDD), 0);
    lv_obj_remove_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(viewport, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(viewport, main_viewport_gesture_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(viewport, main_viewport_gesture_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(viewport, main_viewport_gesture_cb, LV_EVENT_PRESS_LOST, NULL);

    lv_obj_t *eye = lv_label_create(viewport);
    lv_label_set_text_static(eye, LV_SYMBOL_EYE_OPEN);
    lv_label_set_long_mode(eye, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(eye, lv_color_hex(0x90EE90), 0);
    lv_obj_set_style_text_font(eye, &lv_font_montserrat_48, 0);
    lv_obj_center(eye);

    lv_obj_t *youhua_row = lv_obj_create(viewport);
    lv_obj_set_size(youhua_row, LV_PCT(100), 56);
    lv_obj_align(youhua_row, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_remove_flag(youhua_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(youhua_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(youhua_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(youhua_row, 0, 0);
    lv_obj_set_style_pad_all(youhua_row, 4, 0);

    lv_obj_t *youhua_shibie = lv_label_create(youhua_row);
    ui_i18n_bind_label(youhua_shibie, UI_STR_SWIPE_RIGHT_REPLAY);
    lv_label_set_long_mode(youhua_shibie, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(youhua_shibie, LV_PCT(100));
    ui_style_zone_label(youhua_shibie);
    lv_obj_set_style_text_align(youhua_shibie, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(youhua_shibie);

    s_isp = lv_obj_create(mid);
    lv_obj_set_size(s_isp, 0, LV_PCT(100));
    lv_obj_set_style_bg_color(s_isp, lv_color_hex(UI_ZONE_CYAN), 0);
    lv_obj_set_style_border_width(s_isp, 0, 0);
    lv_obj_remove_flag(s_isp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_isp, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *isp_l = lv_label_create(s_isp);
    ui_i18n_bind_label(isp_l, UI_STR_ISP_ZONE);
    lv_label_set_long_mode(isp_l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(isp_l, UI_SIDE_PANEL_W - 8);
    ui_style_zone_label(isp_l);
    lv_obj_center(isp_l);
    lv_obj_add_event_cb(s_isp, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)"ISP参数显示区域");

    lv_obj_t *bottom = lv_obj_create(scr);
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
        lv_obj_set_flex_grow(cell, 1);
        lv_obj_set_height(cell, LV_PCT(100));
        lv_obj_set_style_bg_color(cell, lv_color_hex(UI_ZONE_CYAN), 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *cell_l = lv_label_create(cell);
        ui_i18n_bind_label(cell_l, bottom_ids[i]);
        lv_label_set_long_mode(cell_l, LV_LABEL_LONG_CLIP);
        ui_style_zone_label(cell_l);
        lv_obj_center(cell_l);
        lv_obj_add_event_cb(cell, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)bottom_dbg[i]);
    }

    main_create_control_center(scr);
    main_create_mode_panel(scr);

    lv_obj_move_foreground(note);
}
