/**
 * @file ui_i18n.h
 * @brief 简中英文词条：绑定 Label 后在 `ui_i18n_refresh_all()` 中统一刷新；切页前须 `ui_i18n_reset_bindings()`。
 */
#ifndef UI_I18N_H
#define UI_I18N_H

#include "lvgl/lvgl.h"

typedef enum {
    UI_LANG_ZH = 0,
    UI_LANG_EN,
} ui_lang_t;

typedef enum {
    UI_STR_STATUS_ZONE = 0,
    UI_STR_RESERVED_ZONE,
    UI_STR_ISP_ZONE,
    /** 预览区手势行：左列文案 + 箭头符号 + 右列文案（各 4 组） */
    UI_STR_VP_GEST_RIGHT,
    UI_STR_VP_ACT_REPLAY,
    UI_STR_VP_GEST_LEFT,
    UI_STR_VP_ACT_ISP,
    UI_STR_VP_GEST_DOWN,
    UI_STR_VP_ACT_CC,
    UI_STR_VP_GEST_UP,
    UI_STR_VP_ACT_MODE,
    UI_STR_BOTTOM_MODE_DISP,
    UI_STR_BOTTOM_MODE_PARAM,
    UI_STR_BOTTOM_VIEW_CTRL,
    UI_STR_REPLAY_TITLE,
    UI_STR_REPLAY_BODY,
    UI_STR_CC_TITLE,
    UI_STR_CC_LANG_LABEL,
    UI_STR_CC_TILE_ROTATION,
    UI_STR_CC_ROT_TITLE,
    UI_STR_CC_ROT_ENABLE,
    UI_STR_CC_ROT_ANGLE_0,
    UI_STR_CC_ROT_ANGLE_90,
    UI_STR_CC_ROT_ANGLE_180,
    UI_STR_CC_ROT_ANGLE_270,
    UI_STR_CC_TILE_LOCK_SCREEN,
    UI_STR_CC_TILE_VOICE,
    UI_STR_CC_TILE_SYSTEM,
    UI_STR_CC_TILE_BRIGHTNESS,
    UI_STR_CC_TILE_VOLUME,
    UI_STR_CC_TILE_QUICK,
    UI_STR_CC_TILE_THEME,
    UI_STR_SETTINGS_TITLE,
    UI_STR_SETTINGS_BACK,
    UI_STR_SETTINGS_DATETIME,
    UI_STR_SETTINGS_FACTORY,
    UI_STR_SETTINGS_DEVICE,
    /** 固件框架 §6 / §5：系统设置列表（见 `docs/firmware-fw-v1-framework.md`） */
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
    /** 蓝牙设置页 / 主屏提示（§5.2） */
    UI_STR_BT_PAGE_TITLE,
    UI_STR_BT_MASTER_SWITCH,
    UI_STR_BT_PAIR,
    UI_STR_BT_RECONNECT,
    UI_STR_BT_LOW_POWER,
    UI_STR_BT_WIFI_SET,
    UI_STR_BT_STATUS_ON,
    UI_STR_MODE_TITLE,
    UI_STR_MODE_BODY,
    UI_STR_COUNT
} ui_str_id_t;

void ui_i18n_reset_bindings(void);
void ui_i18n_bind_label(lv_obj_t *label, ui_str_id_t id);
const char *ui_i18n_str(ui_str_id_t id);
void ui_i18n_set_lang(ui_lang_t lang);
ui_lang_t ui_i18n_get_lang(void);
void ui_i18n_refresh_all(void);

#endif
