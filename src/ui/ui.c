//new分支，用于迭代UI界面，可移植
#include "ui.h" // 包含 UI 头文件，声明接口和依赖
#include "../logging.h"


#if UI_DEVELOP
//ui迭代
/*创建主界面，包含相机主要功能：
1.左下角切换前后摄像头，右下角切换闪光灯模式，顶部左上角显示当前拍摄模式、存储状态，右上角显示电池电量 。
2.中间是取景区域，显示摄像头预览画面。
3.在取景区域上叠加一些辅助线（如九宫格线）和对焦框，帮助用户构图和对焦。
4.在取景区域下方放置一个大大的拍照按钮，用户点击后触发拍照功能。
5.在拍照按钮旁边放置一个小的相册按钮，用户点击后进入相册界面查看已拍照片。
6.在取景区域的右侧放置一个滑块，用户可以通过滑动调整曝光补偿值，实时影响预览画面亮度。
7.在取景区域的左侧放置一个水平滚动的模式选择器，用户可以左右滑动选择不同的拍摄模式（如照片、视频、专业模式等），当前选中的模式在顶部显示。
8.在取景区域的左上角偏中部放置一个小的缩略图，显示最近拍摄的照片，用户点击后进入相册界面查看。
9.在取景区域的左上角放置一个小的指示文本，显示当前摄像头状态（如对焦成功、对焦失败、正在对焦等），通过不同颜色或图标表示不同状态。
*/

// 全局变量用于状态管理
static lv_obj_t *mode_label; // 顶部显示当前模式的标签
static lv_obj_t *status_label; // 摄像头状态指示文本
static lv_obj_t *battery_label; // 电池电量标签
static lv_obj_t *storage_label; // 存储状态标签
static lv_obj_t *preview_area; // 取景区域
static lv_obj_t *focus_frame; // 对焦框
static lv_obj_t *thumbnail; // 缩略图
static lv_obj_t *exposure_slider; // 曝光滑块
static lv_obj_t *mode_roller; // 模式选择器

// 事件回调函数
static void switch_camera_cb(lv_event_t *e) {   // 1备注: 后续实现切换前后摄像头逻辑
    LV_UNUSED(e);
    LOG_INFO("Switch camera button clicked"); // 1备注: 后续实现切换前后摄像头逻辑
}

static void switch_flash_cb(lv_event_t *e) {   // 2备注: 后续实现切换闪光灯模式逻辑
    LV_UNUSED(e);
    LOG_INFO("Switch flash button clicked"); // 2备注: 后续实现切换闪光灯模式逻辑
}

static void capture_cb(lv_event_t *e) {   // 3备注: 后续实现拍照功能
    LV_UNUSED(e);
    LOG_INFO("Capture button clicked"); // 3备注: 后续实现拍照功能
}

static void gallery_cb(lv_event_t *e) {   // 4备注: 后续进入相册界面
    LV_UNUSED(e);
    LOG_INFO("Gallery button clicked"); // 4备注: 后续进入相册界面
}

static void exposure_slider_cb(lv_event_t *e) {   // 5备注: 后续调整曝光补偿值，影响预览画面亮度
    static int old_value = 0; // 静态变量记录旧值
    lv_obj_t *slider = lv_event_get_target(e);
    int new_value = lv_slider_get_value(slider);
    LOG_INFO("Exposure slider changed from %d to %d", old_value, new_value); // 5备注: 后续调整曝光补偿值，影响预览画面亮度
    old_value = new_value;
}

static void mode_roller_cb(lv_event_t *e) {   // 6备注: 后续更新拍摄模式
    static uint16_t old_sel = 0; // 静态变量记录旧选择
    lv_obj_t *roller = lv_event_get_target(e);
    uint16_t new_sel = lv_roller_get_selected(roller);
    static const char * const modes[] = {"Photo", "Video", "Pro Mode"}; // 6备注: 中文"照片、视频、专业模式"用英文代替，后续用图标
    uint16_t safe_new_sel = new_sel;
    uint16_t safe_old_sel = old_sel;
    if (safe_new_sel >= (uint16_t)LV_ARRAYLEN(modes)) safe_new_sel = 0;
    if (safe_old_sel >= (uint16_t)LV_ARRAYLEN(modes)) safe_old_sel = 0;

    lv_label_set_text(mode_label, modes[safe_new_sel]);
    LOG_INFO("Mode changed from %s to %s", modes[safe_old_sel], modes[safe_new_sel]); // 7备注: 后续更新拍摄模式
    old_sel = new_sel;
}

static void thumbnail_cb(lv_event_t *e) {   // 7备注: 后续进入相册界面查看最近照片
    LV_UNUSED(e);
    LOG_INFO("Thumbnail clicked"); // 8备注: 后续进入相册界面查看最近照片
}

static void focus_frame_cb(lv_event_t *e) {   // 8备注: 后续触发重新对焦逻辑
    LV_UNUSED(e);
    LOG_INFO("Focus frame clicked"); // 9备注: 后续触发重新对焦逻辑
}

static void preview_area_cb(lv_event_t *e) {   // 9备注: 后续实现触摸对焦逻辑
    LV_UNUSED(e);
    LOG_INFO("Preview area touched"); // 10备注: 后续实现触摸对焦逻辑
}

void my_ui_init(void) {                     //ui界面初始化函数
    LOG_INFO("my_ui_init (UI_DEVELOP) called");
    lv_obj_t *scr = lv_scr_act();

    // 设置屏幕背景色
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x111111), 0);

    // 顶部状态栏
    lv_obj_t *top_bar = lv_obj_create(scr);
    lv_obj_set_size(top_bar, MY_SCREEN_WIDTH, 50);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);

    // 左上角：拍摄模式和存储状态
    lv_obj_t *mode_storage_cont = lv_obj_create(top_bar);
    lv_obj_set_size(mode_storage_cont, 200, 40);
    lv_obj_align(mode_storage_cont, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_opa(mode_storage_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mode_storage_cont, 0, 0);
    lv_obj_set_flex_flow(mode_storage_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_storage_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    mode_label = lv_label_create(mode_storage_cont);
    lv_label_set_text(mode_label, "Photo"); // 备注: 显示当前拍摄模式，后续用图标表示
    lv_obj_set_style_text_color(mode_label, lv_color_white(), 0);

    storage_label = lv_label_create(mode_storage_cont);
    lv_label_set_text(storage_label, "Storage: OK"); // 备注: 显示存储状态，后续用图标表示
    lv_obj_set_style_text_color(storage_label, lv_color_white(), 0);
    lv_obj_set_style_margin_left(storage_label, 20, 0);

    // 右上角：电池电量
    battery_label = lv_label_create(top_bar);
    lv_label_set_text(battery_label, "Battery: 80%"); // 备注: 显示电池电量，后续用图标表示
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(battery_label, lv_color_white(), 0);

    // 取景区域
    preview_area = lv_obj_create(scr);
    lv_obj_set_size(preview_area, MY_SCREEN_WIDTH - 200, MY_SCREEN_HEIGHT - 200);
    lv_obj_align(preview_area, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_bg_color(preview_area, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_color(preview_area, lv_color_white(), 0);
    lv_obj_set_style_border_width(preview_area, 2, 0);
    lv_obj_add_event_cb(preview_area, preview_area_cb, LV_EVENT_CLICKED, NULL); // 备注: 取景区域点击事件，后续实现触摸对焦

    // 模拟预览画面（这里用一个简单的矩形表示）
    lv_obj_t *preview_image = lv_obj_create(preview_area);
    lv_obj_set_size(preview_image, lv_obj_get_width(preview_area) - 20, lv_obj_get_height(preview_area) - 20);
    lv_obj_center(preview_image);
    lv_obj_set_style_bg_color(preview_image, lv_color_hex(0x222222), 0);

    // 九宫格线
    int w = lv_obj_get_width(preview_area);
    int h = lv_obj_get_height(preview_area);

    // 水平线1
    static lv_point_precise_t hpoints1[2];
    hpoints1[0].x = 0; hpoints1[0].y = (lv_value_precise_t)h / 3.0f;
    hpoints1[1].x = w; hpoints1[1].y = (lv_value_precise_t)h / 3.0f;
    lv_obj_t *hline1 = lv_line_create(preview_area);
    lv_line_set_points(hline1, hpoints1, 2);
    lv_obj_set_style_line_color(hline1, lv_color_white(), 0);
    lv_obj_set_style_line_width(hline1, 1, 0);

    // 水平线2
    static lv_point_precise_t hpoints2[2];
    hpoints2[0].x = 0; hpoints2[0].y = (lv_value_precise_t)h * 2.0f / 3.0f;
    hpoints2[1].x = w; hpoints2[1].y = (lv_value_precise_t)h * 2.0f / 3.0f;
    lv_obj_t *hline2 = lv_line_create(preview_area);
    lv_line_set_points(hline2, hpoints2, 2);
    lv_obj_set_style_line_color(hline2, lv_color_white(), 0);
    lv_obj_set_style_line_width(hline2, 1, 0);

    // 垂直线1
    static lv_point_precise_t vpoints1[2];
    vpoints1[0].x = (lv_value_precise_t)w / 3.0f; vpoints1[0].y = 0;
    vpoints1[1].x = (lv_value_precise_t)w / 3.0f; vpoints1[1].y = h;
    lv_obj_t *vline1 = lv_line_create(preview_area);
    lv_line_set_points(vline1, vpoints1, 2);
    lv_obj_set_style_line_color(vline1, lv_color_white(), 0);
    lv_obj_set_style_line_width(vline1, 1, 0);

    // 垂直线2
    static lv_point_precise_t vpoints2[2];
    vpoints2[0].x = (lv_value_precise_t)w * 2.0f / 3.0f; vpoints2[0].y = 0;
    vpoints2[1].x = (lv_value_precise_t)w * 2.0f / 3.0f; vpoints2[1].y = h;
    lv_obj_t *vline2 = lv_line_create(preview_area);
    lv_line_set_points(vline2, vpoints2, 2);
    lv_obj_set_style_line_color(vline2, lv_color_white(), 0);
    lv_obj_set_style_line_width(vline2, 1, 0);

    // 对焦框（模拟一个矩形框）
    focus_frame = lv_obj_create(preview_area);
    lv_obj_set_size(focus_frame, 100, 100);
    lv_obj_align(focus_frame, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_border_color(focus_frame, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_border_width(focus_frame, 3, 0);
    lv_obj_set_style_bg_opa(focus_frame, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(focus_frame, focus_frame_cb, LV_EVENT_CLICKED, NULL); // 备注: 对焦框点击事件，后续实现对焦逻辑

    // 缩略图（左上角偏中部）
    thumbnail = lv_img_create(preview_area);
    lv_obj_set_size(thumbnail, 60, 60);
    lv_obj_align(thumbnail, LV_ALIGN_TOP_LEFT, 20, 50);
    // TODO: 设置缩略图图片源
    lv_obj_add_event_cb(thumbnail, thumbnail_cb, LV_EVENT_CLICKED, NULL);

    // 状态指示文本（左上角）
    status_label = lv_label_create(preview_area);
    lv_label_set_text(status_label, "Focus OK"); // 备注: 显示摄像头状态，后续用图标表示不同状态
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 20, 10);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);

    // 拍照按钮（下方大按钮）
    lv_obj_t *capture_btn = lv_btn_create(scr);
    lv_obj_set_size(capture_btn, 100, 100);
    lv_obj_align(capture_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_radius(capture_btn, 50, 0);
    lv_obj_set_style_bg_color(capture_btn, lv_color_white(), 0);
    lv_obj_add_event_cb(capture_btn, capture_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *capture_label = lv_label_create(capture_btn); // 备注: 拍照按钮，后续用图标
    lv_label_set_text(capture_label, "Capture");
    lv_obj_center(capture_label);

    // 相册按钮（拍照按钮旁边）
    lv_obj_t *gallery_btn = lv_btn_create(scr);
    lv_obj_set_size(gallery_btn, 50, 50);
    lv_obj_align(gallery_btn, LV_ALIGN_BOTTOM_MID, 80, -45);
    lv_obj_set_style_bg_color(gallery_btn, lv_color_hex(0x666666), 0);
    lv_obj_add_event_cb(gallery_btn, gallery_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *gallery_label = lv_label_create(gallery_btn); // 备注: 相册按钮，后续用图标
    lv_label_set_text(gallery_label, "Gallery");
    lv_obj_center(gallery_label);

    // 曝光滑块（右侧）
    exposure_slider = lv_slider_create(scr);
    lv_obj_set_size(exposure_slider, 20, lv_obj_get_height(preview_area));
    lv_obj_align(exposure_slider, LV_ALIGN_RIGHT_MID, -20, -50);
    lv_slider_set_range(exposure_slider, -100, 100);
    lv_slider_set_value(exposure_slider, 0, LV_ANIM_OFF);
    lv_obj_add_event_cb(exposure_slider, exposure_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 模式选择器（左侧，水平滚动）
    mode_roller = lv_roller_create(scr);
    lv_obj_set_size(mode_roller, 100, lv_obj_get_height(preview_area));
    lv_obj_align(mode_roller, LV_ALIGN_LEFT_MID, 20, -50);
    lv_roller_set_options(mode_roller, "Photo\nVideo\nPro Mode", LV_ROLLER_MODE_NORMAL); // 备注: 拍摄模式选择，后续用图标
    lv_obj_add_event_cb(mode_roller, mode_roller_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 左下角：切换摄像头按钮
    lv_obj_t *switch_camera_btn = lv_btn_create(scr);
    lv_obj_set_size(switch_camera_btn, 60, 60);
    lv_obj_align(switch_camera_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(switch_camera_btn, lv_color_hex(0x444444), 0);
    lv_obj_add_event_cb(switch_camera_btn, switch_camera_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *camera_label = lv_label_create(switch_camera_btn); // 备注: 切换摄像头按钮，后续用图标
    lv_label_set_text(camera_label, "Camera");
    lv_obj_center(camera_label);

    // 右下角：切换闪光灯按钮
    lv_obj_t *switch_flash_btn = lv_btn_create(scr);
    lv_obj_set_size(switch_flash_btn, 60, 60);
    lv_obj_align(switch_flash_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(switch_flash_btn, lv_color_hex(0x444444), 0);
    lv_obj_add_event_cb(switch_flash_btn, switch_flash_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *flash_label = lv_label_create(switch_flash_btn); // 备注: 切换闪光灯按钮，后续用图标
    lv_label_set_text(flash_label, "Flash");
    lv_obj_center(flash_label);

    LOG_INFO("Camera UI initialized");
}

#endif
