/**
 * @file ui_page_replay.h
 * @brief 声明回放页构建函数 `ui_page_replay_create`。
 */
#ifndef UI_PAGE_REPLAY_H
#define UI_PAGE_REPLAY_H

#include "lvgl/lvgl.h"

/** 构建回放页（暂停态为默认；点「播放」进入播放态 UI，视频解码留 `ui_replay_player_load_request`）。 */
void ui_page_replay_create(lv_obj_t *scr);

/**
 * 占位接口：后续在此根据 `uri` 打开文件/流并绑定画面层；当前不加载视频，仅打日志。
 * @param uri 可为 `NULL`；实现阶段可为路径或 URL。
 */
void ui_replay_player_load_request(const char *uri);

/** 将暂停态底栏「当前模式」行与 `ui_app_get_shoot_mode()` 对齐（进入回放页时由 `create` 调用）。 */
void ui_page_replay_sync_shoot_mode_display(void);

#endif
