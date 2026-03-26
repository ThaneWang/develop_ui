/**
 * @file ui.h
 * @brief UI 对外唯一头文件：分辨率宏、`UI_DEVELOP`、`my_ui_init()` 声明。应用层（如 main.c）只应包含本文件。
 */
#ifndef __UI_H_
#define __UI_H_

#include "lvgl/lvgl.h"

#define MY_SCREEN_WIDTH   800
#define MY_SCREEN_HEIGHT  600

#define UI_DEVELOP 1

/** 入口：创建初始界面（当前为相机主界面） */
void my_ui_init(void);

#endif /* __UI_H_ */
