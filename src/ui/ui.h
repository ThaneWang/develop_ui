#ifndef __UI_H_
#define __UI_H_

#include "lvgl/lvgl.h"
#include "stdio.h"

#define MY_SCREEN_WIDTH   800
#define MY_SCREEN_HEIGHT  600

#define UI_DEVELOP 0
#define DEEPSEEK 1
#define AI 0

void my_ui_init(void);

#if UI_DEVELOP
void switch_camera_cb(lv_event_t *e);       // 1备注: 后续实现切换前后摄像头逻辑
void switch_flash_cb(lv_event_t *e);        // 2备注: 后续实现切换闪光灯模式逻辑
void capture_cb(lv_event_t *e);             // 3备注: 后续实现拍照功能
void gallery_cb(lv_event_t *e);             // 4备注: 后续进入相册界面
void exposure_slider_cb(lv_event_t *e);     // 5备注: 后续调整曝光补偿值，影响预览画面亮度
void mode_roller_cb(lv_event_t *e);         // 6备注: 后续更新拍摄模式
void thumbnail_cb(lv_event_t *e);           // 7备注: 后续进入相册界面查看最近照片
void focus_frame_cb(lv_event_t *e);         // 8备注: 后续触发重新对焦逻辑
void preview_area_cb(lv_event_t *e);        // 9备注: 后续实现触摸对焦逻辑
#endif

#if DEEPSEEK
void item_click_cb(lv_event_t *e);
#endif

#if AI
void checkbox_event_cb(lv_event_t * e);
#endif

#endif /* __UI_H_ */
