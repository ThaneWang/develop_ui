/**
 * @file ui_app_state.c
 * @brief 应用全局状态默认值与访问器（模拟器占位）；拍摄模式可写入 **`UI_APP_SHOOT_MODE_PERSIST_FILE`**。
 */
#include "ui_app_state.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ui_hw_hal.h"
#include "lvgl/lvgl.h"

#ifndef UI_APP_SHOOT_MODE_OBSERVER_MAX
#define UI_APP_SHOOT_MODE_OBSERVER_MAX 8
#endif

static ui_shoot_mode_t s_shoot_mode = UI_SHOOT_MODE_3DGS;

static ui_app_shoot_mode_observer_fn s_mode_obs_fn[UI_APP_SHOOT_MODE_OBSERVER_MAX];
static void *s_mode_obs_ud[UI_APP_SHOOT_MODE_OBSERVER_MAX];

/** 依次调用已登记观察者。 */
static void ui_app_shoot_mode_emit_observers(void)
{
    for(unsigned i = 0; i < UI_APP_SHOOT_MODE_OBSERVER_MAX; i++) {
        if(s_mode_obs_fn[i] != NULL) {
            s_mode_obs_fn[i](s_mode_obs_ud[i]);
        }
    }
}

bool ui_app_shoot_mode_observer_register(ui_app_shoot_mode_observer_fn fn, void *user_data)
{
    if(fn == NULL) {
        return false;
    }
    for(unsigned i = 0; i < UI_APP_SHOOT_MODE_OBSERVER_MAX; i++) {
        if(s_mode_obs_fn[i] == fn && s_mode_obs_ud[i] == user_data) {
            return true;
        }
    }
    for(unsigned j = 0; j < UI_APP_SHOOT_MODE_OBSERVER_MAX; j++) {
        if(s_mode_obs_fn[j] == NULL) {
            s_mode_obs_fn[j] = fn;
            s_mode_obs_ud[j] = user_data;
            return true;
        }
    }
    return false;
}

void ui_app_shoot_mode_observer_unregister(ui_app_shoot_mode_observer_fn fn, void *user_data)
{
    if(fn == NULL) {
        return;
    }
    for(unsigned i = 0; i < UI_APP_SHOOT_MODE_OBSERVER_MAX; i++) {
        if(s_mode_obs_fn[i] == fn && s_mode_obs_ud[i] == user_data) {
            s_mode_obs_fn[i] = NULL;
            s_mode_obs_ud[i] = NULL;
            return;
        }
    }
}

void ui_app_shoot_mode_observer_unregister_all(void)
{
    memset(s_mode_obs_fn, 0, sizeof(s_mode_obs_fn));
    memset(s_mode_obs_ud, 0, sizeof(s_mode_obs_ud));
}

/** 将当前 `s_shoot_mode` 写入持久化文件（失败则静默）。 */
static void ui_app_persist_shoot_mode_save(void)
{
    FILE *fp = fopen(UI_APP_SHOOT_MODE_PERSIST_FILE, "wb");
    if(fp == NULL) {
        return;
    }
    const unsigned char v = (unsigned char)s_shoot_mode;
    (void)fwrite(&v, 1, 1, fp);
    fclose(fp);
}
static uint32_t s_storage_free_gb = 32u;
static uint8_t s_battery_pct = 100u;

bool ui_shoot_mode_option_enabled(ui_shoot_mode_t m)
{
    switch(m) {
        case UI_SHOOT_MODE_3DGS:
            return UI_SHOOT_ENABLE_3DGS != 0;
        case UI_SHOOT_MODE_VIDEO:
            return UI_SHOOT_ENABLE_VIDEO != 0;
        case UI_SHOOT_MODE_PHOTO:
            return UI_SHOOT_ENABLE_PHOTO != 0;
        case UI_SHOOT_MODE_AI_DIRECTOR:
            return UI_SHOOT_ENABLE_AI_DIRECTOR != 0;
        case UI_SHOOT_MODE_3DGS_VIDEO:
            return UI_SHOOT_ENABLE_3DGS_VIDEO != 0;
        case UI_SHOOT_MODE_FREE_RATIO_VIDEO:
            return UI_SHOOT_ENABLE_FREE_RATIO_VIDEO != 0;
        case UI_SHOOT_MODE_DUAL_LENS_VIDEO:
            return UI_SHOOT_ENABLE_DUAL_LENS_VIDEO != 0;
        default:
            return false;
    }
}

ui_shoot_mode_t ui_shoot_mode_first_enabled(void)
{
    for(ui_shoot_mode_t m = 0; m < UI_SHOOT_MODE_COUNT; m++) {
        if(ui_shoot_mode_option_enabled(m)) {
            return m;
        }
    }
    return UI_SHOOT_MODE_3DGS;
}

void ui_app_shoot_mode_ensure_enabled(void)
{
    if(s_shoot_mode >= UI_SHOOT_MODE_COUNT || !ui_shoot_mode_option_enabled(s_shoot_mode)) {
        s_shoot_mode = ui_shoot_mode_first_enabled();
        ui_app_persist_shoot_mode_save();
        ui_app_shoot_mode_emit_observers();
    }
}

/** 将拍摄模式、存储与电量恢复为模拟器默认值。 */
void ui_app_state_init(void)
{
    s_shoot_mode = ui_shoot_mode_first_enabled();
    s_storage_free_gb = 32u;
    s_battery_pct = 100u;
}

void ui_app_state_boot_load(void)
{
    FILE *fp = fopen(UI_APP_SHOOT_MODE_PERSIST_FILE, "rb");
    if(fp == NULL) {
        s_shoot_mode = ui_shoot_mode_first_enabled();
        return;
    }
    unsigned char v = 0xFFu;
    const size_t n = fread(&v, 1, 1, fp);
    fclose(fp);
    if(n != 1u || v >= (unsigned char)UI_SHOOT_MODE_COUNT || !ui_shoot_mode_option_enabled((ui_shoot_mode_t)v)) {
        s_shoot_mode = ui_shoot_mode_first_enabled();
        return;
    }
    s_shoot_mode = (ui_shoot_mode_t)v;
}

/** 设置当前拍摄模式；非法或未开放则忽略。 */
void ui_app_set_shoot_mode(ui_shoot_mode_t m)
{
    if(m >= UI_SHOOT_MODE_COUNT || !ui_shoot_mode_option_enabled(m)) {
        return;
    }
    s_shoot_mode = m;
    ui_app_persist_shoot_mode_save();
    ui_hw_shoot_mode_apply(m);
    ui_app_shoot_mode_emit_observers();
}

/** 返回当前拍摄模式。 */
ui_shoot_mode_t ui_app_get_shoot_mode(void)
{
    return s_shoot_mode;
}

/** 返回用于 Label 的 LVGL 符号字符串（Montserrat 图标字体）。 */
const char *ui_app_shoot_mode_icon_glyph(ui_shoot_mode_t m)
{
    switch(m) {
        case UI_SHOOT_MODE_3DGS:
            return LV_SYMBOL_EYE_OPEN;
        case UI_SHOOT_MODE_VIDEO:
            return LV_SYMBOL_VIDEO;
        case UI_SHOOT_MODE_PHOTO:
            return LV_SYMBOL_IMAGE;
        case UI_SHOOT_MODE_AI_DIRECTOR:
            return LV_SYMBOL_GPS;
        case UI_SHOOT_MODE_3DGS_VIDEO:
            return LV_SYMBOL_LOOP;
        case UI_SHOOT_MODE_FREE_RATIO_VIDEO:
            return LV_SYMBOL_TINT;
        case UI_SHOOT_MODE_DUAL_LENS_VIDEO:
            return LV_SYMBOL_EYE_CLOSE;
        default:
            return LV_SYMBOL_IMAGE;
    }
}

/** 占位：剩余存储空间（GiB 整数）。 */
uint32_t ui_app_get_storage_free_gb(void)
{
    return s_storage_free_gb;
}

/** 占位：设置剩余存储 GiB。 */
void ui_app_set_storage_free_gb(uint32_t gb)
{
    s_storage_free_gb = gb;
}

/** 占位：电量百分比 0～100。 */
uint8_t ui_app_get_battery_percent(void)
{
    return s_battery_pct;
}

/** 占位：设置电量百分比，大于 100 时钳位到 100。 */
void ui_app_set_battery_percent(uint8_t pct)
{
    if(pct > 100u) {
        pct = 100u;
    }
    s_battery_pct = pct;
}
