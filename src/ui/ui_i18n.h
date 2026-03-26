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
    UI_STR_NOTE = 0,
    UI_STR_STATUS_ZONE,
    UI_STR_RESERVED_ZONE,
    UI_STR_ISP_ZONE,
    UI_STR_SWIPE_RIGHT_REPLAY,
    UI_STR_BOTTOM_MODE_DISP,
    UI_STR_BOTTOM_MODE_PARAM,
    UI_STR_BOTTOM_VIEW_CTRL,
    UI_STR_REPLAY_TITLE,
    UI_STR_REPLAY_BODY,
    UI_STR_CC_TITLE,
    UI_STR_CC_LANG_LABEL,
    UI_STR_CC_TILE_ROTATION,
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
