/**
 * @file ui_font.c
 * @brief 中文字体：静态字库 `noto_sans_sc_16`（`LV_USE_NOTO_SANS_SC_16_STATIC`）。
 */
#include "ui_font.h"
#include "../logging.h"
#include "lvgl/lvgl.h"

#if LV_USE_NOTO_SANS_SC_16_STATIC

/** 可写副本：保证 ASCII 标点等走 `fallback`（Montserrat）；生成体里的 `.fallback` 若因工具链/宏未生效会缺字占位 */
static lv_font_t s_cjk;
static bool s_cjk_ready;

static void ui_font_cjk_prepare(void)
{
    if(s_cjk_ready) {
        return;
    }
    lv_memcpy(&s_cjk, &noto_sans_sc_16, sizeof(lv_font_t));
    s_cjk.fallback = &lv_font_montserrat_16;
    s_cjk.kerning = LV_FONT_KERNING_NONE;
    s_cjk_ready = true;
}

lv_result_t ui_font_init(void)
{
    ui_font_cjk_prepare();
    LOG_INFO("CJK font: static noto_sans_sc_16 (16 px, bpp 4), fallback lv_font_montserrat_16");
    return LV_RESULT_OK;
}

void ui_font_uninit(void) {}

const lv_font_t *ui_font_cjk(void)
{
    ui_font_cjk_prepare();
    return &s_cjk;
}

#else

lv_result_t ui_font_init(void)
{
    return LV_RESULT_OK;
}

void ui_font_uninit(void) {}

const lv_font_t *ui_font_cjk(void)
{
    return NULL;
}

#endif
