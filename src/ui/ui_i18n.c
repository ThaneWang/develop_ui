/**
 * @file ui_i18n.c
 * @brief 中英文词条表与 Label 绑定刷新。
 */
#include "ui_i18n.h"

#define UI_I18N_MAX_BINDINGS 128

typedef struct {
    lv_obj_t *label;
    ui_str_id_t id;
} ui_i18n_binding_t;

static const char *const s_zh[UI_STR_COUNT] = {
    [UI_STR_STATUS_ZONE] = "状态区",
    [UI_STR_RESERVED_ZONE] = "预留区",
    [UI_STR_ISP_ZONE] = "ISP参数显示区域",
    [UI_STR_VP_GEST_RIGHT] = "右滑",
    [UI_STR_VP_ACT_REPLAY] = "回放",
    [UI_STR_VP_GEST_LEFT] = "左滑",
    [UI_STR_VP_ACT_ISP] = "ISP",
    [UI_STR_VP_GEST_DOWN] = "下拉",
    [UI_STR_VP_ACT_CC] = "控制中心",
    [UI_STR_VP_GEST_UP] = "上滑",
    [UI_STR_VP_ACT_MODE] = "模式",
    [UI_STR_BOTTOM_MODE_DISP] = "模式显示区",
    [UI_STR_BOTTOM_MODE_PARAM] = "模式参数区",
    [UI_STR_BOTTOM_VIEW_CTRL] = "视图控制",
    [UI_STR_REPLAY_TITLE] = "回放页面",
    [UI_STR_REPLAY_BODY] = "回放(占位)\n左滑返回主页(向左拖动)",
    [UI_STR_CC_TITLE] = "控制中心",
    [UI_STR_CC_LANG_LABEL] = "语言",
    [UI_STR_CC_TILE_ROTATION] = "旋转方向锁定/开启",
    [UI_STR_CC_ROT_TITLE] = "旋转方向",
    [UI_STR_CC_ROT_ENABLE] = "启用屏幕旋转",
    [UI_STR_CC_ROT_ANGLE_0] = "0°（360°）",
    [UI_STR_CC_ROT_ANGLE_90] = "90°",
    [UI_STR_CC_ROT_ANGLE_180] = "180°",
    [UI_STR_CC_ROT_ANGLE_270] = "270°",
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
    [UI_STR_SETTINGS_POWER_SLEEP] = "开关机与休眠",
    [UI_STR_SETTINGS_BATTERY] = "电池与充电",
    [UI_STR_SETTINGS_THERMAL] = "温控与散热",
    [UI_STR_SETTINGS_SDCARD] = "存储卡",
    [UI_STR_SETTINGS_FIRMWARE] = "固件升级",
    [UI_STR_SETTINGS_LOG] = "日志与问题反馈",
    [UI_STR_SETTINGS_SECURITY] = "安全与权限",
    [UI_STR_SETTINGS_BT] = "蓝牙",
    [UI_STR_SETTINGS_WIFI] = "Wi-Fi",
    [UI_STR_SETTINGS_USB] = "USB 与传输",
    [UI_STR_SETTINGS_EXPORT] = "文件导出",
    [UI_STR_BT_PAGE_TITLE] = "蓝牙设置",
    [UI_STR_BT_MASTER_SWITCH] = "蓝牙",
    [UI_STR_BT_PAIR] = "配对",
    [UI_STR_BT_RECONNECT] = "重连",
    [UI_STR_BT_LOW_POWER] = "低功耗策略",
    [UI_STR_BT_WIFI_SET] = "WiFi SSID / 设置 WiFi 密码",
    [UI_STR_BT_STATUS_ON] = "蓝牙已开启",
    [UI_STR_MODE_TITLE] = "拍摄模式",
    [UI_STR_MODE_BODY] =
        "左右滑动切换模式\n点击屏幕侧边模式会先滚到中央再显示选中\n点击中央模式图标确认并返回主页\n下滑标题栏或点关闭不改动模式",
    [UI_STR_SHOOT_MODE_3DGS] = "3DGS 实景",
    [UI_STR_SHOOT_MODE_VIDEO] = "视频",
    [UI_STR_SHOOT_MODE_3DGS_V] = "3DGS + 视频",
    [UI_STR_SHOOT_MODE_STILL] = "图片",
    [UI_STR_STORAGE_FREE_FMT] = "剩余%uG",
};

static const char *const s_en[UI_STR_COUNT] = {
    [UI_STR_STATUS_ZONE] = "Status",
    [UI_STR_RESERVED_ZONE] = "Reserved",
    [UI_STR_ISP_ZONE] = "ISP parameters",
    [UI_STR_VP_GEST_RIGHT] = "Swipe right",
    [UI_STR_VP_ACT_REPLAY] = "Replay",
    [UI_STR_VP_GEST_LEFT] = "Swipe left",
    [UI_STR_VP_ACT_ISP] = "ISP",
    [UI_STR_VP_GEST_DOWN] = "Pull down",
    [UI_STR_VP_ACT_CC] = "Control center",
    [UI_STR_VP_GEST_UP] = "Swipe up",
    [UI_STR_VP_ACT_MODE] = "Mode",
    [UI_STR_BOTTOM_MODE_DISP] = "Mode display",
    [UI_STR_BOTTOM_MODE_PARAM] = "Mode settings",
    [UI_STR_BOTTOM_VIEW_CTRL] = "View control",
    [UI_STR_REPLAY_TITLE] = "Replay",
    [UI_STR_REPLAY_BODY] = "Replay (placeholder)\nSwipe left to home",
    [UI_STR_CC_TITLE] = "Control Center",
    [UI_STR_CC_LANG_LABEL] = "Language",
    [UI_STR_CC_TILE_ROTATION] = "Rotation lock",
    [UI_STR_CC_ROT_TITLE] = "Display rotation",
    [UI_STR_CC_ROT_ENABLE] = "Enable display rotation",
    [UI_STR_CC_ROT_ANGLE_0] = "0° (360°)",
    [UI_STR_CC_ROT_ANGLE_90] = "90°",
    [UI_STR_CC_ROT_ANGLE_180] = "180°",
    [UI_STR_CC_ROT_ANGLE_270] = "270°",
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
    [UI_STR_SETTINGS_POWER_SLEEP] = "Power & sleep",
    [UI_STR_SETTINGS_BATTERY] = "Battery & charging",
    [UI_STR_SETTINGS_THERMAL] = "Thermal",
    [UI_STR_SETTINGS_SDCARD] = "Memory card",
    [UI_STR_SETTINGS_FIRMWARE] = "Firmware update",
    [UI_STR_SETTINGS_LOG] = "Logs & feedback",
    [UI_STR_SETTINGS_SECURITY] = "Security",
    [UI_STR_SETTINGS_BT] = "Bluetooth",
    [UI_STR_SETTINGS_WIFI] = "Wi-Fi",
    [UI_STR_SETTINGS_USB] = "USB & transfer",
    [UI_STR_SETTINGS_EXPORT] = "File export",
    [UI_STR_BT_PAGE_TITLE] = "Bluetooth",
    [UI_STR_BT_MASTER_SWITCH] = "Bluetooth",
    [UI_STR_BT_PAIR] = "Pairing",
    [UI_STR_BT_RECONNECT] = "Reconnect",
    [UI_STR_BT_LOW_POWER] = "Low power",
    [UI_STR_BT_WIFI_SET] = "WiFi SSID / set Wi-Fi password",
    [UI_STR_BT_STATUS_ON] = "Bluetooth on",
    [UI_STR_MODE_TITLE] = "Shooting mode",
    [UI_STR_MODE_BODY] = "Swipe to change mode\n"
                          "Tap a side mode to scroll it to the center first, then it shows as selected\n"
                          "Tap the centered mode icon to apply and return home\n"
                          "Swipe down on the title bar or tap close without changing mode",
    [UI_STR_SHOOT_MODE_3DGS] = "3DGS",
    [UI_STR_SHOOT_MODE_VIDEO] = "Video",
    [UI_STR_SHOOT_MODE_3DGS_V] = "3DGS + Video",
    [UI_STR_SHOOT_MODE_STILL] = "Photo",
    [UI_STR_STORAGE_FREE_FMT] = "Free%uG",
};

static ui_i18n_binding_t s_bindings[UI_I18N_MAX_BINDINGS];
static uint8_t s_binding_n;
static ui_lang_t s_lang = UI_LANG_ZH;

/** 清空 Label 绑定表；切页重建 UI 前须调用。 */
void ui_i18n_reset_bindings(void)
{
    s_binding_n = 0;
}

/** 登记 Label 与词条 id，并立即设为当前语言文本；表满则静默忽略。 */
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

/** 返回当前语言下静态字符串指针；非法 id 返回空串。 */
const char *ui_i18n_str(ui_str_id_t id)
{
    if(id >= UI_STR_COUNT) {
        return "";
    }
    return s_lang == UI_LANG_ZH ? s_zh[id] : s_en[id];
}

/** 设置当前语种；非法值则忽略。 */
void ui_i18n_set_lang(ui_lang_t lang)
{
    if(lang != UI_LANG_ZH && lang != UI_LANG_EN) {
        return;
    }
    s_lang = lang;
}

/** 返回当前语种。 */
ui_lang_t ui_i18n_get_lang(void)
{
    return s_lang;
}

/** 按绑定表刷新全部 Label 文本，并对当前屏 `update_layout`。 */
void ui_i18n_refresh_all(void)
{
    for(uint8_t i = 0; i < s_binding_n; i++) {
        lv_obj_t *lb = s_bindings[i].label;
        if(lb != NULL && lv_obj_is_valid(lb)) {
            lv_label_set_text(lb, ui_i18n_str(s_bindings[i].id));
        }
    }
    lv_obj_t *scr = lv_scr_act();
    if(scr != NULL && lv_obj_is_valid(scr)) {
        lv_obj_update_layout(scr);
    }
}

