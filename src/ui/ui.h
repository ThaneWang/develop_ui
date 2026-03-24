#ifndef __UI_H_
#define __UI_H_

#include "lvgl/lvgl.h"
#include "stdio.h"

#define MY_SCREEN_WIDTH   800
#define MY_SCREEN_HEIGHT  600

#define PRECTICE 1
#define DEEPSEEK 0
#define AI 0

void my_ui_init(void);

#if PRECTICE
void slider_event_cb(lv_event_t * e);
#endif

#if DEEPSEEK
void item_click_cb(lv_event_t *e);
#endif

#if AI
void checkbox_event_cb(lv_event_t * e);
#endif

#endif
