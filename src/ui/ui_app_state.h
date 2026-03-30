/**
 * @file ui_app_state.h
 * @brief 全局应用选项占位（语种见 ui_i18n；此处为拍摄模式、存储与电量等，后续可接固件/设置）。
 */
#ifndef UI_APP_STATE_H
#define UI_APP_STATE_H

#include <stdint.h>

typedef enum {
    UI_SHOOT_MODE_3DGS = 0,
    UI_SHOOT_MODE_VIDEO,
    UI_SHOOT_MODE_3DGS_PLUS_VIDEO,
    UI_SHOOT_MODE_STILL,
    UI_SHOOT_MODE_COUNT,
} ui_shoot_mode_t;

/** 恢复默认拍摄模式、存储与电量占位。 */
void ui_app_state_init(void);

/** 设置当前拍摄模式。 */
void ui_app_set_shoot_mode(ui_shoot_mode_t m);
/** 查询当前拍摄模式。 */
ui_shoot_mode_t ui_app_get_shoot_mode(void);

/** 状态栏 / 模式卡片用大号符号字体显示 */
const char *ui_app_shoot_mode_icon_glyph(ui_shoot_mode_t m);

/** 占位：剩余存储 GiB。 */
uint32_t ui_app_get_storage_free_gb(void);
/** 占位：设置剩余存储 GiB。 */
void ui_app_set_storage_free_gb(uint32_t gb);

/** 占位：电量 0～100。 */
uint8_t ui_app_get_battery_percent(void);
/** 占位：设置电量百分比。 */
void ui_app_set_battery_percent(uint8_t pct);

#endif
