/**
 * @file main.c
 * @brief PC 模拟器入口：SDL → LVGL → HAL → 自定义 UI 主循环。
 *
 * 与 UI 的依赖关系（调用顺序不可颠倒）：
 * 1. `lv_init()` → `ui_font_init()`（静态 Noto SC 16）→ HAL → `my_ui_init()`。
 * 2. `#include "ui/ui.h"`：声明 `my_ui_init()`，并提供分辨率宏给 HAL。
 * 3. `#include "ui/ui_font.h"` + `ui_font_init()`：在 `lv_init()` 之后完成中文字体就绪。
 * 4. `#include "logging.h"`：与 UI 模块共用日志输出。
 * 5. 各 `src/ui/*.c` 由 CMake 链入 `main`；除 `ui/ui.h`、`ui/ui_font.h` 外不必包含其它 UI 内部头文件。
 */

/*********************
 *      INCLUDES
 *********************/
#ifndef _DEFAULT_SOURCE
  #define _DEFAULT_SOURCE /* needed for usleep() */
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
#if !defined(_MSC_VER)
#include <unistd.h>
#include <pthread.h>
#endif
#include "lvgl/lvgl.h"
/*#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"*/
#include <SDL.h>

#include "hal/hal.h"

#include "ui/ui.h"
#include "ui/ui_font.h"
#include "logging.h"

/*********************
 *      DEFINES
 *********************/
 /*      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#if LV_USE_OS != LV_OS_FREERTOS



int main(int argc, char **argv)
{
  (void)argc; /*Unused*/
  (void)argv; /*Unused*/

#ifdef _WIN32
  /* 源码为 UTF-8 时，避免 printf / LOG_* 中文在默认 GBK 控制台下乱码 */
  SetConsoleOutputCP(65001);
  SetConsoleCP(65001);
#endif

  /*Initialize SDL*/
  if(SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  /*Initialize LVGL*/
  lv_init();
  LOG_INFO("LVGL initialized");

  if(ui_font_init() != LV_RESULT_OK) {
    LOG_WARN("ui_font_init failed, Chinese UI may use LV_FONT_DEFAULT");
  }

  /*Initialize the HAL (display, input devices, tick) for LVGL*/
  sdl_hal_init(MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);
  LOG_INFO("HAL initialized: %dx%d", MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT);

  /* Run the default demo */
  /* To try a different demo or example, replace this with one of: */
  //lv_demo_benchmark();
   //lv_demo_stress();
 // lv_example_label_1();
  /* - etc. */
 //lv_demo_widgets();
  //lv_demo_music();*/

  //初始化自定义ui
   LOG_INFO("Initializing UI...");
   printf("Initializing UI...\n");
   my_ui_init();
   LOG_INFO("UI initialized");
   printf("UI initialized\n");

  while(1) {
    uint32_t sleep_time_ms = lv_timer_handler();
    if(sleep_time_ms == LV_NO_TIMER_READY) {
      sleep_time_ms = LV_DEF_REFR_PERIOD;
    }

#ifdef _MSC_VER
    Sleep(sleep_time_ms);
#else
    usleep(sleep_time_ms * 1000);
#endif
  }

  return 0;
}


#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/
