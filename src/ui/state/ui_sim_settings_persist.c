/**
 * @file ui_sim_settings_persist.c
 * @brief 模拟器设置文件：`SMIS` + `version` + 载荷。
 * - **v1**：蓝牙、低功耗 + 6 字节预留（旧版）。
 * - **v2**：蓝牙、低功耗、语种、亮度、音量、旋转开关、旋转象限 + 9 字节预留。
 */
#include "ui_sim_settings_persist.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ui_bt_state.h"
#include "ui_cc_settings_state.h"
#include "ui_i18n.h"

#define UI_SIM_SETTINGS_VER_V1 1u
#define UI_SIM_SETTINGS_VER_V2 2u

/** 文件头四字节 ASCII `SMIS`（本机小端 `uint32_t` 值）。 */
#define UI_SIM_SETTINGS_MAGIC 0x53494d53u

#define PAYLOAD_V2_BYTES 16u

void ui_sim_settings_boot_load(void)
{
    FILE *fp = fopen(UI_SIM_SETTINGS_PERSIST_FILE, "rb");
    if(fp == NULL) {
        return;
    }
    uint32_t magic = 0;
    uint32_t ver = 0;
    if(fread(&magic, sizeof magic, 1, fp) != 1u) {
        fclose(fp);
        return;
    }
    if(fread(&ver, sizeof ver, 1, fp) != 1u) {
        fclose(fp);
        return;
    }
    if(magic != UI_SIM_SETTINGS_MAGIC) {
        fclose(fp);
        return;
    }

    if(ver == UI_SIM_SETTINGS_VER_V1) {
        unsigned char bt = 0;
        unsigned char lp = 1;
        unsigned char r6[6];
        if(fread(&bt, 1, 1, fp) != 1u || fread(&lp, 1, 1, fp) != 1u) {
            fclose(fp);
            return;
        }
        (void)fread(r6, 1, sizeof r6, fp);
        fclose(fp);
        ui_bt_state_apply_from_persist(bt != 0, lp != 0);
        ui_cc_settings_boot_restore(50u, 50u, 0u, 0u);
        return;
    }

    if(ver == UI_SIM_SETTINGS_VER_V2) {
        unsigned char p[PAYLOAD_V2_BYTES];
        if(fread(p, 1, PAYLOAD_V2_BYTES, fp) != (size_t)PAYLOAD_V2_BYTES) {
            fclose(fp);
            return;
        }
        fclose(fp);
        ui_bt_state_apply_from_persist(p[0] != 0, p[1] != 0);
        ui_i18n_set_lang_without_persist(p[2] == 0u ? UI_LANG_ZH : UI_LANG_EN);
        ui_cc_settings_boot_restore(p[3], p[4], p[5], p[6]);
        return;
    }

    fclose(fp);
}

void ui_sim_settings_persist_save(void)
{
    FILE *fp = fopen(UI_SIM_SETTINGS_PERSIST_FILE, "wb");
    if(fp == NULL) {
        return;
    }
    const uint32_t magic = UI_SIM_SETTINGS_MAGIC;
    const uint32_t ver = UI_SIM_SETTINGS_VER_V2;
    unsigned char p[PAYLOAD_V2_BYTES];
    memset(p, 0, sizeof p);
    p[0] = ui_bt_is_enabled() ? 1u : 0u;
    p[1] = ui_bt_low_power_is_on() ? 1u : 0u;
    p[2] = (ui_i18n_get_lang() == UI_LANG_ZH) ? 0u : 1u;
    p[3] = ui_cc_settings_get_brightness();
    p[4] = ui_cc_settings_get_volume();
    p[5] = ui_cc_settings_get_rotation_enabled() ? 1u : 0u;
    p[6] = ui_cc_settings_get_rotation_index();

    (void)fwrite(&magic, sizeof magic, 1, fp);
    (void)fwrite(&ver, sizeof ver, 1, fp);
    (void)fwrite(p, 1, sizeof p, fp);
    fclose(fp);
}
