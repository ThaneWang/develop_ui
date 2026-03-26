/**
 * @file ui_events.c
 * @brief 集中实现 LVGL 事件回调：区域点击、回放页左滑；主界面滑动手势在 `ui_page_main.c`。
 *
 * @note 回放页左滑与主页、控制中心、模式页使用同一套位移阈值（`ui_common.h`），
 *       详细约定见 **`.cursor/ui_swipe_gestures.md`**。
 */
#include "ui_events.h"
#include "ui_common.h"
#include "ui_nav.h"
#include "../logging.h"
#include "lvgl/lvgl.h"

/** 回放页手势：按下时的触点（屏幕坐标） */
static lv_point_t s_huifang_press;
/** 回放页：是否在等待一次完整的 PRESSED→RELEASED（避免 PRESS_LOST 后仍误判） */
static bool s_huifang_tracking;

/**
 * @brief 区域点击调试：将 `user_data` 中的名称打印到控制台与日志。
 */
void ui_evt_zone_click_cb(lv_event_t *e)
{
    const char *name = (const char *)lv_event_get_user_data(e);
    if(name != NULL) {
        printf("%s\n", name);
        LOG_DEBUG("%s", name);
    }
}

/**
 * @brief 回放全屏区域指针手势：左滑返回主页。
 *
 * 事件链：`PRESSED` 记录起点 → `RELEASED` 计算 dx/dy。
 * 判定（与文档一致）：dx <= -UI_SWIPE_MIN_DX，|dy| <= UI_SWIPE_MAX_ABS_DY，且 |dx| > |dy|（横向占优）。
 * 切页通过 `lv_async_call(ui_nav_replace_with_main_async)`，避免在输入事件回调内直接 `lv_obj_clean`。
 */
void ui_evt_replay_zuohua_pointer_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    if(indev == NULL) {
        return;
    }

    if(code == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &s_huifang_press);
        s_huifang_tracking = true;
        return;
    }

    if(code == LV_EVENT_PRESS_LOST) {
        s_huifang_tracking = false;
        return;
    }

    if(code != LV_EVENT_RELEASED || !s_huifang_tracking) {
        return;
    }
    s_huifang_tracking = false;

    lv_point_t rel;
    lv_indev_get_point(indev, &rel);
    const int dx = rel.x - s_huifang_press.x;
    const int dy = rel.y - s_huifang_press.y;

    if(dx <= -UI_SWIPE_COMMIT_DX && LV_ABS(dy) <= UI_SWIPE_MAX_ABS_DY && LV_ABS(dx) > LV_ABS(dy)) {
        printf("用户左滑返回主页\n");
        LOG_DEBUG("用户左滑返回主页");
        lv_async_call(ui_nav_replace_with_main_async, NULL);
    }
}
