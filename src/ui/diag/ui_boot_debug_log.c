/**
 * @file ui_boot_debug_log.c
 * @brief 汇总打印模拟器/占位状态，便于开机对照 `ui_sim_settings.bin`、`ui_shoot_mode.dat` 与 `ui_common` 阈值。
 */
#include "ui_boot_debug_log.h"

#include <stdbool.h>
#include <stdio.h>

#include "../../logging.h"
#include "ui.h"
#include "ui_app_state.h"
#include "ui_bt_state.h"
#include "ui_cc_settings_state.h"
#include "ui_common.h"
#include "ui_i18n.h"
#include "ui_sim_settings_persist.h"

static bool s_boot_params_logged;

void ui_boot_debug_log_params(void)
{
    if(s_boot_params_logged) {
        return;
    }
    s_boot_params_logged = true;

    const ui_shoot_mode_t sm = ui_app_get_shoot_mode();
    printf("[BOOT] ========== system params (debug) ==========\n");
    printf("[BOOT] screen: %d x %d\n", (int)MY_SCREEN_WIDTH, (int)MY_SCREEN_HEIGHT);
    printf("[BOOT] FW model=%s ver=%s build=%s\n", UI_FW_PRODUCT_MODEL, UI_FW_VERSION_STRING, UI_FW_BUILD_ID);
    printf("[BOOT] persist: settings=%s shoot_mode=%s\n", UI_SIM_SETTINGS_PERSIST_FILE, UI_APP_SHOOT_MODE_PERSIST_FILE);
    printf("[BOOT] shoot_mode: %d (%s)\n", (int)sm, ui_i18n_str(ui_i18n_shoot_mode_label_id(sm)));
    printf("[BOOT] storage_free_gib: %u, battery: %u%%\n", (unsigned)ui_app_get_storage_free_gb(),
           (unsigned)ui_app_get_battery_percent());
    printf("[BOOT] i18n_lang: %s\n", ui_i18n_get_lang() == UI_LANG_ZH ? "ZH" : "EN");
    printf("[BOOT] bluetooth_enabled: %d, bt_low_power: %d\n", ui_bt_is_enabled() ? 1 : 0, ui_bt_low_power_is_on() ? 1 : 0);
    printf("[BOOT] brightness: %u%%, volume: %u%%\n", (unsigned)ui_cc_settings_get_brightness(),
           (unsigned)ui_cc_settings_get_volume());
    printf("[BOOT] display_rotation: enabled=%d quadrant_idx=%u\n", ui_cc_settings_get_rotation_enabled() ? 1 : 0,
           (unsigned)ui_cc_settings_get_rotation_index());
    printf("[BOOT] swipe commit_dx=%d commit_dy=%d dir_lock_px=%d max_abs_dy=%d max_abs_dx=%d\n", (int)UI_SWIPE_COMMIT_DX,
           (int)UI_SWIPE_COMMIT_DY, (int)UI_SWIPE_DIR_LOCK_PX, (int)UI_SWIPE_MAX_ABS_DY, (int)UI_SWIPE_MAX_ABS_DX);
    printf("[BOOT] indev scroll_limit_px=%d scroll_throw_pct=%d long_press_ms=%d\n", (int)UI_INDEV_SCROLL_LIMIT_PX,
           (int)UI_INDEV_SCROLL_THROW_PCT, (int)UI_INDEV_LONG_PRESS_MS);
    printf("[BOOT] =============================================\n");

    LOG_DEBUG(
        "BOOT params: %dx%d mode=%d lang=%s bt=%d lp=%d bri=%u vol=%u rot=%d/%u", (int)MY_SCREEN_WIDTH,
        (int)MY_SCREEN_HEIGHT, (int)sm, ui_i18n_get_lang() == UI_LANG_ZH ? "ZH" : "EN", ui_bt_is_enabled() ? 1 : 0,
        ui_bt_low_power_is_on() ? 1 : 0, (unsigned)ui_cc_settings_get_brightness(), (unsigned)ui_cc_settings_get_volume(),
        ui_cc_settings_get_rotation_enabled() ? 1 : 0, (unsigned)ui_cc_settings_get_rotation_index());
}
