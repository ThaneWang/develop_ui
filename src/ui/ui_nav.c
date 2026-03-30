/**
 * @file ui_nav.c
 * @brief 全屏页面切换：`lv_obj_clean` + `ui_page_*_create`，供 `lv_async_call` 与事件层调用，避免在事件回调内直接清屏。
 *
 * 清屏后须 **`lv_indev_reset(NULL, NULL)`**：否则指针类 indev 仍可能引用已删除对象，下一页会出现点击漂移、手势错乱或「主页异常」。
 */
#include "ui_nav.h"
#include "ui_app_state.h"
#include "ui_bt_nav.h"
#include "ui_display.h"
#include "ui_i18n.h"
#include "ui_page_main.h"
#include "ui_page_replay.h"
#include "../logging.h"
#include "lvgl/lvgl.h"

void ui_nav_rebuild_main(lv_obj_t *scr)
{
    ui_app_shoot_mode_observer_unregister_all();
    ui_i18n_reset_bindings();
    lv_obj_clean(scr);
    lv_indev_reset(NULL, NULL);
    ui_display_set_screen_rotation(UI_SCREEN_ROT_CW_0);
    ui_page_main_create(scr);
}

/** `lv_async_call`：清屏、复位旋转与 i18n 绑定，进入回放页。 */
void ui_nav_replace_with_replay_async(void *user_data)
{
    LV_UNUSED(user_data);
    lv_obj_t *scr = lv_scr_act();
    ui_page_main_scr_detach_gestures(scr);
    ui_display_set_screen_rotation(UI_SCREEN_ROT_CW_0);
    ui_app_shoot_mode_observer_unregister_all();
    ui_i18n_reset_bindings();
    lv_obj_clean(scr);
    lv_indev_reset(NULL, NULL);
    ui_page_replay_create(scr);
}

/** `lv_async_call`：清屏并重置 i18n 绑定，重建主界面。 */
void ui_nav_replace_with_main_async(void *user_data)
{
    LV_UNUSED(user_data);
    ui_nav_rebuild_main(lv_scr_act());
    LOG_DEBUG("主页已恢复（与进入回放前一致）");
}

/** `lv_async_call`：进入蓝牙设置（实现见 **`ui_bt_nav`**，防重入）。 */
void ui_nav_replace_with_bt_settings_async(void *user_data)
{
    LV_UNUSED(user_data);
    ui_bt_nav_open_async();
}
