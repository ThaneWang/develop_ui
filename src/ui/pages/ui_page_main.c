/**
 * @file ui_page_main.c
 * @brief 相机主界面：状态栏、预览区、底栏；预览区手势(右滑回放/左滑 ISP/下拉全屏控制中心/上滑模式页)；控制中心 8 宫格与系统设置子页。
 * 拍摄模式单一数据源为 `ui_app`：`ui_app_set_shoot_mode` 在横条 `SCROLL_END` **立即**写全局状态（观察者刷新顶栏/底栏/快切）；横条卡片选中边框/底色用 **`UI_MODE_CARD_SELECT_TRANSITION_MS`** 过渡，与甩动惯性（`UI_INDEV_SCROLL_THROW_PCT`）分工。
 * 全屏手势跟手：**遮罩透明度** 随露出千分比渐变（上限半透）；**右滑进回放无右缘条**；控制中心/模式层：**遮罩** + 内容 **`lv_obj_set_y`**（不用 **`translate_y`**），主界面 **不** 位移；**上滑跟手首次露出**与 **建横条后** 即 **`main_mode_strip_scroll_to_current(false)`**，避免先见默认横条再跳当前模式。松手按 **`UI_PANEL_SNAP_OPEN_PERMILLE`** 吸附展开或 **ease_out** 收回；程序化关页 **滑出动画**。
 * 可翻译文案经 `ui_i18n_bind_label()` 绑定；符号与 Montserrat 专用字体仍用 static 文本。
 * 主屏区划文案无底色、换行与滚动见 `ui_label_i18n_wrap` / `ui_region_strip_enable_scroll` 及 `.cursor/rules.md`「1.1」。
 * @note 滑动手势与阈值约定见 **`.cursor/rules/ui_swipe_gestures.md`**；固件能力章节与 UI 对照见 **`docs/firmware-fw-v1-framework.md`**（版本见文首）、**`docs/ui-fw-v1-mapping.md`**。
 */
#include "ui_page_main.h"
#include "ui_app_state.h"
#include "ui_common.h"
#include "ui_settings_config.h"
#include "ui_events.h"
#include "ui_i18n.h"
#include "ui_nav.h"
#include "ui_bt_nav.h"
#include "ui_bt_state.h"
#include "ui_boot_debug_log.h"
#include "ui_cc_settings_state.h"
#include "ui_style.h"
#include "ui_font.h"
#include "ui_indev.h"
#include "ui_hw_hal.h"
#include "ui_mode_carousel.h"
#include "../../logging.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/core/lv_obj_event_private.h"
#include "lvgl/src/layouts/grid/lv_grid.h"

/** 模式卡片选中态（边框/底色）过渡 */
static lv_style_transition_dsc_t s_mode_card_tr;
static bool s_mode_card_tr_inited;

static void cc_sync_settings_list_geom(void);
static void cc_settings_list_scroll_end_cb(lv_event_t *e);
static void cc_show_rotation_view(void);

static void main_refresh_status_bar(void);
static void main_refresh_bottom_mode_cell(void);
static void main_refresh_cc_quick_tile_label(void);
static void main_shoot_mode_observer_cb(void *user_data);
static void main_mode_strip_scroll_to_current(bool scroll_anim);
static void main_show_control_center(bool show);
static void main_show_mode_panel(bool show);
static void main_mode_update_card_selection(void);
static void mode_carousel_on_visual_scroll(void *user_data);
static void mode_carousel_on_mode_committed(ui_shoot_mode_t m, void *user_data);
static void mode_strip_apply_neighbor_delta(int32_t delta);
static void mode_strip_hit_test_center_only_cb(lv_event_t *e);
static void mode_tap_left_strip_cb(lv_event_t *e);
static void mode_tap_right_strip_cb(lv_event_t *e);

static lv_obj_t *s_isp;
static lv_obj_t *s_cc_dim;
static lv_obj_t *s_cc_sheet;
/** 模式全屏层遮罩（与控制中心 `s_cc_dim` 一致：主界面不位移，仅叠层） */
static lv_obj_t *s_mode_dim;
static lv_obj_t *s_mode;
/** 模式页横向条：`ui_mode_carousel`（多段缓冲循环，见 `docs/ui-mode-strip-carousel-options.md`） */
static lv_obj_t *s_mode_strip;
/** 横条 `HIT_TEST` 左右不吸收宽度，与宽触区 **`2×(page_w+gap)`** 一致；`main_create_mode_panel` 写入 */
static lv_coord_t s_mode_tap_side_miss_w;
/** 底栏左格：当前拍摄模式符号 + 名称（与 `ui_app_get_shoot_mode()` 同步） */
static lv_obj_t *s_bottom_mode_icon_l;
static lv_obj_t *s_bottom_mode_name_l;
/** 顶栏根容器（用于文案变更后 `update_layout`） */
static lv_obj_t *s_status_bar;
/** 顶栏：存储文案、模式符号、电量、蓝牙字条 */
static lv_obj_t *s_status_storage_l;
static lv_obj_t *s_status_mode_ic;
static lv_obj_t *s_status_bat_l;
static lv_obj_t *s_chrome_bt_l;
static lv_obj_t *s_cc_lang_dd;
static lv_obj_t *s_cc_grid;
/** 控制中心宫格索引 6：文案为「快切 + 当前模式名」，不走 `ui_i18n_bind_label`。 */
static lv_obj_t *s_cc_quick_tile_l;
static lv_obj_t *s_cc_settings;
/** 系统设置页可滚动列表（用于高度同步、滚动预览、与上滑关闭手势区分） */
static lv_obj_t *s_cc_set_list;
/** 控制中心 `PRESSED` 时记录列表 `scroll_y`，用于判断本次手势是否为列表滚动 */
static int32_t s_cc_setlist_scroll_y_at_press;
static lv_obj_t *s_cc_rotation_panel;
static lv_obj_t *s_cc_rot_switch;
static lv_obj_t *s_cc_rot_angle_list;
static lv_obj_t *s_cc_rot_angle_btns[4];
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

/**
 * 单次按下周期内已选定的 **竖直** 语义：防止下拉跟手拉回原点后误锁 **上滑**（或对称情况）。
 * 在 **PRESSED / RELEASED / PRESS_LOST** 时清零；滑回按压点附近解锁主向时 **不** 清零。
 */
typedef enum {
    MAIN_VP_VERT_LATCH_NONE = 0,
    MAIN_VP_VERT_LATCH_DOWN,
    MAIN_VP_VERT_LATCH_UP,
} main_vp_vert_latch_t;
static main_vp_vert_latch_t s_vp_vert_latch;

/** 当前手势锁定的方向（全屏手势，PRESSING 跟手预览） */
static main_vp_dir_t s_vp_dir;
/** 下拉控制中心跟手预览中（尚未 `s_cc_open`） */
static bool s_vp_cc_drag;
/** 上滑模式页跟手预览 */
static bool s_vp_mode_preview;
/** 左滑 ISP 跟手预览（仅从未展开状态拖出） */
static bool s_vp_isp_preview;
/** 全屏宽、仅中间带高的透明层：眼睛与手势提示相对「主内容区」几何中心固定，不随 ISP 列宽变化 */
static lv_obj_t *s_vp_decor_layer;

/** 按电量百分比返回 LVGL 电池符号字符串。 */
static const char *main_battery_glyph(uint8_t pct)
{
    if(pct >= 90u) {
        return LV_SYMBOL_BATTERY_FULL;
    }
    if(pct >= 60u) {
        return LV_SYMBOL_BATTERY_3;
    }
    if(pct >= 30u) {
        return LV_SYMBOL_BATTERY_2;
    }
    if(pct >= 10u) {
        return LV_SYMBOL_BATTERY_1;
    }
    return LV_SYMBOL_BATTERY_EMPTY;
}

/** 根据 `ui_app_state` / `ui_bt` 刷新顶栏存储、模式图标、电量与蓝牙字条显隐。 */
static void main_refresh_status_bar(void)
{
    if(s_status_storage_l != NULL && lv_obj_is_valid(s_status_storage_l)) {
        lv_label_set_text_fmt(s_status_storage_l, ui_i18n_str(UI_STR_STORAGE_FREE_FMT),
                              (unsigned)ui_app_get_storage_free_gb());
    }
    if(s_status_mode_ic != NULL && lv_obj_is_valid(s_status_mode_ic)) {
        lv_label_set_text_static(s_status_mode_ic, ui_app_shoot_mode_icon_glyph(ui_app_get_shoot_mode()));
    }
    if(s_status_bat_l != NULL && lv_obj_is_valid(s_status_bat_l)) {
        const uint8_t p = ui_app_get_battery_percent();
        lv_label_set_text_fmt(s_status_bat_l, "%s %u%%", main_battery_glyph(p), (unsigned)p);
    }
    if(s_chrome_bt_l != NULL && lv_obj_is_valid(s_chrome_bt_l)) {
        if(ui_bt_is_enabled()) {
            lv_obj_clear_flag(s_chrome_bt_l, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(s_chrome_bt_l, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if(s_status_bar != NULL && lv_obj_is_valid(s_status_bar)) {
        lv_obj_update_layout(s_status_bar);
    }
    main_refresh_bottom_mode_cell();
    main_refresh_cc_quick_tile_label();
}

/** `ui_app_shoot_mode_observer_register`：模式变更时刷新顶栏/底栏/快切磁贴；面板未开时同步横条 scroll，避免下次上滑先见默认列表再跳。 */
static void main_shoot_mode_observer_cb(void *user_data)
{
    LV_UNUSED(user_data);
    main_refresh_status_bar();
    if(s_mode_strip != NULL && lv_obj_is_valid(s_mode_strip) && s_mode != NULL && lv_obj_is_valid(s_mode) && !s_mode_open && !s_vp_mode_preview) {
        ui_app_shoot_mode_ensure_enabled();
        lv_obj_update_layout(s_mode);
        ui_mode_carousel_scroll_to_mode(s_mode_strip, ui_app_get_shoot_mode(), LV_ANIM_OFF);
    }
}

/** 控制中心「快切」磁贴：与底栏左格共用 `ui_app_get_shoot_mode()` 文案。 */
static void main_refresh_cc_quick_tile_label(void)
{
    if(s_cc_quick_tile_l == NULL || !lv_obj_is_valid(s_cc_quick_tile_l)) {
        return;
    }
    ui_app_shoot_mode_ensure_enabled();
    lv_label_set_text_fmt(s_cc_quick_tile_l, "%s\n%s", ui_i18n_str(UI_STR_CC_TILE_QUICK),
                          ui_i18n_str(ui_i18n_shoot_mode_label_id(ui_app_get_shoot_mode())));
    ui_style_zone_label(s_cc_quick_tile_l);
}

/** 刷新底栏左格：仅显示当前拍摄模式（符号 + i18n 名称）。 */
static void main_refresh_bottom_mode_cell(void)
{
    ui_app_shoot_mode_ensure_enabled();
    const ui_shoot_mode_t m = ui_app_get_shoot_mode();
    if(s_bottom_mode_icon_l != NULL && lv_obj_is_valid(s_bottom_mode_icon_l)) {
        lv_label_set_text_static(s_bottom_mode_icon_l, ui_app_shoot_mode_icon_glyph(m));
    }
    if(s_bottom_mode_name_l != NULL && lv_obj_is_valid(s_bottom_mode_name_l)) {
        lv_label_set_text(s_bottom_mode_name_l, ui_i18n_str(ui_i18n_shoot_mode_label_id(m)));
        ui_style_zone_label(s_bottom_mode_name_l);
    }
}

/** 若当前模式未开放或非法，则钳到第一个已开放项（见 `ui_app_state.h` 中 `UI_SHOOT_ENABLE_*`）。 */
static void main_mode_shoot_mode_clamp_default(void)
{
    ui_app_shoot_mode_ensure_enabled();
}

/** `ui_mode_carousel` 跟手：按视口中心刷新卡片高亮（不写回 `ui_app`）。 */
static void mode_carousel_on_visual_scroll(void *user_data)
{
    LV_UNUSED(user_data);
    main_mode_update_card_selection();
}

/** `ui_mode_carousel` 停稳：**立即** `ui_app_set_shoot_mode`（持久化 + 观察者刷新顶栏/底栏）；随后 `on_visual_scroll` 更新卡片高亮，选中态样式见 `UI_MODE_CARD_SELECT_TRANSITION_MS`。 */
static void mode_carousel_on_mode_committed(ui_shoot_mode_t m, void *user_data)
{
    LV_UNUSED(user_data);
    ui_app_set_shoot_mode(m);
}

/** 按横条 **当前滚动位置**（视口中心最近子页）更新卡片高亮。 */
static void main_mode_update_card_selection(void)
{
    if(s_mode_strip == NULL || !lv_obj_is_valid(s_mode_strip)) {
        return;
    }
    lv_obj_update_layout(s_mode_strip);
    ui_mode_carousel_highlight_slot(s_mode_strip, ui_mode_carousel_nearest_index(s_mode_strip));
}

/**
 * 将横条滚动并使 **当前全局模式** 对应 **页** 居中进视口。
 * @param scroll_anim `true`：`scroll_to_view` 带动画；`false`：立即定位（打开模式页淡入结束、上滑提交等；进入模式页不播放横条滑入动画）。
 */
static void main_mode_strip_scroll_to_current(bool scroll_anim)
{
    if(s_mode_strip == NULL || !lv_obj_is_valid(s_mode_strip)) {
        return;
    }
    if(ui_mode_carousel_phys_count(s_mode_strip) == 0u) {
        return;
    }
    main_mode_shoot_mode_clamp_default();

    if(s_mode != NULL && lv_obj_is_valid(s_mode)) {
        lv_obj_update_layout(s_mode);
    }
    ui_mode_carousel_scroll_to_mode(s_mode_strip, ui_app_get_shoot_mode(), scroll_anim ? LV_ANIM_ON : LV_ANIM_OFF);
    main_refresh_status_bar();
}

/** 将语种下拉选中项与 `ui_i18n_get_lang()` 对齐（不触发回调）。 */
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

/** 语种下拉变更：更新 i18n、全表刷新与顶栏存储格式串。 */
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
    main_refresh_status_bar();
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
    if(s_cc_rotation_panel != NULL && lv_obj_is_valid(s_cc_rotation_panel)) {
        lv_obj_add_flag(s_cc_rotation_panel, LV_OBJ_FLAG_HIDDEN);
    }
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
    if(s_cc_rotation_panel != NULL && lv_obj_is_valid(s_cc_rotation_panel)) {
        lv_obj_add_flag(s_cc_rotation_panel, LV_OBJ_FLAG_HIDDEN);
    }
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
        ui_bt_nav_open_async();
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
            /* 行内首列为符号 Label，文案在索引 1（语言行为：图标、标题、下拉） */
            const uint32_t cnt = lv_obj_get_child_cnt(row);
            lv_obj_t *txt_lb = (cnt >= 2u) ? lv_obj_get_child(row, 1) : lv_obj_get_child(row, 0);
            if(txt_lb != NULL && lv_obj_check_type(txt_lb, &lv_label_class)) {
                const char *txt = lv_label_get_text(txt_lb);
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
    if(idx == 0u) {
        cc_show_rotation_view();
        printf("[CC] %s -> rotation detail\n", ui_i18n_str(s_cc_tile_str_ids[idx]));
        LOG_DEBUG("CC: rotation detail opened");
        return;
    }
    if(idx == 4u) {
        uint8_t b = ui_cc_settings_get_brightness();
        b = (uint8_t)((b + 10u) > 100u ? 10u : (b + 10u));
        ui_cc_settings_set_brightness(b);
        printf("[CC] %s -> %u%%\n", ui_i18n_str(s_cc_tile_str_ids[idx]), (unsigned)b);
        LOG_DEBUG("CC: brightness %u", (unsigned)b);
        return;
    }
    if(idx == 5u) {
        uint8_t v = ui_cc_settings_get_volume();
        v = (uint8_t)((v + 10u) > 100u ? 10u : (v + 10u));
        ui_cc_settings_set_volume(v);
        printf("[CC] %s -> %u%%\n", ui_i18n_str(s_cc_tile_str_ids[idx]), (unsigned)v);
        LOG_DEBUG("CC: volume %u", (unsigned)v);
        return;
    }
    /** 快切（宫格第 7 项，0-based 索引 6）：进入模式切换页 */
    if(idx == 6u) {
        main_show_control_center(false);
        main_show_mode_panel(true);
        printf("[CC] quick switch -> mode panel (current %s)\n", ui_i18n_str(ui_i18n_shoot_mode_label_id(ui_app_get_shoot_mode())));
        LOG_DEBUG("CC: quick switch -> mode panel");
        return;
    }
    printf("[CC] %s effect triggered\n", ui_i18n_str(s_cc_tile_str_ids[idx]));
    LOG_DEBUG("CC tile: %s triggered", ui_i18n_str(s_cc_tile_str_ids[idx]));
}

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

/** 控制中心 sheet 垂直位移；同步遮罩不透明度（露出越多蒙层越深，关闭动画时同步淡出）。 */
static void cc_sheet_y_anim_cb(void *var, int32_t v)
{
    lv_obj_t *sheet = (lv_obj_t *)var;
    lv_obj_set_y(sheet, (lv_coord_t)v);
    if(s_cc_dim != NULL && lv_obj_is_valid(s_cc_dim) && s_cc_sheet_h > 0) {
        int32_t opa = ((int32_t)v + (int32_t)s_cc_sheet_h) * (int32_t)LV_OPA_50 / (int32_t)s_cc_sheet_h;
        if(opa < 0) {
            opa = 0;
        }
        if(opa > LV_OPA_50) {
            opa = LV_OPA_50;
        }
        lv_obj_set_style_bg_opa(s_cc_dim, (lv_opa_t)opa, 0);
    }
}

/** 模式全屏层垂直位移 + 遮罩同步（与 `cc_sheet_y_anim_cb` 分离，便于 `lv_anim_delete` 按对象区分）。 */
static void mode_panel_y_anim_cb(void *var, int32_t v)
{
    lv_obj_t *mode = (lv_obj_t *)var;
    lv_obj_set_y(mode, (lv_coord_t)v);
    if(s_mode_dim != NULL && lv_obj_is_valid(s_mode_dim) && MY_SCREEN_HEIGHT > 0) {
        int32_t opa = ((int32_t)MY_SCREEN_HEIGHT - v) * (int32_t)LV_OPA_50 / (int32_t)MY_SCREEN_HEIGHT;
        if(opa < 0) {
            opa = 0;
        }
        if(opa > LV_OPA_50) {
            opa = LV_OPA_50;
        }
        lv_obj_set_style_bg_opa(s_mode_dim, (lv_opa_t)opa, 0);
    }
}

/** 按剩余行程比例缩放吸附动画时长（ease_out，见 `UI_PANEL_EDGE_ANIM_*`）。 */
static uint32_t main_panel_edge_anim_ms(lv_coord_t travel_abs, lv_coord_t reference_span)
{
    if(reference_span <= 0) {
        return (uint32_t)UI_PANEL_EDGE_ANIM_MS;
    }
    int64_t ms = (int64_t)UI_PANEL_EDGE_ANIM_MS * (int64_t)travel_abs / (int64_t)reference_span;
    if(ms < (int64_t)UI_PANEL_EDGE_ANIM_MS_MIN) {
        ms = UI_PANEL_EDGE_ANIM_MS_MIN;
    }
    if(ms > (int64_t)UI_PANEL_EDGE_ANIM_MS_MAX) {
        ms = UI_PANEL_EDGE_ANIM_MS_MAX;
    }
    return (uint32_t)ms;
}

static void cc_preview_close_finish(void)
{
    s_vp_cc_drag = false;
    if(s_cc_dim != NULL && lv_obj_is_valid(s_cc_dim)) {
        lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
        lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_cc_sheet != NULL && lv_obj_is_valid(s_cc_sheet)) {
        lv_obj_add_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(s_cc_sheet, -s_cc_sheet_h);
    }
}

static void cc_preview_close_anim_done(lv_anim_t *a)
{
    LV_UNUSED(a);
    cc_preview_close_finish();
}

static void cc_sheet_user_close_anim_done(lv_anim_t *a)
{
    LV_UNUSED(a);
    s_cc_open = false;
    s_vp_cc_drag = false;
    if(s_cc_dim != NULL && lv_obj_is_valid(s_cc_dim)) {
        lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
        lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_cc_sheet != NULL && lv_obj_is_valid(s_cc_sheet)) {
        lv_obj_add_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(s_cc_sheet, -s_cc_sheet_h);
    }
}

static void mode_preview_close_finish(void)
{
    s_vp_mode_preview = false;
    if(s_mode_dim != NULL && lv_obj_is_valid(s_mode_dim)) {
        lv_obj_add_flag(s_mode_dim, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_mode != NULL && lv_obj_is_valid(s_mode)) {
        lv_obj_add_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_translate_y(s_mode, 0, LV_PART_MAIN);
        lv_obj_set_y(s_mode, (lv_coord_t)MY_SCREEN_HEIGHT);
    }
}

static void mode_preview_close_anim_done(lv_anim_t *a)
{
    LV_UNUSED(a);
    mode_preview_close_finish();
}

static void mode_user_close_anim_done(lv_anim_t *a)
{
    LV_UNUSED(a);
    s_mode_open = false;
    s_vp_mode_preview = false;
    if(s_mode_dim != NULL && lv_obj_is_valid(s_mode_dim)) {
        lv_obj_add_flag(s_mode_dim, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_mode != NULL && lv_obj_is_valid(s_mode)) {
        lv_obj_add_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_translate_y(s_mode, 0, LV_PART_MAIN);
        lv_obj_set_y(s_mode, (lv_coord_t)MY_SCREEN_HEIGHT);
    }
}

static void main_cc_start_sheet_y_anim(lv_coord_t y_from, lv_coord_t y_to, lv_anim_completed_cb_t on_done)
{
    if(s_cc_sheet == NULL || !lv_obj_is_valid(s_cc_sheet) || s_cc_sheet_h <= 0) {
        if(on_done != NULL) {
            on_done(NULL);
        }
        return;
    }
    lv_anim_delete(s_cc_sheet, cc_sheet_y_anim_cb);
    const lv_coord_t travel = (lv_coord_t)LV_ABS((int32_t)y_to - (int32_t)y_from);
    const uint32_t ms = main_panel_edge_anim_ms(travel, s_cc_sheet_h);
    lv_anim_t an;
    lv_anim_init(&an);
    lv_anim_set_var(&an, s_cc_sheet);
    lv_anim_set_values(&an, (int32_t)y_from, (int32_t)y_to);
    lv_anim_set_exec_cb(&an, cc_sheet_y_anim_cb);
    lv_anim_set_duration(&an, ms);
    lv_anim_set_path_cb(&an, lv_anim_path_ease_out);
    if(on_done != NULL) {
        lv_anim_set_completed_cb(&an, on_done);
    }
    lv_anim_start(&an);
}

static void main_mode_start_panel_y_anim(lv_coord_t y_from, lv_coord_t y_to, lv_anim_completed_cb_t on_done)
{
    if(s_mode == NULL || !lv_obj_is_valid(s_mode)) {
        if(on_done != NULL) {
            on_done(NULL);
        }
        return;
    }
    lv_anim_delete(s_mode, mode_panel_y_anim_cb);
    const lv_coord_t travel = (lv_coord_t)LV_ABS((int32_t)y_to - (int32_t)y_from);
    const uint32_t ms = main_panel_edge_anim_ms(travel, (lv_coord_t)MY_SCREEN_HEIGHT);
    lv_anim_t an;
    lv_anim_init(&an);
    lv_anim_set_var(&an, s_mode);
    lv_anim_set_values(&an, (int32_t)y_from, (int32_t)y_to);
    lv_anim_set_exec_cb(&an, mode_panel_y_anim_cb);
    lv_anim_set_duration(&an, ms);
    lv_anim_set_path_cb(&an, lv_anim_path_ease_out);
    if(on_done != NULL) {
        lv_anim_set_completed_cb(&an, on_done);
    }
    lv_anim_start(&an);
}

static void main_cc_animate_preview_close(void)
{
    if(s_cc_sheet == NULL || !lv_obj_is_valid(s_cc_sheet) || s_cc_sheet_h <= 0) {
        cc_preview_close_finish();
        return;
    }
    const lv_coord_t y0 = lv_obj_get_y(s_cc_sheet);
    const lv_coord_t y_end = (lv_coord_t)(-s_cc_sheet_h);
    if(y0 <= y_end + 2) {
        cc_preview_close_finish();
        return;
    }
    main_cc_start_sheet_y_anim(y0, y_end, cc_preview_close_anim_done);
}

static void main_mode_animate_preview_close(void)
{
    if(s_mode == NULL || !lv_obj_is_valid(s_mode)) {
        mode_preview_close_finish();
        return;
    }
    const lv_coord_t y0 = lv_obj_get_y(s_mode);
    const lv_coord_t y_end = (lv_coord_t)MY_SCREEN_HEIGHT;
    if(y0 >= y_end - 2) {
        mode_preview_close_finish();
        return;
    }
    main_mode_start_panel_y_anim(y0, y_end, mode_preview_close_anim_done);
}

/** 点击半透明遮罩：关闭控制中心 */
static void cc_dim_clicked_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    main_show_control_center(false);
}

/**
 * @brief 打开/关闭全屏控制中心（遮罩 + sheet）。
 * @param show true 打开并复位到宫格视图；false 滑出收起（预览态亦动画收回）。
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
        s_vp_cc_drag = false;
        cc_show_grid_view();
        lv_obj_clear_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_cc_dim);
        lv_obj_move_foreground(s_cc_sheet);
        lv_obj_set_y(s_cc_sheet, 0);
        lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
        cc_lang_dd_sync_from_i18n();
    }
    else {
        if(!s_cc_open && !s_vp_cc_drag) {
            return;
        }
        if(s_cc_lang_dd != NULL && lv_obj_is_valid(s_cc_lang_dd)) {
            lv_dropdown_close(s_cc_lang_dd);
        }
        cc_show_grid_view();

        if(s_vp_cc_drag && !s_cc_open) {
            main_cc_animate_preview_close();
            return;
        }

        if(!s_cc_open) {
            return;
        }

        const lv_coord_t y0 = lv_obj_get_y(s_cc_sheet);
        const lv_coord_t y_end = (lv_coord_t)(-s_cc_sheet_h);
        if(y0 <= y_end + 2) {
            s_cc_open = false;
            s_vp_cc_drag = false;
            if(s_cc_dim != NULL && lv_obj_is_valid(s_cc_dim)) {
                lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
                lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_add_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_y(s_cc_sheet, y_end);
            return;
        }
        main_cc_start_sheet_y_anim(y0, y_end, cc_sheet_user_close_anim_done);
    }
}

/** 控制中心上滑关闭：按下起点 */
static lv_point_t s_cc_swipe_press;
/** 控制中心上滑：是否跟踪手势 */
static bool s_cc_swipe_track;

/**
 * @brief 控制中心内上滑收起（与主屏「下拉打开」方向相反）。
 *
 * 判定：-dy >= UI_SWIPE_COMMIT_DY，|dx| <= UI_SWIPE_MAX_ABS_DX，且 |dy| > |dx|（纵向占优）。
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
        if(LV_ABS(sy - s_cc_setlist_scroll_y_at_press) > UI_SWIPE_LIST_SCROLL_EPS) {
            return;
        }
    }
    if(-dy >= UI_SWIPE_COMMIT_DY && LV_ABS(dx) <= UI_SWIPE_MAX_ABS_DX && LV_ABS(dy) > LV_ABS(dx)) {
        LOG_DEBUG("控制中心上滑关闭");
        main_show_control_center(false);
    }
}

/**
 * @brief 模式参数页：下滑关闭（与「上滑打开」相反）。
 *
 * 判定：dy >= UI_SWIPE_COMMIT_DY，|dx| <= UI_SWIPE_MAX_ABS_DX，且 dy > |dx|（纵向占优）。
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

/** 点击模式层遮罩：关闭（与控制中心遮罩一致） */
static void mode_dim_clicked_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    main_show_mode_panel(false);
}

/** 显示/隐藏模式参数全屏层（遮罩 + 白底层；横条在完全展开后对齐当前模式） */
static void main_show_mode_panel(bool show)
{
    if(s_mode == NULL || !lv_obj_is_valid(s_mode)) {
        return;
    }

    if(show) {
        lv_anim_delete(s_mode, mode_panel_y_anim_cb);
        if(s_mode_open) {
            return;
        }
        s_mode_open = true;
        s_vp_mode_preview = false;
        if(s_mode_dim != NULL && lv_obj_is_valid(s_mode_dim)) {
            lv_obj_clear_flag(s_mode_dim, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_opa(s_mode_dim, LV_OPA_50, 0);
            lv_obj_move_foreground(s_mode_dim);
        }
        lv_obj_clear_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_translate_y(s_mode, 0, LV_PART_MAIN);
        lv_obj_set_y(s_mode, 0);
        lv_obj_move_foreground(s_mode);
        lv_obj_update_layout(s_mode);
        main_mode_strip_scroll_to_current(false);
    }
    else {
        /* 含上滑未提交预览（`s_vp_mode_preview`）：点遮罩关闭须收起层，不能仅 `s_mode_open` 判断 */
        if(!s_mode_open && !s_vp_mode_preview) {
            return;
        }
        lv_anim_delete(s_mode, mode_panel_y_anim_cb);

        if(s_vp_mode_preview && !s_mode_open) {
            main_mode_animate_preview_close();
            return;
        }

        if(!s_mode_open) {
            return;
        }

        const lv_coord_t y0 = lv_obj_get_y(s_mode);
        const lv_coord_t y_end = (lv_coord_t)MY_SCREEN_HEIGHT;
        if(y0 >= y_end - 2) {
            mode_user_close_anim_done(NULL);
            return;
        }
        main_mode_start_panel_y_anim(y0, y_end, mode_user_close_anim_done);
    }
}

/**
 * 以视口中心物理槽为基准，横向步进 `delta` 个槽（±1 / ±2）；与左右宽触区分工一致。
 */
static void mode_strip_apply_neighbor_delta(int32_t delta)
{
    if(s_mode_strip == NULL || !lv_obj_is_valid(s_mode_strip)) {
        return;
    }
    const uint32_t phys = ui_mode_carousel_phys_count(s_mode_strip);
    if(phys == 0u) {
        return;
    }
    lv_obj_update_layout(s_mode_strip);
    const uint32_t c = ui_mode_carousel_nearest_index(s_mode_strip);
    int64_t t = (int64_t)c + (int64_t)delta;
    if(t < 0) {
        t = 0;
    }
    if(t >= (int64_t)phys) {
        t = (int64_t)phys - 1;
    }
    lv_obj_t *page = ui_mode_carousel_page(s_mode_strip, (uint32_t)t);
    if(page != NULL && lv_obj_is_valid(page)) {
        lv_obj_scroll_to_view(page, LV_ANIM_ON);
    }
}

/**
 * 横条在 **左右各 `s_mode_tap_side_miss_w`**（**`2×(UI_MODE_STRIP_PAGE_W+GAP)`**）内不吸收命中，触摸落到下层透明触区；仅中间带可拖滑。
 */
static void mode_strip_hit_test_center_only_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_HIT_TEST) {
        return;
    }
    lv_hit_test_info_t *hti = lv_event_get_hit_test_info(e);
    if(hti == NULL || !hti->res) {
        return;
    }
    const lv_point_t *pt = hti->point;
    lv_obj_t *strip = lv_event_get_target(e);
    lv_obj_t *panel = lv_obj_get_parent(strip);
    if(panel == NULL || !lv_obj_is_valid(panel)) {
        return;
    }
    lv_area_t scr;
    lv_obj_get_coords(panel, &scr);
    const int32_t rx = pt->x - scr.x1;
    const int32_t w = lv_area_get_width(&scr);
    if(w <= 0) {
        return;
    }
    const int32_t side = (int32_t)s_mode_tap_side_miss_w;
    if(side <= 0) {
        return;
    }
    const int32_t r0 = w - side;
    if(rx < side || rx >= r0) {
        hti->res = false;
    }
}

/** 左触区内：与横条 **单列宽** 对齐 — **外列** −2、**内列** −1（整屏高，顶栏 `move_foreground` 压在左区上）。 */
static void mode_tap_left_strip_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    lv_obj_t *z = lv_event_get_target(e);
    lv_indev_t *indev = lv_indev_active();
    if(indev == NULL) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t a;
    lv_obj_get_coords(z, &a);
    const int32_t lx = p.x - a.x1;
    const lv_coord_t zw = lv_obj_get_width(z);
    const int32_t half = (int32_t)zw / 2;
    const int32_t delta = (lx < half) ? -2 : -1;
    mode_strip_apply_neighbor_delta(delta);
}

/** 右触区内：与横条单列对齐 — **内列** +1、**外列** +2（标题栏以下，避免挡关闭钮）。 */
static void mode_tap_right_strip_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    lv_obj_t *z = lv_event_get_target(e);
    lv_indev_t *indev = lv_indev_active();
    if(indev == NULL) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t a;
    lv_obj_get_coords(z, &a);
    const int32_t lx = p.x - a.x1;
    const lv_coord_t zw = lv_obj_get_width(z);
    const int32_t half = (int32_t)zw / 2;
    const int32_t delta = (lx < half) ? 1 : 2;
    mode_strip_apply_neighbor_delta(delta);
}

/**
 * 点击 **模式图标**：若该页当前不在横条视口中央，则 **仅滚动** 到该页居中，由 `SCROLL_END` 再同步模式与选中态；
 * 若已在中央，则 **确认** 当前模式、刷新顶栏并 **关闭模式页**。
 */
static void mode_icon_clicked_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    const uintptr_t slot = (uintptr_t)lv_event_get_user_data(e);
    if(s_mode_strip == NULL || !lv_obj_is_valid(s_mode_strip)) {
        return;
    }
    if(slot >= ui_mode_carousel_phys_count(s_mode_strip)) {
        return;
    }
    lv_obj_t *page = ui_mode_carousel_page(s_mode_strip, (uint32_t)slot);
    if(page == NULL || !lv_obj_is_valid(page)) {
        return;
    }
    lv_obj_update_layout(s_mode_strip);
    const uint32_t centered = ui_mode_carousel_nearest_index(s_mode_strip);
    if((uint32_t)slot != centered) {
        lv_obj_scroll_to_view(page, LV_ANIM_ON);
        return;
    }
    ui_app_set_shoot_mode(ui_mode_carousel_mode_at(s_mode_strip, (uint32_t)slot));
    main_mode_update_card_selection();
    main_show_mode_panel(false);
}

/** 预览区下拉跟手：按千分比移动控制中心 sheet 与遮罩透明度（未提交打开）。 */
static void main_cc_apply_drag_permille(int32_t permille)
{
    if(s_cc_dim == NULL || s_cc_sheet == NULL || s_cc_sheet_h <= 0 || s_cc_open) {
        return;
    }
    if(permille <= 0) {
        s_vp_cc_drag = false;
        lv_anim_delete(s_cc_sheet, cc_sheet_y_anim_cb);
        lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
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
    {
        const int32_t opa = (permille * (int32_t)LV_OPA_50) / 1000;
        lv_obj_set_style_bg_opa(s_cc_dim, (lv_opa_t)LV_CLAMP(0, opa, LV_OPA_50), 0);
    }
}

static void cc_sheet_drag_commit_anim_done(lv_anim_t *a)
{
    LV_UNUSED(a);
    cc_lang_dd_sync_from_i18n();
}

/** 下拉手势提交：ease_out 吸附完全展开控制中心并进入宫格视图。 */
static void main_cc_commit_from_drag(void)
{
    if(s_cc_dim == NULL || s_cc_sheet == NULL || s_cc_sheet_h <= 0) {
        return;
    }
    s_vp_cc_drag = false;
    s_cc_open = true;
    cc_show_grid_view();
    lv_obj_clear_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_cc_dim);
    lv_obj_move_foreground(s_cc_sheet);

    const lv_coord_t y0 = lv_obj_get_y(s_cc_sheet);
    if(y0 >= -1) {
        lv_obj_set_y(s_cc_sheet, 0);
        lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
        cc_lang_dd_sync_from_i18n();
        return;
    }
    main_cc_start_sheet_y_anim(y0, 0, cc_sheet_drag_commit_anim_done);
}

/** 预览区上滑跟手：与下拉控制中心一致——**遮罩 + 层 `y`**，主界面不位移（不用 `translate_y`）。 */
static void main_mode_apply_drag_permille(int32_t permille)
{
    if(s_mode == NULL || s_mode_open) {
        return;
    }
    if(permille <= 0) {
        s_vp_mode_preview = false;
        lv_anim_delete(s_mode, mode_panel_y_anim_cb);
        if(s_mode_dim != NULL && lv_obj_is_valid(s_mode_dim)) {
            lv_obj_set_style_bg_opa(s_mode_dim, LV_OPA_50, 0);
            lv_obj_add_flag(s_mode_dim, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_add_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_translate_y(s_mode, 0, LV_PART_MAIN);
        lv_obj_set_y(s_mode, (lv_coord_t)MY_SCREEN_HEIGHT);
        return;
    }
    const bool entering_mode_preview = !s_vp_mode_preview;
    s_vp_mode_preview = true;
    lv_anim_delete(s_mode, mode_panel_y_anim_cb);
    if(s_mode_dim != NULL && lv_obj_is_valid(s_mode_dim)) {
        lv_obj_clear_flag(s_mode_dim, LV_OBJ_FLAG_HIDDEN);
        {
            const int32_t p = LV_MIN(permille, 1000);
            const int32_t opa = (p * (int32_t)LV_OPA_50) / 1000;
            lv_obj_set_style_bg_opa(s_mode_dim, (lv_opa_t)LV_CLAMP(0, opa, LV_OPA_50), 0);
        }
        lv_obj_move_foreground(s_mode_dim);
    }
    lv_obj_clear_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_translate_y(s_mode, 0, LV_PART_MAIN);
    {
        const int32_t p = LV_MIN(permille, 1000);
        const lv_coord_t y = (lv_coord_t)(((int32_t)MY_SCREEN_HEIGHT * (1000 - p)) / 1000);
        lv_obj_set_y(s_mode, y);
    }
    lv_obj_move_foreground(s_mode);
    if(entering_mode_preview) {
        lv_obj_update_layout(s_mode);
        main_mode_strip_scroll_to_current(false);
    }
}

/**
 * 上滑提交后 **Y 吸附动画结束**：仅刷新布局与横条高亮。
 * 横条 **`scroll_to_mode`** 须在动画 **开始前** 调用（见 `main_mode_commit_from_drag`），否则跟手阶段横条仍为旧位置，松手后整段 Y 动画播完再瞬间 `scroll_to_view` 会与控制中心入口的「先对齐再完整出现」不一致，产生 **跳变**。
 */
static void mode_drag_commit_anim_done(lv_anim_t *a)
{
    LV_UNUSED(a);
    if(s_mode != NULL && lv_obj_is_valid(s_mode)) {
        lv_obj_update_layout(s_mode);
        main_mode_update_card_selection();
    }
}

/** 上滑手势提交：ease_out 吸附完全显示模式页并同步横条滚动与选中态。 */
static void main_mode_commit_from_drag(void)
{
    if(s_mode == NULL || !lv_obj_is_valid(s_mode)) {
        return;
    }
    s_vp_mode_preview = false;
    s_mode_open = true;
    if(s_mode_dim != NULL && lv_obj_is_valid(s_mode_dim)) {
        lv_obj_clear_flag(s_mode_dim, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_mode_dim);
    }
    lv_obj_clear_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_translate_y(s_mode, 0, LV_PART_MAIN);
    lv_obj_move_foreground(s_mode);

    lv_obj_update_layout(s_mode);
    /* 与 `main_show_mode_panel(true)` 一致：先对齐当前模式槽位，再（必要时）播 Y 吸附；避免横条在 Y 动画末尾才瞬间跳转 */
    main_mode_strip_scroll_to_current(false);

    const lv_coord_t y0 = lv_obj_get_y(s_mode);
    if(y0 <= 1) {
        lv_obj_set_y(s_mode, 0);
        if(s_mode_dim != NULL && lv_obj_is_valid(s_mode_dim)) {
            lv_obj_set_style_bg_opa(s_mode_dim, LV_OPA_50, 0);
        }
        lv_obj_update_layout(s_mode);
        return;
    }
    main_mode_start_panel_y_anim(y0, 0, mode_drag_commit_anim_done);
}

/** 预览区左滑跟手：按千分比设置 ISP 侧栏宽度（未提交展开）。 */
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

/**
 * 右滑进入回放：与上/下滑一致 **不做右缘半透明跟手条**（旧实现会在画面右侧叠灰条，且与 ISP 列同区易透出 ISP 文案）。
 * 仅清空其它方向的跟手预览；ISP 已展开时保持展开直至松手提交切页。
 */
static void main_vp_replay_drag_side_effects(void)
{
    main_cc_apply_drag_permille(0);
    main_mode_apply_drag_permille(0);
    main_isp_apply_drag_permille(0);
}

/** 根据锁定方向与位移计算跟手千分比（相对 `UI_SWIPE_COMMIT_*`）。 */
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

/** 判断当前轴向位移是否超出副轴允许阈值（防斜滑误判）。 */
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

/** 控制中心竖直轴是否仍占用（已展开、跟手预览、sheet Y 动画中）。与上滑模式页互斥。 */
static bool main_vp_cc_vertical_busy(void)
{
    if(s_cc_open) {
        return true;
    }
    if(s_vp_cc_drag) {
        return true;
    }
    if(s_cc_sheet != NULL && lv_obj_is_valid(s_cc_sheet) && lv_anim_get(s_cc_sheet, cc_sheet_y_anim_cb) != NULL) {
        return true;
    }
    return false;
}

/** 模式页竖直轴是否仍占用（已展开、跟手预览、层 Y 动画中）。与下拉控制中心互斥。 */
static bool main_vp_mode_vertical_busy(void)
{
    if(s_mode_open) {
        return true;
    }
    if(s_vp_mode_preview) {
        return true;
    }
    if(s_mode != NULL && lv_obj_is_valid(s_mode) && lv_anim_get(s_mode, mode_panel_y_anim_cb) != NULL) {
        return true;
    }
    return false;
}

/** 松手时是否满足该方向的提交距离与轴向占优条件。 */
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

/** 取消预览跟手：复位 ISP；控制中心/模式预览按 instant 决定瞬时收回或 ease_out 收回。 */
static void main_vp_cancel_drag(bool instant)
{
    s_vp_dir = MAIN_VP_DIR_NONE;

    main_isp_apply_drag_permille(0);

    if(s_vp_cc_drag && !s_cc_open && s_cc_sheet != NULL && lv_obj_is_valid(s_cc_sheet) && s_cc_dim != NULL &&
       lv_obj_is_valid(s_cc_dim) && s_cc_sheet_h > 0) {
        if(instant) {
            lv_anim_delete(s_cc_sheet, cc_sheet_y_anim_cb);
            s_vp_cc_drag = false;
            lv_obj_set_style_bg_opa(s_cc_dim, LV_OPA_50, 0);
            lv_obj_add_flag(s_cc_dim, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_cc_sheet, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_y(s_cc_sheet, -s_cc_sheet_h);
        }
        else {
            main_cc_animate_preview_close();
        }
    }
    else {
        s_vp_cc_drag = false;
    }

    if(s_vp_mode_preview && !s_mode_open && s_mode != NULL && lv_obj_is_valid(s_mode)) {
        if(instant) {
            lv_anim_delete(s_mode, mode_panel_y_anim_cb);
            s_vp_mode_preview = false;
            if(s_mode_dim != NULL && lv_obj_is_valid(s_mode_dim)) {
                lv_obj_set_style_bg_opa(s_mode_dim, LV_OPA_50, 0);
                lv_obj_add_flag(s_mode_dim, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_add_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(s_mode, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_translate_y(s_mode, 0, LV_PART_MAIN);
            lv_obj_set_y(s_mode, (lv_coord_t)MY_SCREEN_HEIGHT);
        }
        else {
            main_mode_animate_preview_close();
        }
    }
    else if(instant) {
        lv_anim_delete(s_mode, mode_panel_y_anim_cb);
        main_mode_apply_drag_permille(0);
    }
}

/**
 * @brief 主界面全屏四向滑动手势（绑定在 `scr` + 子控件 EVENT_BUBBLE）。
 *
 * PRESSED 记点；PRESSING 锁定主向后按位移给 0～1000‰ 跟手预览（右滑进回放 **无** 右缘条，与上/下滑一致）；滑回锁阈内可解锁改向；副轴超阈仅清空预览、不丢跟踪。
 * **竖直闩锁**：本指一旦曾锁 **下**（或 **上**），在 **松手前** 不可再锁相反竖直向（避免下拉拉满再回退时误触上滑模式）；**松手 / PRESS_LOST / 新 PRESSED** 时闩锁清零，下一次按下可重新选向。
 * **竖直互斥**：模式竖直轴未结束（预览/动画/已展开）时不锁定、不提交 **下拉**；控制中心竖直轴未结束时不锁定、不提交 **上滑**。
 * RELEASED：左右滑仍按 **松手位移** 与 `UI_SWIPE_COMMIT_*` 提交；上/下滑按 **露出千分比**
 * `UI_PANEL_SNAP_OPEN_PERMILLE`（约半屏）吸附展开，否则 ease_out 收回。
 * 控制中心/模式层打开时不处理，避免与覆盖层冲突。
 */
static void main_viewport_gesture_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_user_data(e);
    (void)scr;
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    if(indev == NULL) {
        return;
    }

    if(s_cc_open || s_mode_open) {
        if(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESS_LOST) {
            s_vp_tracking = false;
            s_vp_dir = MAIN_VP_DIR_NONE;
            s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
        }
        return;
    }

    if(code == LV_EVENT_PRESSED) {
        main_vp_cancel_drag(true);
        lv_indev_get_point(indev, &s_vp_press);
        s_vp_tracking = true;
        s_vp_dir = MAIN_VP_DIR_NONE;
        s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
        return;
    }

    if(code == LV_EVENT_PRESS_LOST) {
        s_vp_tracking = false;
        main_vp_cancel_drag(false);
        s_vp_dir = MAIN_VP_DIR_NONE;
        s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
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
                const main_vp_dir_t vdir = (dy > 0) ? MAIN_VP_DIR_DOWN : MAIN_VP_DIR_UP;
                if(vdir == MAIN_VP_DIR_DOWN && main_vp_mode_vertical_busy()) {
                    return;
                }
                if(vdir == MAIN_VP_DIR_UP && main_vp_cc_vertical_busy()) {
                    return;
                }
                if(vdir == MAIN_VP_DIR_DOWN && s_vp_vert_latch == MAIN_VP_VERT_LATCH_UP) {
                    return;
                }
                if(vdir == MAIN_VP_DIR_UP && s_vp_vert_latch == MAIN_VP_VERT_LATCH_DOWN) {
                    return;
                }
                s_vp_dir = vdir;
                if(vdir == MAIN_VP_DIR_DOWN) {
                    s_vp_vert_latch = MAIN_VP_VERT_LATCH_DOWN;
                }
                else {
                    s_vp_vert_latch = MAIN_VP_VERT_LATCH_UP;
                }
            }
        }
        else if(adx < UI_SWIPE_DIR_LOCK_PX && ady < UI_SWIPE_DIR_LOCK_PX) {
            /* 滑回按压点附近：解锁主向，可不抬手改方向；跟手预览清零 */
            s_vp_dir = MAIN_VP_DIR_NONE;
            main_cc_apply_drag_permille(0);
            main_mode_apply_drag_permille(0);
            main_isp_apply_drag_permille(0);
            return;
        }

        /* 副轴超阈：仅清空跟手预览，不结束跟踪；回退到合规位移后可继续跟手，松手仍以终点判定 */
        if(main_vp_cross_axis_bad(s_vp_dir, adx, ady)) {
            main_cc_apply_drag_permille(0);
            main_mode_apply_drag_permille(0);
            main_isp_apply_drag_permille(0);
            return;
        }

        int32_t pm = main_vp_permille_for_dir(s_vp_dir, dx, dy);
        if(pm > 1000) {
            pm = 1000;
        }

        switch(s_vp_dir) {
            case MAIN_VP_DIR_RIGHT:
                main_vp_replay_drag_side_effects();
                break;
            case MAIN_VP_DIR_LEFT:
                main_cc_apply_drag_permille(0);
                main_mode_apply_drag_permille(0);
                if(!s_isp_open) {
                    main_isp_apply_drag_permille(pm);
                }
                break;
            case MAIN_VP_DIR_DOWN:
                main_mode_apply_drag_permille(0);
                main_isp_apply_drag_permille(0);
                main_cc_apply_drag_permille(pm);
                break;
            case MAIN_VP_DIR_UP:
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
            s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
            return;
        }
        if(adxr >= adyr) {
            s_vp_dir = (dx > 0) ? MAIN_VP_DIR_RIGHT : MAIN_VP_DIR_LEFT;
        }
        else {
            const main_vp_dir_t vdir = (dy > 0) ? MAIN_VP_DIR_DOWN : MAIN_VP_DIR_UP;
            if(vdir == MAIN_VP_DIR_DOWN && main_vp_mode_vertical_busy()) {
                main_vp_cancel_drag(false);
                s_vp_dir = MAIN_VP_DIR_NONE;
                s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
                return;
            }
            if(vdir == MAIN_VP_DIR_UP && main_vp_cc_vertical_busy()) {
                main_vp_cancel_drag(false);
                s_vp_dir = MAIN_VP_DIR_NONE;
                s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
                return;
            }
            if(vdir == MAIN_VP_DIR_DOWN && s_vp_vert_latch == MAIN_VP_VERT_LATCH_UP) {
                main_vp_cancel_drag(false);
                s_vp_dir = MAIN_VP_DIR_NONE;
                s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
                return;
            }
            if(vdir == MAIN_VP_DIR_UP && s_vp_vert_latch == MAIN_VP_VERT_LATCH_DOWN) {
                main_vp_cancel_drag(false);
                s_vp_dir = MAIN_VP_DIR_NONE;
                s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
                return;
            }
            s_vp_dir = vdir;
            if(vdir == MAIN_VP_DIR_DOWN) {
                s_vp_vert_latch = MAIN_VP_VERT_LATCH_DOWN;
            }
            else {
                s_vp_vert_latch = MAIN_VP_VERT_LATCH_UP;
            }
        }
    }

    if(main_vp_cross_axis_bad(s_vp_dir, adxr, adyr)) {
        main_vp_cancel_drag(false);
        s_vp_dir = MAIN_VP_DIR_NONE;
        s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
        return;
    }

    bool commit = false;
    switch(s_vp_dir) {
        case MAIN_VP_DIR_RIGHT:
        case MAIN_VP_DIR_LEFT:
            commit = main_vp_release_commit_ok(s_vp_dir, dx, dy);
            break;
        case MAIN_VP_DIR_DOWN:
        case MAIN_VP_DIR_UP: {
            int32_t pm = main_vp_permille_for_dir(s_vp_dir, dx, dy);
            if(pm > 1000) {
                pm = 1000;
            }
            const int adx = LV_ABS(dx);
            commit = (pm >= (int32_t)UI_PANEL_SNAP_OPEN_PERMILLE);
            if(s_vp_dir == MAIN_VP_DIR_DOWN) {
                commit = commit && (adx <= UI_SWIPE_MAX_ABS_DX) && (dy > adx);
            }
            else {
                commit = commit && (adx <= UI_SWIPE_MAX_ABS_DX) && ((-dy) > adx);
            }
            break;
        }
        default:
            break;
    }

    if(s_vp_dir == MAIN_VP_DIR_DOWN && main_vp_mode_vertical_busy()) {
        main_vp_cancel_drag(false);
        s_vp_dir = MAIN_VP_DIR_NONE;
        s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
        return;
    }
    if(s_vp_dir == MAIN_VP_DIR_UP && main_vp_cc_vertical_busy()) {
        main_vp_cancel_drag(false);
        s_vp_dir = MAIN_VP_DIR_NONE;
        s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
        return;
    }

    if(commit) {
        switch(s_vp_dir) {
            case MAIN_VP_DIR_RIGHT:
                LOG_DEBUG("用户右滑");
                main_cc_apply_drag_permille(0);
                main_mode_apply_drag_permille(0);
                main_isp_apply_drag_permille(0);
                s_vp_cc_drag = false;
                lv_async_call(ui_nav_replace_with_replay_async, NULL);
                break;
            case MAIN_VP_DIR_LEFT:
                LOG_DEBUG("左滑 ISP");
                main_cc_apply_drag_permille(0);
                main_mode_apply_drag_permille(0);
                s_vp_isp_preview = false;
                main_set_isp_open(!s_isp_open);
                break;
            case MAIN_VP_DIR_DOWN:
                LOG_DEBUG("下拉控制中心");
                main_mode_apply_drag_permille(0);
                main_isp_apply_drag_permille(0);
                if(!s_cc_open) {
                    main_cc_commit_from_drag();
                }
                break;
            case MAIN_VP_DIR_UP:
                LOG_DEBUG("上滑模式参数");
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
    s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
}

void ui_page_main_scr_detach_gestures(lv_obj_t *scr)
{
    if(scr == NULL) {
        return;
    }
    lv_obj_remove_event_cb(scr, main_viewport_gesture_cb);
}

/** 按 `ui_cc_settings_get_rotation_index()` 更新四个角度按钮描边焦点。 */
static void cc_rot_refresh_angle_focus(void)
{
    const uint8_t sel = ui_cc_settings_get_rotation_index();
    for(unsigned i = 0; i < 4; i++) {
        lv_obj_t *b = s_cc_rot_angle_btns[i];
        if(b == NULL || !lv_obj_is_valid(b)) {
            continue;
        }
        if(i == sel) {
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

static void cc_rot_angle_clicked_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    const unsigned i = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(i >= 4) {
        return;
    }
    ui_cc_settings_set_rotation(ui_cc_settings_get_rotation_enabled(), (uint8_t)i);
    cc_rot_refresh_angle_focus();
}

static void cc_rot_switch_changed_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    lv_obj_t *sw = lv_event_get_target(e);
    const bool en = lv_obj_has_state(sw, LV_STATE_CHECKED);
    ui_cc_settings_set_rotation(en, ui_cc_settings_get_rotation_index());
    if(s_cc_rot_angle_list != NULL && lv_obj_is_valid(s_cc_rot_angle_list)) {
        if(en) {
            lv_obj_remove_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
        }
    }
    cc_rot_refresh_angle_focus();
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
    if(s_cc_grid != NULL && lv_obj_is_valid(s_cc_grid)) {
        lv_obj_add_flag(s_cc_grid, LV_OBJ_FLAG_HIDDEN);
    }
    if(s_cc_settings != NULL && lv_obj_is_valid(s_cc_settings)) {
        lv_obj_add_flag(s_cc_settings, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_remove_flag(s_cc_rotation_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_cc_rotation_panel);
    if(s_cc_rot_switch != NULL && lv_obj_is_valid(s_cc_rot_switch)) {
        if(ui_cc_settings_get_rotation_enabled()) {
            lv_obj_add_state(s_cc_rot_switch, LV_STATE_CHECKED);
        }
        else {
            lv_obj_remove_state(s_cc_rot_switch, LV_STATE_CHECKED);
        }
    }
    if(s_cc_rot_angle_list != NULL && lv_obj_is_valid(s_cc_rot_angle_list)) {
        if(ui_cc_settings_get_rotation_enabled()) {
            lv_obj_remove_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
        }
    }
    cc_rot_refresh_angle_focus();
}

/** 在控制中心 body 内创建旋转详情（默认隐藏，与系统设置子页同级）。 */
static void main_cc_create_rotation_panel(lv_obj_t *cc_body)
{
    s_cc_rotation_panel = lv_obj_create(cc_body);
    lv_obj_set_size(s_cc_rotation_panel, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_cc_rotation_panel, lv_color_hex(UI_THEME_CC_PAGE_BG), 0);
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
    lv_obj_set_style_bg_color(rb, lv_color_hex(UI_THEME_CC_BACK_BTN_BG), 0);
    lv_obj_set_style_border_width(rb, 0, 0);
    lv_obj_remove_flag(rb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(rb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(rb, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(rb, cc_rotation_back_cb, LV_EVENT_CLICKED, NULL);
    ui_style_cc_interactive_focus(rb);
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
    ui_style_cc_interactive_focus(s_cc_rot_switch);

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
        lv_obj_set_style_bg_color(b, lv_color_hex(UI_CC_SETTINGS_ROW_BG), 0);
        lv_obj_set_style_bg_opa(b, UI_CC_SETTINGS_ROW_BG_OPA, 0);
        lv_obj_set_style_border_color(b, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
        lv_obj_set_style_border_width(b, UI_CC_SETTINGS_ROW_BORDER_W, 0);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);
        ui_style_cc_interactive_focus(b);
        lv_obj_add_event_cb(b, cc_rot_angle_clicked_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *bl = lv_label_create(b);
        ui_i18n_bind_label(bl, angle_ids[i]);
        lv_label_set_long_mode(bl, LV_LABEL_LONG_CLIP);
        ui_style_zone_label(bl);
        lv_obj_center(bl);
    }
    if(ui_cc_settings_get_rotation_enabled()) {
        lv_obj_add_state(s_cc_rot_switch, LV_STATE_CHECKED);
        lv_obj_remove_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_remove_state(s_cc_rot_switch, LV_STATE_CHECKED);
        lv_obj_add_flag(s_cc_rot_angle_list, LV_OBJ_FLAG_HIDDEN);
    }
    cc_rot_refresh_angle_focus();
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
    lv_obj_set_style_bg_color(s_cc_sheet, lv_color_hex(UI_THEME_CC_SHEET_BG), 0);
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
        lv_obj_set_style_bg_color(cell, lv_color_hex(UI_CC_SETTINGS_ROW_BG), 0);
        lv_obj_set_style_bg_opa(cell, UI_CC_SETTINGS_ROW_BG_OPA, 0);
        lv_obj_set_style_border_color(cell, lv_color_hex(UI_CC_SETTINGS_ROW_BORDER), 0);
        lv_obj_set_style_border_width(cell, UI_CC_SETTINGS_ROW_BORDER_W, 0);
        lv_obj_set_style_radius(cell, 10, 0);
        lv_obj_set_layout(cell, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        ui_style_cc_interactive_focus(cell);
        lv_obj_add_event_cb(cell, cc_tile_clicked_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_add_flag(cell, LV_OBJ_FLAG_EVENT_BUBBLE);

        lv_obj_t *lb = lv_label_create(cell);
        if(i == 6u) {
            s_cc_quick_tile_l = lb;
            lv_label_set_long_mode(lb, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(lb, LV_PCT(92));
            lv_obj_set_style_text_align(lb, LV_TEXT_ALIGN_CENTER, 0);
            main_refresh_cc_quick_tile_label();
        }
        else {
            ui_i18n_bind_label(lb, s_cc_tile_str_ids[i]);
            lv_label_set_long_mode(lb, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(lb, LV_PCT(92));
            lv_obj_set_style_text_align(lb, LV_TEXT_ALIGN_CENTER, 0);
            ui_style_zone_label(lb);
        }
    }

    main_cc_create_rotation_panel(cc_body);

    s_cc_settings = lv_obj_create(cc_body);
    lv_obj_set_size(s_cc_settings, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_cc_settings, lv_color_hex(UI_THEME_CC_PAGE_BG), 0);
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
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(UI_THEME_CC_BACK_BTN_BG), 0);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_remove_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(back_btn, cc_settings_back_cb, LV_EVENT_CLICKED, NULL);
    ui_style_cc_interactive_focus(back_btn);
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
    lv_obj_remove_flag(set_list, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
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
    ui_style_cc_settings_row_apply(lang_row);
    lv_obj_remove_flag(lang_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(lang_row, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_layout(lang_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lang_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lang_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(lang_row, 8, 0);

    lv_obj_t *lang_ic = lv_label_create(lang_row);
    lv_label_set_text_static(lang_ic, LV_SYMBOL_KEYBOARD);
    lv_label_set_long_mode(lang_ic, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(lang_ic, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lang_ic, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_align(lang_ic, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_remove_flag(lang_ic, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(lang_ic, 26);

    lv_obj_t *lang_cap = lv_label_create(lang_row);
    ui_i18n_bind_label(lang_cap, UI_STR_CC_LANG_LABEL);
    lv_label_set_long_mode(lang_cap, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(lang_cap);
    lv_obj_set_flex_grow(lang_cap, 1);

    s_cc_lang_dd = lv_dropdown_create(lang_row);
    lv_dropdown_set_options_static(s_cc_lang_dd, "中文\nEnglish");
    lv_dropdown_set_selected(s_cc_lang_dd, ui_i18n_get_lang() == UI_LANG_ZH ? 0u : 1u);
    lv_obj_set_style_min_width(s_cc_lang_dd, 140, LV_PART_MAIN);
    lv_obj_set_height(s_cc_lang_dd, 40);
    lv_obj_set_style_bg_color(s_cc_lang_dd, lv_color_hex(UI_CC_SETTINGS_CTRL_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_cc_lang_dd, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s_cc_lang_dd, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s_cc_lang_dd, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s_cc_lang_dd, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_cc_lang_dd, UI_CC_SETTINGS_ROW_BORDER_W, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_cc_lang_dd, lv_color_hex(UI_CC_SETTINGS_CTRL_BORDER), LV_PART_MAIN);
    {
        const lv_font_t *f = ui_font_cjk();
        lv_obj_set_style_text_font(s_cc_lang_dd, f != NULL ? f : LV_FONT_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_cc_lang_dd, lv_color_black(), LV_PART_MAIN);
    }
    lv_obj_add_flag(s_cc_lang_dd, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(s_cc_lang_dd, cc_lang_dd_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    ui_style_cc_interactive_focus(s_cc_lang_dd);

    lv_obj_t *dd_list = lv_dropdown_get_list(s_cc_lang_dd);
    if(dd_list != NULL) {
        const lv_font_t *f = ui_font_cjk();
        lv_obj_set_style_text_font(dd_list, f != NULL ? f : LV_FONT_DEFAULT, LV_PART_MAIN);
        lv_obj_set_style_text_font(dd_list, f != NULL ? f : LV_FONT_DEFAULT, LV_PART_SELECTED);
    }

    /** 与 `UI_SETTINGS_SHOW_*`（`ui_settings_config.h`）一一对应；符号见 `lvgl/src/font/lv_symbol_def.h` */
    static const struct {
        ui_str_id_t id;
        const char *sym;
    } s_cc_settings_rows[] = {
#if UI_SETTINGS_SHOW_DATETIME
        { UI_STR_SETTINGS_DATETIME, LV_SYMBOL_LOOP },
#endif
#if UI_SETTINGS_SHOW_POWER_SLEEP
        { UI_STR_SETTINGS_POWER_SLEEP, LV_SYMBOL_POWER },
#endif
#if UI_SETTINGS_SHOW_BATTERY
        { UI_STR_SETTINGS_BATTERY, LV_SYMBOL_BATTERY_FULL },
#endif
#if UI_SETTINGS_SHOW_THERMAL
        { UI_STR_SETTINGS_THERMAL, LV_SYMBOL_WARNING },
#endif
#if UI_SETTINGS_SHOW_SDCARD
        { UI_STR_SETTINGS_SDCARD, LV_SYMBOL_SD_CARD },
#endif
#if UI_SETTINGS_SHOW_FIRMWARE
        { UI_STR_SETTINGS_FIRMWARE, LV_SYMBOL_DOWNLOAD },
#endif
#if UI_SETTINGS_SHOW_LOG
        { UI_STR_SETTINGS_LOG, LV_SYMBOL_FILE },
#endif
#if UI_SETTINGS_SHOW_SECURITY
        { UI_STR_SETTINGS_SECURITY, LV_SYMBOL_EYE_CLOSE },
#endif
#if UI_SETTINGS_SHOW_BT
        { UI_STR_SETTINGS_BT, LV_SYMBOL_BLUETOOTH },
#endif
#if UI_SETTINGS_SHOW_WIFI
        { UI_STR_SETTINGS_WIFI, LV_SYMBOL_WIFI },
#endif
#if UI_SETTINGS_SHOW_USB
        { UI_STR_SETTINGS_USB, LV_SYMBOL_USB },
#endif
#if UI_SETTINGS_SHOW_EXPORT
        { UI_STR_SETTINGS_EXPORT, LV_SYMBOL_UPLOAD },
#endif
#if UI_SETTINGS_SHOW_DEVICE
        { UI_STR_SETTINGS_DEVICE, LV_SYMBOL_DRIVE },
#endif
#if UI_SETTINGS_SHOW_FACTORY
        { UI_STR_SETTINGS_FACTORY, LV_SYMBOL_TRASH },
#endif
    };

    for(unsigned j = 0; j < (unsigned)(sizeof(s_cc_settings_rows) / sizeof(s_cc_settings_rows[0])); j++) {
        const ui_str_id_t sid = s_cc_settings_rows[j].id;
        const char *sym = s_cc_settings_rows[j].sym;
        lv_obj_t *row = lv_obj_create(set_list);
        lv_obj_set_width(row, LV_PCT(100));
        ui_style_cc_settings_row_apply(row);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(row, cc_settings_item_clicked_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)sid);
        ui_style_cc_interactive_focus(row);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *ic = lv_label_create(row);
        lv_label_set_text_static(ic, sym);
        lv_label_set_long_mode(ic, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_font(ic, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(ic, lv_color_hex(0x333333), 0);
        lv_obj_set_style_text_align(ic, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_remove_flag(ic, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_width(ic, 26);

        lv_obj_t *rl = lv_label_create(row);
        ui_i18n_bind_label(rl, sid);
        lv_label_set_long_mode(rl, LV_LABEL_LONG_CLIP);
        ui_style_zone_label(rl);
        lv_obj_set_flex_grow(rl, 1);
    }

    cc_sync_settings_list_geom();

    lv_obj_add_event_cb(s_cc_sheet, cc_sheet_swipe_dismiss_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_cc_sheet, cc_sheet_swipe_dismiss_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_cc_sheet, cc_sheet_swipe_dismiss_cb, LV_EVENT_PRESS_LOST, NULL);

    s_cc_open = false;
}

/** 全屏拍摄模式页：标题栏下滑关闭、横向滚动选模式、卡片图标与 i18n 名称。 */
static void main_create_mode_panel(lv_obj_t *scr)
{
    s_mode_strip = NULL;

    s_mode_dim = lv_obj_create(scr);
    lv_obj_set_size(s_mode_dim, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);
    lv_obj_set_pos(s_mode_dim, 0, 0);
    lv_obj_set_style_bg_color(s_mode_dim, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_mode_dim, LV_OPA_50, 0);
    lv_obj_set_style_border_width(s_mode_dim, 0, 0);
    lv_obj_remove_flag(s_mode_dim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_mode_dim, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_mode_dim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_mode_dim, mode_dim_clicked_cb, LV_EVENT_CLICKED, NULL);

    s_mode = lv_obj_create(scr);
    lv_obj_set_size(s_mode, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);
    lv_obj_set_pos(s_mode, 0, MY_SCREEN_HEIGHT);
    lv_obj_set_style_translate_y(s_mode, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_mode, lv_color_white(), 0);
    lv_obj_set_style_border_width(s_mode, 0, 0);
    lv_obj_remove_flag(s_mode, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_mode, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_mode, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_layout(s_mode, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_mode, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_mode, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *hdr = lv_obj_create(s_mode);
    lv_obj_set_width(hdr, MY_SCREEN_WIDTH);
    lv_obj_set_height(hdr, UI_MODE_PANEL_HEADER_H);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_hor(hdr, 8, 0);
    lv_obj_set_style_pad_ver(hdr, 4, 0);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(hdr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(hdr, mode_panel_gesture_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(hdr, mode_panel_gesture_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(hdr, mode_panel_gesture_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_set_layout(hdr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(hdr);
    ui_i18n_bind_label(title, UI_STR_MODE_TITLE);
    ui_label_i18n_wrap(title, MY_SCREEN_WIDTH - 120);
    ui_style_zone_label(title);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_obj_set_flex_grow(title, 1);
    lv_obj_remove_flag(title, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *close_btn = lv_obj_create(hdr);
    lv_obj_set_size(close_btn, 64, 36);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(UI_THEME_CC_BACK_BTN_BG), 0);
    lv_obj_set_style_border_width(close_btn, 0, 0);
    lv_obj_remove_flag(close_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(close_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(close_btn, mode_close_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_l = lv_label_create(close_btn);
    lv_label_set_text_static(close_l, "X");
    lv_label_set_long_mode(close_l, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(close_l, &lv_font_montserrat_16, 0);
    lv_obj_center(close_l);

    lv_obj_t *body = lv_label_create(s_mode);
    ui_i18n_bind_label(body, UI_STR_MODE_BODY);
    ui_label_i18n_wrap(body, MY_SCREEN_WIDTH - 24);
    ui_style_zone_label(body);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(body, MY_SCREEN_WIDTH - 16);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_CLICKABLE);

    const lv_coord_t card_w = (lv_coord_t)UI_MODE_CARD_W;
    const lv_coord_t page_w = (lv_coord_t)UI_MODE_STRIP_PAGE_W;
    const lv_coord_t col_gap = (lv_coord_t)UI_MODE_STRIP_GAP;
    const lv_coord_t lane_w = page_w + col_gap;
    const lv_coord_t tap_dual_w = lane_w * 2;
    s_mode_tap_side_miss_w = tap_dual_w;

    lv_obj_t *tap_l = lv_obj_create(s_mode);
    lv_obj_add_flag(tap_l, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(tap_l, tap_dual_w, (lv_coord_t)MY_SCREEN_HEIGHT);
    lv_obj_set_pos(tap_l, 0, 0);
    lv_obj_set_style_bg_opa(tap_l, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tap_l, 0, 0);
    lv_obj_remove_flag(tap_l, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tap_l, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tap_l, mode_tap_left_strip_cb, LV_EVENT_CLICKED, NULL);

    const lv_coord_t tap_r_h = (lv_coord_t)(MY_SCREEN_HEIGHT - UI_MODE_PANEL_HEADER_H);
    lv_obj_t *tap_r = lv_obj_create(s_mode);
    lv_obj_add_flag(tap_r, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(tap_r, tap_dual_w, tap_r_h);
    lv_obj_set_pos(tap_r, (lv_coord_t)(MY_SCREEN_WIDTH - tap_dual_w), UI_MODE_PANEL_HEADER_H);
    lv_obj_set_style_bg_opa(tap_r, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tap_r, 0, 0);
    lv_obj_remove_flag(tap_r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tap_r, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tap_r, mode_tap_right_strip_cb, LV_EVENT_CLICKED, NULL);

    s_mode_strip = ui_mode_carousel_create(s_mode, MY_SCREEN_WIDTH);
    lv_obj_add_event_cb(s_mode_strip, mode_strip_hit_test_center_only_cb, LV_EVENT_HIT_TEST, NULL);
    ui_mode_carousel_set_callbacks(s_mode_strip, mode_carousel_on_visual_scroll, mode_carousel_on_mode_committed, NULL);
    ui_mode_carousel_configure_strip(s_mode_strip, card_w, page_w, col_gap);

    ui_shoot_mode_t modes_list[UI_SHOOT_MODE_COUNT];
    uint32_t n = 0u;
    for(ui_shoot_mode_t m = 0; m < UI_SHOOT_MODE_COUNT; m++) {
        if(!ui_shoot_mode_option_enabled(m)) {
            continue;
        }
        if(n < UI_SHOOT_MODE_COUNT) {
            modes_list[n++] = m;
        }
    }

    if(!s_mode_card_tr_inited) {
        static const lv_style_prop_t mode_card_tr_props[] = { LV_STYLE_BORDER_WIDTH, LV_STYLE_BORDER_COLOR,
                                                              LV_STYLE_BG_COLOR, LV_STYLE_PROP_INV };
        lv_style_transition_dsc_init(&s_mode_card_tr, mode_card_tr_props, lv_anim_path_ease_out,
                                     UI_MODE_CARD_SELECT_TRANSITION_MS, 0, NULL);
        s_mode_card_tr_inited = true;
    }

    ui_mode_carousel_build(s_mode_strip, modes_list, n, n >= 2u, &s_mode_card_tr, mode_icon_clicked_cb);

    ui_app_shoot_mode_ensure_enabled();
    /* 默认 scroll 在物理 0；须先对齐 `ui_app` 当前模式（中段槽位），否则开机后首次上滑会先见「列表头」再跳变 */
    main_mode_strip_scroll_to_current(false);
    /* 顶栏盖在左宽触区上：标题区仍以下滑关闭为主，不被整屏左列抢走 */
    lv_obj_move_foreground(hdr);
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

/** 构建相机主界面：顶栏、三列中栏、底栏、控制中心、模式层与预览手势。 */
void ui_page_main_create(lv_obj_t *scr)
{
    ui_page_main_scr_detach_gestures(scr);
    ui_bt_nav_on_main_shown();
    /* 蓝牙页等对 `scr` 设过 FLEX + pad_row；不清则主页子控件仍走 Flex，与下方 align 冲突，跟手滑动易失效 */
    lv_obj_set_layout(scr, LV_LAYOUT_NONE);
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    ui_i18n_reset_bindings();
    ui_indev_apply_pointer_profile();
    ui_app_state_boot_load();
    ui_boot_debug_log_params();

    s_isp = NULL;
    s_cc_dim = NULL;
    s_cc_sheet = NULL;
    s_mode_dim = NULL;
    s_mode = NULL;
    s_mode_strip = NULL;
    s_bottom_mode_icon_l = NULL;
    s_bottom_mode_name_l = NULL;
    s_status_bar = NULL;
    s_status_storage_l = NULL;
    s_status_mode_ic = NULL;
    s_status_bat_l = NULL;
    s_chrome_bt_l = NULL;
    s_cc_lang_dd = NULL;
    s_cc_grid = NULL;
    s_cc_quick_tile_l = NULL;
    s_cc_settings = NULL;
    s_cc_set_list = NULL;
    s_cc_rotation_panel = NULL;
    s_cc_rot_switch = NULL;
    s_cc_rot_angle_list = NULL;
    for(unsigned ri = 0; ri < 4; ri++) {
        s_cc_rot_angle_btns[ri] = NULL;
    }
    s_isp_open = false;
    s_cc_open = false;
    s_mode_open = false;
    s_cc_sheet_h = 0;
    s_vp_dir = MAIN_VP_DIR_NONE;
    s_vp_vert_latch = MAIN_VP_VERT_LATCH_NONE;
    s_vp_cc_drag = false;
    s_vp_mode_preview = false;
    s_vp_isp_preview = false;
    s_vp_decor_layer = NULL;

    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_THEME_SCREEN_BG), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    const int mid_h = MY_SCREEN_HEIGHT - UI_TOP_BAR_H - UI_BOTTOM_BAR_H;

    lv_obj_t *status = lv_obj_create(scr);
    s_status_bar = status;
    lv_obj_add_flag(status, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(status, MY_SCREEN_WIDTH, UI_TOP_BAR_H);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(status, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status, 0, 0);
    lv_obj_set_style_pad_hor(status, 8, 0);
    lv_obj_set_style_pad_ver(status, 4, 0);
    lv_obj_remove_flag(status, LV_OBJ_FLAG_SCROLLABLE);
    /* 顶栏不启用 `ui_region_strip_enable_scroll`：避免纵向可滚与全屏手势抢事件、顶栏「跟着滑」 */
    lv_obj_set_layout(status, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(status, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)"状态栏");

    lv_obj_t *st_left = lv_obj_create(status);
    lv_obj_remove_flag(st_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(st_left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(st_left, 0, 0);
    lv_obj_set_style_pad_column(st_left, 6, 0);
    lv_obj_set_layout(st_left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(st_left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(st_left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(st_left, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *file_ic = lv_label_create(st_left);
    lv_label_set_text_static(file_ic, LV_SYMBOL_FILE);
    lv_obj_set_style_text_font(file_ic, &lv_font_montserrat_20, 0);
    lv_label_set_long_mode(file_ic, LV_LABEL_LONG_CLIP);
    lv_obj_remove_flag(file_ic, LV_OBJ_FLAG_CLICKABLE);

    s_status_storage_l = lv_label_create(st_left);
    lv_label_set_long_mode(s_status_storage_l, LV_LABEL_LONG_CLIP);
    ui_style_zone_label(s_status_storage_l);

    lv_obj_t *st_right = lv_obj_create(status);
    lv_obj_remove_flag(st_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(st_right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(st_right, 0, 0);
    lv_obj_set_style_pad_column(st_right, 8, 0);
    lv_obj_set_layout(st_right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(st_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(st_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(st_right, LV_OBJ_FLAG_EVENT_BUBBLE);
    /* 占满状态栏除左侧外的宽度，避免蓝牙词条与多图标被固定 100px 裁切 */
    lv_obj_set_flex_grow(st_right, 1);
    lv_obj_set_style_min_width(st_right, UI_STATUS_BAR_RIGHT_MIN_W, 0);

    s_chrome_bt_l = lv_label_create(st_right);
    ui_i18n_bind_label(s_chrome_bt_l, UI_STR_BT_STATUS_ON);
    ui_style_zone_label(s_chrome_bt_l);
    lv_label_set_long_mode(s_chrome_bt_l, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(s_chrome_bt_l, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(s_chrome_bt_l, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_remove_flag(s_chrome_bt_l, LV_OBJ_FLAG_CLICKABLE);
    if(!ui_bt_is_enabled()) {
        lv_obj_add_flag(s_chrome_bt_l, LV_OBJ_FLAG_HIDDEN);
    }

    s_status_mode_ic = lv_label_create(st_right);
    lv_label_set_long_mode(s_status_mode_ic, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(s_status_mode_ic, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(s_status_mode_ic, &lv_font_montserrat_20, 0);
    lv_obj_remove_flag(s_status_mode_ic, LV_OBJ_FLAG_CLICKABLE);

    s_status_bat_l = lv_label_create(st_right);
    lv_label_set_long_mode(s_status_bat_l, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(s_status_bat_l, LV_SIZE_CONTENT);
    ui_style_zone_label(s_status_bat_l);
    lv_obj_remove_flag(s_status_bat_l, LV_OBJ_FLAG_CLICKABLE);

    main_refresh_status_bar();

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
        if(i == 0u) {
            lv_obj_set_layout(cell, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(cell, 8, 0);
            lv_obj_remove_flag(cell, LV_OBJ_FLAG_CLICKABLE);
            s_bottom_mode_icon_l = lv_label_create(cell);
            lv_label_set_long_mode(s_bottom_mode_icon_l, LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_font(s_bottom_mode_icon_l, &lv_font_montserrat_20, 0);
            lv_obj_remove_flag(s_bottom_mode_icon_l, LV_OBJ_FLAG_CLICKABLE);
            s_bottom_mode_name_l = lv_label_create(cell);
            lv_label_set_long_mode(s_bottom_mode_name_l, LV_LABEL_LONG_WRAP);
            ui_label_i18n_wrap(s_bottom_mode_name_l, MY_SCREEN_WIDTH / 3 - 48);
            lv_obj_set_style_text_align(s_bottom_mode_name_l, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
            lv_obj_remove_flag(s_bottom_mode_name_l, LV_OBJ_FLAG_CLICKABLE);
        }
        else {
            /* 与左格一致：Flex 纵横向居中，避免 `TOP_MID` 与「图标+模式名」行顶不齐 */
            lv_obj_set_layout(cell, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_t *cell_l = lv_label_create(cell);
            ui_i18n_bind_label(cell_l, bottom_ids[i]);
            ui_label_i18n_wrap(cell_l, MY_SCREEN_WIDTH / 3 - 20);
            lv_obj_set_width(cell_l, LV_PCT(100));
            lv_obj_set_style_text_align(cell_l, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            ui_style_zone_label(cell_l);
            lv_obj_add_event_cb(cell, ui_evt_zone_click_cb, LV_EVENT_CLICKED, (void *)bottom_dbg[i]);
        }
    }
    main_refresh_bottom_mode_cell();

    main_scr_build_preview_decor(scr, mid_h);
    lv_obj_move_foreground(s_vp_decor_layer);

    main_create_control_center(scr);
    main_create_mode_panel(scr);

    lv_obj_add_event_cb(scr, main_viewport_gesture_cb, LV_EVENT_PRESSED, scr);
    lv_obj_add_event_cb(scr, main_viewport_gesture_cb, LV_EVENT_PRESSING, scr);
    lv_obj_add_event_cb(scr, main_viewport_gesture_cb, LV_EVENT_RELEASED, scr);
    lv_obj_add_event_cb(scr, main_viewport_gesture_cb, LV_EVENT_PRESS_LOST, scr);

    lv_indev_reset(NULL, NULL);

    ui_cc_settings_apply_rotation_to_hw();

    ui_app_shoot_mode_observer_register(main_shoot_mode_observer_cb, NULL);
}

/** 刷新顶栏存储/模式/电量/蓝牙等；仅主屏控件仍有效时安全调用。 */
void ui_page_main_refresh_chrome(void)
{
    main_refresh_status_bar();
}
