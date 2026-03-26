/**
 * @file ui_i18n.c
 * @brief 中英文词条表与 Label 绑定刷新。
 */
#include "ui_i18n.h"

#define UI_I18N_MAX_BINDINGS 96

typedef struct {
    lv_obj_t *label;
    ui_str_id_t id;
} ui_i18n_binding_t;

static const char *const s_zh[UI_STR_COUNT] = {
    [UI_STR_NOTE] = "预览区:右滑回放/左滑ISP/下拉控制中心/上滑模式",
    [UI_STR_STATUS_ZONE] = "状态区",
    [UI_STR_RESERVED_ZONE] = "预留区",
    [UI_STR_ISP_ZONE] = "ISP参数显示区域",
    [UI_STR_SWIPE_RIGHT_REPLAY] = "用户右滑识别(向右拖动)",
    [UI_STR_BOTTOM_MODE_DISP] = "模式显示区",
    [UI_STR_BOTTOM_MODE_PARAM] = "模式参数区",
    [UI_STR_BOTTOM_VIEW_CTRL] = "视图控制",
    [UI_STR_REPLAY_TITLE] = "回放页面",
    [UI_STR_REPLAY_BODY] = "回放(占位)\n左滑返回主页(向左拖动)",
    [UI_STR_CC_TITLE] = "控制中心",
    [UI_STR_CC_LANG_LABEL] = "语言",
    [UI_STR_CC_TILE_ROTATION] = "旋转方向锁定/开启",
    [UI_STR_CC_TILE_LOCK_SCREEN] = "锁定屏幕",
    [UI_STR_CC_TILE_VOICE] = "语音控制",
    [UI_STR_CC_TILE_SYSTEM] = "系统设置",
    [UI_STR_CC_TILE_BRIGHTNESS] = "亮度",
    [UI_STR_CC_TILE_VOLUME] = "音量",
    [UI_STR_CC_TILE_QUICK] = "快切",
    [UI_STR_CC_TILE_THEME] = "主题颜色",
    [UI_STR_SETTINGS_TITLE] = "系统设置",
    [UI_STR_SETTINGS_BACK] = "返回",
    [UI_STR_SETTINGS_DATETIME] = "时间日期",
    [UI_STR_SETTINGS_FACTORY] = "恢复出厂设置",
    [UI_STR_SETTINGS_DEVICE] = "设备信息",
    [UI_STR_MODE_TITLE] = "模式参数",
    [UI_STR_MODE_BODY] = "模式参数控制(占位)\n下滑或点关闭返回",
};

static const char *const s_en[UI_STR_COUNT] = {
    [UI_STR_NOTE] = "Viewport: R=replay L=ISP down=CC up=mode",
    [UI_STR_STATUS_ZONE] = "Status",
    [UI_STR_RESERVED_ZONE] = "Reserved",
    [UI_STR_ISP_ZONE] = "ISP parameters",
    [UI_STR_SWIPE_RIGHT_REPLAY] = "Swipe right for replay",
    [UI_STR_BOTTOM_MODE_DISP] = "Mode display",
    [UI_STR_BOTTOM_MODE_PARAM] = "Mode settings",
    [UI_STR_BOTTOM_VIEW_CTRL] = "View control",
    [UI_STR_REPLAY_TITLE] = "Replay",
    [UI_STR_REPLAY_BODY] = "Replay (placeholder)\nSwipe left to home",
    [UI_STR_CC_TITLE] = "Control Center",
    [UI_STR_CC_LANG_LABEL] = "Language",
    [UI_STR_CC_TILE_ROTATION] = "Rotation lock",
    [UI_STR_CC_TILE_LOCK_SCREEN] = "Lock screen",
    [UI_STR_CC_TILE_VOICE] = "Voice control",
    [UI_STR_CC_TILE_SYSTEM] = "System settings",
    [UI_STR_CC_TILE_BRIGHTNESS] = "Brightness",
    [UI_STR_CC_TILE_VOLUME] = "Volume",
    [UI_STR_CC_TILE_QUICK] = "Quick switch",
    [UI_STR_CC_TILE_THEME] = "Theme color",
    [UI_STR_SETTINGS_TITLE] = "System settings",
    [UI_STR_SETTINGS_BACK] = "Back",
    [UI_STR_SETTINGS_DATETIME] = "Date & time",
    [UI_STR_SETTINGS_FACTORY] = "Factory reset",
    [UI_STR_SETTINGS_DEVICE] = "Device info",
    [UI_STR_MODE_TITLE] = "Mode parameters",
    [UI_STR_MODE_BODY] = "Mode control (placeholder)\nSwipe down or tap close",
};

static ui_i18n_binding_t s_bindings[UI_I18N_MAX_BINDINGS];
static uint8_t s_binding_n;
static ui_lang_t s_lang = UI_LANG_ZH;

void ui_i18n_reset_bindings(void)
{
    s_binding_n = 0;
}

void ui_i18n_bind_label(lv_obj_t *label, ui_str_id_t id)
{
    if(label == NULL || id >= UI_STR_COUNT) {
        return;
    }
    if(s_binding_n >= UI_I18N_MAX_BINDINGS) {
        return;
    }
    s_bindings[s_binding_n].label = label;
    s_bindings[s_binding_n].id = id;
    s_binding_n++;
    lv_label_set_text(label, ui_i18n_str(id));
}

const char *ui_i18n_str(ui_str_id_t id)
{
    if(id >= UI_STR_COUNT) {
        return "";
    }
    return s_lang == UI_LANG_ZH ? s_zh[id] : s_en[id];
}

void ui_i18n_set_lang(ui_lang_t lang)
{
    if(lang != UI_LANG_ZH && lang != UI_LANG_EN) {
        return;
    }
    s_lang = lang;
}

ui_lang_t ui_i18n_get_lang(void)
{
    return s_lang;
}

void ui_i18n_refresh_all(void)
{
    for(uint8_t i = 0; i < s_binding_n; i++) {
        lv_obj_t *lb = s_bindings[i].label;
        if(lb != NULL && lv_obj_is_valid(lb)) {
            lv_label_set_text(lb, ui_i18n_str(s_bindings[i].id));
        }
    }
}

