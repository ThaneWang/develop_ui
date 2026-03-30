/**
 * @file ui_hw_hal.c
 * @brief 外设占位：打印参数，便于模拟器验收与后续替换为真实驱动。
 */
#include "ui_hw_hal.h"

#include <stdio.h>

#include "../logging.h"

static const char *screen_rot_name(ui_screen_rotation_t r)
{
    switch(r) {
        case UI_SCREEN_ROT_CW_0:
            return "0";
        case UI_SCREEN_ROT_CW_90:
            return "90";
        case UI_SCREEN_ROT_CW_180:
            return "180";
        case UI_SCREEN_ROT_CW_270:
            return "270";
        default:
            return "?";
    }
}

void ui_hw_display_rotation_set(bool enabled, ui_screen_rotation_t rot)
{
    printf("[HW] display rotation: %s, %s deg CW\n", enabled ? "ON" : "OFF", screen_rot_name(rot));
    LOG_DEBUG("HW display rotation %s %s deg", enabled ? "ON" : "OFF", screen_rot_name(rot));
}

void ui_hw_bluetooth_set_enabled(bool on)
{
    printf("[HW] bluetooth: %s\n", on ? "ON" : "OFF");
    LOG_DEBUG("HW bluetooth %s", on ? "ON" : "OFF");
}

void ui_hw_brightness_set(uint8_t level_0_100)
{
    if(level_0_100 > 100u) {
        level_0_100 = 100u;
    }
    printf("[HW] brightness: %u%%\n", (unsigned)level_0_100);
    LOG_DEBUG("HW brightness %u", (unsigned)level_0_100);
}

void ui_hw_volume_set(uint8_t level_0_100)
{
    if(level_0_100 > 100u) {
        level_0_100 = 100u;
    }
    printf("[HW] volume: %u%%\n", (unsigned)level_0_100);
    LOG_DEBUG("HW volume %u", (unsigned)level_0_100);
}

void ui_hw_shoot_mode_apply(ui_shoot_mode_t m)
{
    printf("[HW] shoot mode apply: enum=%d\n", (int)m);
    LOG_DEBUG("HW shoot mode %d", (int)m);
}
