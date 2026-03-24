/**
 * @file main.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#ifndef _DEFAULT_SOURCE
  #define _DEFAULT_SOURCE /* needed for usleep() */
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef _MSC_VER
  #include <Windows.h>
#else
  #include <unistd.h>
  #include <pthread.h>
#endif
#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include <SDL.h>

#include "hal/hal.h"

#include  "ui/ui.h"
#include "logging.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
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

  /*Initialize SDL*/
  if(SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  /*Initialize LVGL*/
  lv_init();
  LOG_INFO("LVGL initialized");

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
    /* Handle SDL events */
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT) {
        return 0;
      }
    }

    /* Periodically call the lv_task handler.
     * It could be done in a timer interrupt or an OS task too.*/
    uint32_t sleep_time_ms = lv_timer_handler();
    if(sleep_time_ms == LV_NO_TIMER_READY){
	sleep_time_ms =  LV_DEF_REFR_PERIOD;
    }

    if(sleep_time_ms > 50) sleep_time_ms = 50; /*Limit the sleep time to 50 ms*/
    if(sleep_time_ms > 0)
      SDL_Delay(sleep_time_ms);   /*Sleep until the next event happens*/
  }

  return 0;
}


#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/
