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

/** 从静态 `noto_sans_sc_16` 拷贝到可写副本并设置 Montserrat fallback（仅首次）。 */
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

/** 准备 CJK 字体并打日志；须在 `lv_init()` 之后调用。 */
lv_result_t ui_font_init(void)
{
    ui_font_cjk_prepare();
    LOG_INFO("CJK font: static noto_sans_sc_16 (16 px, bpp 4), fallback lv_font_montserrat_16");
    return LV_RESULT_OK;
}

/** 静态方案下无动态释放需求，空实现。 */
void ui_font_uninit(void) {}

/** 返回可写副本 `s_cjk`（含 fallback），首次调用时完成准备。 */
const lv_font_t *ui_font_cjk(void)
{
    ui_font_cjk_prepare();
    return &s_cjk;
}

#else

/** 未启用 `LV_USE_NOTO_SANS_SC_16_STATIC`：直接成功。 */
lv_result_t ui_font_init(void)
{
    return LV_RESULT_OK;
}

/** 无静态 CJK 字库：空实现。 */
void ui_font_uninit(void) {}

/** 无静态 CJK 字库：返回 NULL。 */
const lv_font_t *ui_font_cjk(void)
{
    return NULL;
}

#endif
