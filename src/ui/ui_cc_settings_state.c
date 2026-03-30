/**
 * @file ui_cc_settings_state.c
 * @brief 控制中心亮度/音量/旋转占位状态，与 `ui_sim_settings_persist_save` 联动。
 */
#include "ui_cc_settings_state.h"

#include "ui_display.h"
#include "ui_hw_hal.h"
#include "ui_sim_settings_persist.h"

static const ui_screen_rotation_t s_rot_map[4] = {
    UI_SCREEN_ROT_CW_0,
    UI_SCREEN_ROT_CW_90,
    UI_SCREEN_ROT_CW_180,
    UI_SCREEN_ROT_CW_270,
};

static uint8_t s_brightness = 50u;
static uint8_t s_volume = 50u;
static bool s_rot_enabled;
static uint8_t s_rot_idx;

void ui_cc_settings_boot_restore(uint8_t brightness_0_100, uint8_t volume_0_100, uint8_t rotation_enabled,
                                 uint8_t rotation_idx_0_3)
{
    s_brightness = brightness_0_100 > 100u ? 100u : brightness_0_100;
    s_volume = volume_0_100 > 100u ? 100u : volume_0_100;
    s_rot_enabled = rotation_enabled != 0;
    s_rot_idx = rotation_idx_0_3 > 3u ? 0u : rotation_idx_0_3;
    ui_hw_brightness_set(s_brightness);
    ui_hw_volume_set(s_volume);
    ui_cc_settings_apply_rotation_to_hw();
}

uint8_t ui_cc_settings_get_brightness(void)
{
    return s_brightness;
}

uint8_t ui_cc_settings_get_volume(void)
{
    return s_volume;
}

bool ui_cc_settings_get_rotation_enabled(void)
{
    return s_rot_enabled;
}

uint8_t ui_cc_settings_get_rotation_index(void)
{
    return s_rot_idx;
}

void ui_cc_settings_set_brightness(uint8_t level_0_100)
{
    if(level_0_100 > 100u) {
        level_0_100 = 100u;
    }
    if(level_0_100 == s_brightness) {
        return;
    }
    s_brightness = level_0_100;
    ui_hw_brightness_set(s_brightness);
    ui_sim_settings_persist_save();
}

void ui_cc_settings_set_volume(uint8_t level_0_100)
{
    if(level_0_100 > 100u) {
        level_0_100 = 100u;
    }
    if(level_0_100 == s_volume) {
        return;
    }
    s_volume = level_0_100;
    ui_hw_volume_set(s_volume);
    ui_sim_settings_persist_save();
}

void ui_cc_settings_set_rotation(bool enabled, uint8_t idx_0_3)
{
    const uint8_t idx = idx_0_3 > 3u ? 0u : idx_0_3;
    if(enabled == s_rot_enabled && idx == s_rot_idx) {
        ui_cc_settings_apply_rotation_to_hw();
        return;
    }
    s_rot_enabled = enabled;
    s_rot_idx = idx;
    ui_cc_settings_apply_rotation_to_hw();
    ui_sim_settings_persist_save();
}

void ui_cc_settings_apply_rotation_to_hw(void)
{
    if(!s_rot_enabled) {
        ui_hw_display_rotation_set(false, UI_SCREEN_ROT_CW_0);
        return;
    }
    ui_hw_display_rotation_set(true, s_rot_map[s_rot_idx]);
}
