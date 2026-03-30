/**
 * @file ui_page_main.h
 * @brief 声明主界面构建函数 `ui_page_main_create`。
 */
#ifndef UI_PAGE_MAIN_H
#define UI_PAGE_MAIN_H

#include "lvgl/lvgl.h"

/** 构建相机主界面（预览/状态/底栏等） */
void ui_page_main_create(lv_obj_t *scr);

/** 刷新顶栏存储/电量/模式/蓝牙等（语言切换、蓝牙开关后调用） */
void ui_page_main_refresh_chrome(void);

#endif
