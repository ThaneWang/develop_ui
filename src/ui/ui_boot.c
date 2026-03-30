/**
 * @file ui_boot.c
 * @brief 全屏开机动画：标题渐显、进度条、转圈，结束后淡出并进入 `ui_page_main_create`。
 * 主副标题文案与字体随 `ui_i18n_get_lang()`（中文用 `ui_font_cjk()`，英文用 Montserrat）。
 */
#include "ui_boot.h"
#include "ui.h"
#include "ui_app_state.h"
#include "ui_font.h"
#include "ui_i18n.h"
#include "ui_page_main.h"
#include "../logging.h"
#include "lvgl/lvgl.h"

#define UI_BOOT_BAR_MS    2200
#define UI_BOOT_FADE_MS   420
#define UI_BOOT_TITLE_MS  3000

static lv_obj_t *s_boot_root;

/** 开机动画：根容器透明度动画执行回调。 */
static void boot_fade_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_MAIN);
}

/** 进度条数值动画：将 bar 设为当前帧 `v`。 */
static void boot_bar_value_cb(void *var, int32_t v)
{
    lv_bar_set_value((lv_obj_t *)var, (int32_t)v, LV_ANIM_OFF);
}

/** 开机动画淡出结束：清屏并创建主界面。 */
static void boot_fade_done_cb(lv_anim_t *a)
{
    LV_UNUSED(a);
    if(s_boot_root == NULL || !lv_obj_is_valid(s_boot_root)) {
        s_boot_root = NULL;
        ui_page_main_create(lv_scr_act());
        return;
    }
    lv_obj_t *scr = lv_obj_get_screen(s_boot_root);
    s_boot_root = NULL;
    lv_obj_clean(scr);
    ui_page_main_create(scr);
    LOG_DEBUG("开机动画结束，进入主界面");
}

/** 进度条跑满后：启动全屏淡出动画。 */
static void boot_bar_done_cb(lv_anim_t *a)
{
    LV_UNUSED(a);
    if(s_boot_root == NULL || !lv_obj_is_valid(s_boot_root)) {
        return;
    }
    lv_anim_t fade;
    lv_anim_init(&fade);
    lv_anim_set_var(&fade, s_boot_root);
    lv_anim_set_values(&fade, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_exec_cb(&fade, boot_fade_opa_cb);
    lv_anim_set_duration(&fade, UI_BOOT_FADE_MS);
    lv_anim_set_path_cb(&fade, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&fade, boot_fade_done_cb);
    lv_anim_start(&fade);
}

/** 标题 Label 渐显动画执行回调。 */
static void boot_title_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_MAIN);
}

/** 按 `ui_i18n_get_lang()` 与 `UI_FW_*` 写入开机动画主标题（两行）。 */
static void boot_set_title_i18n(lv_obj_t *title)
{
    char line1[96];
    char line2[96];
    char buf[220];
    lv_snprintf(line1, sizeof(line1), ui_i18n_str(UI_STR_BOOT_WELCOME_FMT), UI_FW_PRODUCT_MODEL);
    lv_snprintf(line2, sizeof(line2), ui_i18n_str(UI_STR_BOOT_VERSION_FMT), UI_FW_VERSION_STRING);
    lv_snprintf(buf, sizeof(buf), "%s\n%s", line1, line2);
    lv_label_set_text(title, buf);
    if(ui_i18n_get_lang() == UI_LANG_ZH) {
        const lv_font_t *cj = ui_font_cjk();
        if(cj != NULL) {
            lv_obj_set_style_text_font(title, cj, LV_PART_MAIN);
        }
    }
    else {
        lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    }
}

/** 开机动画副标题：与系统语种一致。 */
static void boot_set_subtitle_i18n(lv_obj_t *sub)
{
    lv_label_set_text(sub, ui_i18n_str(UI_STR_BOOT_STARTING));
    if(ui_i18n_get_lang() == UI_LANG_ZH) {
        const lv_font_t *cj = ui_font_cjk();
        if(cj != NULL) {
            lv_obj_set_style_text_font(sub, cj, LV_PART_MAIN);
        }
    }
    else {
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, LV_PART_MAIN);
    }
}

/** 清当前屏、搭建开机动画层并启动进度条与标题动画。 */
void ui_boot_show_then_main(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);

    s_boot_root = lv_obj_create(scr);
    lv_obj_set_size(s_boot_root, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);
    lv_obj_set_pos(s_boot_root, 0, 0);
    lv_obj_set_style_bg_color(s_boot_root, lv_color_hex(0x12121c), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_boot_root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_boot_root, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_boot_root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_boot_root);
    boot_set_title_i18n(title);
    lv_obj_set_style_text_color(title, lv_color_hex(0xe8e8f0), LV_PART_MAIN);
    lv_obj_set_style_opa(title, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -72);

    lv_anim_t title_anim;
    lv_anim_init(&title_anim);
    lv_anim_set_var(&title_anim, title);
    lv_anim_set_values(&title_anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&title_anim, boot_title_opa_cb);
    lv_anim_set_duration(&title_anim, UI_BOOT_TITLE_MS);
    lv_anim_set_path_cb(&title_anim, lv_anim_path_ease_out);
    lv_anim_start(&title_anim);

    lv_obj_t *sub = lv_label_create(s_boot_root);
    boot_set_subtitle_i18n(sub);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x8888a0), LV_PART_MAIN);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, -38);

    lv_obj_t *bar = lv_bar_create(s_boot_root);
    lv_obj_set_size(bar, MY_SCREEN_WIDTH - 120, 8);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 28);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x2a2a3a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x5b7fd9), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);

    lv_obj_t *spin = lv_spinner_create(s_boot_root);
    lv_obj_set_size(spin, 48, 48);
    lv_obj_align(spin, LV_ALIGN_CENTER, 0, 88);
    lv_obj_set_style_arc_color(spin, lv_color_hex(0x5b7fd9), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spin, 4, LV_PART_INDICATOR);
    lv_spinner_set_anim_params(spin, 900, 200);

    lv_anim_t bar_anim;
    lv_anim_init(&bar_anim);
    lv_anim_set_var(&bar_anim, bar);
    lv_anim_set_values(&bar_anim, 0, 100);
    lv_anim_set_exec_cb(&bar_anim, boot_bar_value_cb);
    lv_anim_set_duration(&bar_anim, UI_BOOT_BAR_MS);
    lv_anim_set_path_cb(&bar_anim, lv_anim_path_ease_in_out);
    lv_anim_set_completed_cb(&bar_anim, boot_bar_done_cb);
    lv_anim_start(&bar_anim);

    LOG_DEBUG("开机动画开始");
}
