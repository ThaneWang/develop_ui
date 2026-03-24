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
void switch_camera_cb(lv_event_t *e) {
    LOG_INFO("Switch camera button clicked"); // 1备注: 后续实现切换前后摄像头逻辑
}

void switch_flash_cb(lv_event_t *e) {
    LOG_INFO("Switch flash button clicked"); // 2备注: 后续实现切换闪光灯模式逻辑
}

void capture_cb(lv_event_t *e) {
    LOG_INFO("Capture button clicked"); // 3备注: 后续实现拍照功能
}

void gallery_cb(lv_event_t *e) {
    LOG_INFO("Gallery button clicked"); // 备注: 后续进入相册界面
}

void exposure_slider_cb(lv_event_t *e) {
    static int old_value = 0; // 静态变量记录旧值
    int new_value = lv_slider_get_value(exposure_slider);
    LOG_INFO("Exposure slider changed from %d to %d", old_value, new_value); // 5备注: 后续调整曝光补偿值，影响预览画面亮度
    old_value = new_value;
}

void mode_roller_cb(lv_event_t *e) {
    static uint16_t old_sel = 0; // 静态变量记录旧选择
    uint16_t new_sel = lv_roller_get_selected(mode_roller);
    const char *modes[] = {"Photo", "Video", "Pro Mode"}; // 备注: 中文"照片、视频、专业模式"用英文代替，后续用图标
    lv_label_set_text(mode_label, modes[new_sel]);
    LOG_INFO("Mode changed from %s to %s", modes[old_sel], modes[new_sel]); // 6备注: 后续更新拍摄模式
    old_sel = new_sel;
}

void thumbnail_cb(lv_event_t *e) {
    LOG_INFO("Thumbnail clicked"); // 7备注: 后续进入相册界面查看最近照片
}

void focus_frame_cb(lv_event_t *e) {
    LOG_INFO("Focus frame clicked"); // 8备注: 后续触发重新对焦逻辑
}

void preview_area_cb(lv_event_t *e) {
    LOG_INFO("Preview area touched"); // 备注: 后续实现触摸对焦逻辑
}

void my_ui_init(void) {
    printf("my_ui_init (UI_DEVELOP) called\n");
    LOG_INFO("my_ui_init (UI_DEVELOP) called");
    lv_obj_t *scr = lv_scr_act();

    // 设置屏幕背景色
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

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
    hpoints1[0].x = 0; hpoints1[0].y = h/3;
    hpoints1[1].x = w; hpoints1[1].y = h/3;
    lv_obj_t *hline1 = lv_line_create(preview_area);
    lv_line_set_points(hline1, hpoints1, 2);
    lv_obj_set_style_line_color(hline1, lv_color_white(), 0);
    lv_obj_set_style_line_width(hline1, 1, 0);

    // 水平线2
    static lv_point_precise_t hpoints2[2];
    hpoints2[0].x = 0; hpoints2[0].y = h*2/3;
    hpoints2[1].x = w; hpoints2[1].y = h*2/3;
    lv_obj_t *hline2 = lv_line_create(preview_area);
    lv_line_set_points(hline2, hpoints2, 2);
    lv_obj_set_style_line_color(hline2, lv_color_white(), 0);
    lv_obj_set_style_line_width(hline2, 1, 0);

    // 垂直线1
    static lv_point_precise_t vpoints1[2];
    vpoints1[0].x = w/3; vpoints1[0].y = 0;
    vpoints1[1].x = w/3; vpoints1[1].y = h;
    lv_obj_t *vline1 = lv_line_create(preview_area);
    lv_line_set_points(vline1, vpoints1, 2);
    lv_obj_set_style_line_color(vline1, lv_color_white(), 0);
    lv_obj_set_style_line_width(vline1, 1, 0);

    // 垂直线2
    static lv_point_precise_t vpoints2[2];
    vpoints2[0].x = w*2/3; vpoints2[0].y = 0;
    vpoints2[1].x = w*2/3; vpoints2[1].y = h;
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

#if AI // AI 分支编译开关（仅在定义 AI 时编译此部分）
#define NUM_ITEMS 4 // 定义菜单项数量为 4

static const char * item_names[NUM_ITEMS] = {"T-Shirt", "Jeans", "Jacket", "Hat"}; // 商品名称数组
static const int item_prices[NUM_ITEMS] = {100, 200, 500, 50}; // 商品价格数组（整数，单位美元）
static lv_obj_t * item_cbs[NUM_ITEMS]; // 存放每个商品复选框对象的数组
static lv_obj_t * total_label; // 指向显示总价的标签对象

static void update_total(void) // 计算并更新总价的函数
{
    int total = 0; // 总价累加器，初始为0
    for(int i = 0; i < NUM_ITEMS; i++) { // 遍历所有商品
        if(item_cbs[i] && lv_obj_has_state(item_cbs[i], LV_STATE_CHECKED)) { // 若复选框存在且被选中
            total += item_prices[i]; // 累加对应价格
        }
    }
    lv_label_set_text_fmt(total_label, "Total: $%d", total); // 使用格式化字符串更新总价标签
    LOG_INFO("update_total: total=%d", total);
}

void checkbox_event_cb(lv_event_t * e) // 复选框事件回调（值改变时调用）
{
    LV_UNUSED(e); // 避免未使用参数的警告
    update_total(); // 当复选框值变化时重新计算总价
    LOG_INFO("checkbox_event_cb: total updated");
}

void my_ui_init(void) // 对外初始化函数，创建并布局所有 UI 元素
{
    lv_obj_t * scr = lv_scr_act(); // 获取当前活动屏幕对象

    /* Main container: full screen flex column */
    lv_obj_t * main = lv_obj_create(scr); // 在屏幕上创建主容器
    lv_obj_set_size(main, MY_SCREEN_WIDTH, MY_SCREEN_HEIGHT); // 将主容器设置为全屏大小
    lv_obj_set_style_pad_all(main, 12, 0); // 为主容器设置内边距
    lv_obj_set_flex_flow(main, LV_FLEX_FLOW_COLUMN); // 使用 Flex 布局，垂直排列子元素
    lv_obj_set_flex_align(main, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); // 主轴两端对齐，交叉轴居中

    /* Title */
    lv_obj_t * title = lv_label_create(main); // 在主容器中创建标题标签
    lv_label_set_text(title, "Clothing Menu"); // 设置标题文本为英文“Clothing Menu”
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0); // 为标题设置较大的 Montserrat 字体
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0); // 标题文字居中对齐
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12); // 将标题放到主容器顶部中间并向下偏移 12 像素

    /* List container (scrollable) */
    lv_obj_t * list = lv_obj_create(main); // 创建用于放置复选框列表的容器
    lv_obj_set_width(list, MY_SCREEN_WIDTH - 48); // 设置列表容器宽度，左右留出间距
    lv_obj_set_height(list, MY_SCREEN_HEIGHT - 180); // 设置列表容器高度，保留顶部和底部空间
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN); // 列表内部按列排列子项
    lv_obj_set_style_pad_row(list, 8, 0); // 设置行间距为 8
    lv_obj_set_scroll_dir(list, LV_DIR_VER); // 启用垂直滚动
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO); // 自动显示滚动条
    lv_obj_set_style_border_width(list, 0, 0); // 取消边框
    lv_obj_set_style_radius(list, 8, 0); // 设置圆角半径
    lv_obj_set_style_bg_color(list, lv_color_hex(0xF5F7FA), 0); // 设置背景色为浅灰蓝
    lv_obj_set_style_pad_all(list, 8, 0); // 为列表容器设置内边距

    /* Create checkboxes */
    for(int i = 0; i < NUM_ITEMS; i++) { // 遍历创建每个商品的复选框项
        char buf[64]; // 文本缓冲区，用于格式化标签文本
        snprintf(buf, sizeof(buf), "%s  $%d", item_names[i], item_prices[i]); // 将名称和价格格式化为字符串
        item_cbs[i] = lv_checkbox_create(list); // 在列表容器中创建复选框对象
        lv_checkbox_set_text(item_cbs[i], buf); // 设置复选框文本为名称和价格
        lv_obj_set_width(item_cbs[i], lv_obj_get_width(list) - 16); // 将复选框宽度设置为列表宽度减去边距
        lv_obj_set_style_text_font(item_cbs[i], &lv_font_montserrat_16, 0); // 为复选框文本设置合适的字体大小
        lv_obj_add_event_cb(item_cbs[i], checkbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL); // 为复选框添加值改变回调
    }

    /* Bottom total label */
    total_label = lv_label_create(main); // 在主容器中创建用于显示总价的标签
    lv_label_set_text(total_label, "Total: $0"); // 初始化总价文本为 $0
    lv_obj_set_style_text_align(total_label, LV_TEXT_ALIGN_CENTER, 0); // 总价文本居中对齐
    lv_obj_set_style_text_font(total_label, &lv_font_montserrat_18, 0); // 为总价设置字体
    lv_obj_set_style_text_color(total_label, lv_color_hex(0x1F2937), 0); // 设置总价文字颜色
    lv_obj_align(total_label, LV_ALIGN_BOTTOM_MID, 0, -12); // 将总价标签对齐到底部中间并上移 12 像素

    /* Initialize total display */
    update_total(); // 启动时计算并显示初始总价
    LOG_INFO("my_ui_init (AI) completed");
}
#endif

#if DEEPSEEK

// 衣服数据结构
typedef struct {
    const char *name;
    double price;
} clothing_item;

// 衣服列表
static clothing_item items[] = {
    {"T-Shirt",  20.0},
    {"Jeans",    40.0},
    {"Jacket",   60.0},
    {"Sweater",  35.0},
    {"Hat",      15.0}
};
// 定义衣服数量宏（编译时常量）
#define NUM_ITEMS (sizeof(items) / sizeof(items[0]))

// 每个衣服项关联的控件和数据
typedef struct {
    lv_obj_t *checkbox;     // 复选框对象
    lv_obj_t *quantity_lbl; // 数量显示标签
    int quantity;           // 当前数量（0~99）
    bool selected;          // 是否选中
} item_ctrl_t;

static item_ctrl_t ctrl[NUM_ITEMS];   // 存储每个衣服的控件句柄和状态
static lv_obj_t *total_label;         // 底部总价标签

// 更新总价（遍历所有衣服，累加选中的数量*价格）
static void update_total(void) {
    double total = 0;
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (ctrl[i].selected) {
            total += items[i].price * ctrl[i].quantity;
        }
    }
    char buf[64];
    lv_snprintf(buf, sizeof(buf), "Total: $%.2f", total);
    lv_label_set_text(total_label, buf);
}

// 数量增加回调
static void inc_quantity_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn); // 衣服索引
    if (idx >= 0 && idx < NUM_ITEMS) {
        if (ctrl[idx].quantity < 99) {
            ctrl[idx].quantity++;
            char qty_buf[4];
            lv_snprintf(qty_buf, sizeof(qty_buf), "%d", ctrl[idx].quantity);
            lv_label_set_text(ctrl[idx].quantity_lbl, qty_buf);
            update_total();  // 数量变化，重新计算总价
            LOG_INFO("inc_quantity_cb: idx=%d quantity=%d", idx, ctrl[idx].quantity);
        }
    }
}

// 数量减少回调
static void dec_quantity_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (idx >= 0 && idx < NUM_ITEMS) {
        if (ctrl[idx].quantity > 0) {
            ctrl[idx].quantity--;
            char qty_buf[4];
            lv_snprintf(qty_buf, sizeof(qty_buf), "%d", ctrl[idx].quantity);
            lv_label_set_text(ctrl[idx].quantity_lbl, qty_buf);
            update_total();
                LOG_INFO("dec_quantity_cb: idx=%d quantity=%d", idx, ctrl[idx].quantity);
        }
    }
}

// 复选框状态改变回调
static void checkbox_cb(lv_event_t *e) {
    lv_obj_t *cb = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(cb);
    if (idx >= 0 && idx < NUM_ITEMS) {
        ctrl[idx].selected = lv_obj_get_state(cb) & LV_STATE_CHECKED;
        update_total();
        LOG_INFO("checkbox_cb: idx=%d selected=%d", idx, ctrl[idx].selected);
    }
}

void my_ui_init(void) {
    lv_obj_t *scr = lv_scr_act();

    // 设置背景色
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF5F5F5), 0);

    // 标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Clothing Menu");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // 滚动容器，用于放置所有衣服项
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, lv_pct(90), lv_pct(70));
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

    // 为每个衣服创建一行（水平布局）
    for (int i = 0; i < NUM_ITEMS; i++) {
        // 创建行容器
        lv_obj_t *row = lv_obj_create(cont);
        lv_obj_set_size(row, lv_pct(100), 70);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 5, 0);

        // 复选框
        lv_obj_t *cb = lv_checkbox_create(row);
        lv_checkbox_set_text(cb, items[i].name);
        lv_obj_set_user_data(cb, (void*)(intptr_t)i);
        lv_obj_add_event_cb(cb, checkbox_cb, LV_EVENT_VALUE_CHANGED, NULL);

        // 价格标签
        lv_obj_t *price_lbl = lv_label_create(row);
        char price_str[32];
        lv_snprintf(price_str, sizeof(price_str), "$%.2f", items[i].price);
        lv_label_set_text(price_lbl, price_str);

        // 数量调节区域：减号、数量标签、加号
        lv_obj_t *qty_cont = lv_obj_create(row);
        lv_obj_set_size(qty_cont, 120, 40);
        lv_obj_set_flex_flow(qty_cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(qty_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_width(qty_cont, 0, 0);
        lv_obj_set_style_pad_all(qty_cont, 0, 0);

        // 减号按钮
        lv_obj_t *dec_btn = lv_btn_create(qty_cont);
        lv_obj_set_size(dec_btn, 30, 30);
        lv_obj_set_user_data(dec_btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(dec_btn, dec_quantity_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *dec_label = lv_label_create(dec_btn);
        lv_label_set_text(dec_label, "-");
        lv_obj_center(dec_label);

        // 数量显示
        lv_obj_t *qty_lbl = lv_label_create(qty_cont);
        lv_label_set_text(qty_lbl, "0");
        lv_obj_set_style_text_align(qty_lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(qty_lbl, 40);

        // 加号按钮
        lv_obj_t *inc_btn = lv_btn_create(qty_cont);
        lv_obj_set_size(inc_btn, 30, 30);
        lv_obj_set_user_data(inc_btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(inc_btn, inc_quantity_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *inc_label = lv_label_create(inc_btn);
        lv_label_set_text(inc_label, "+");
        lv_obj_center(inc_label);

        // 保存控件句柄和数据
        ctrl[i].checkbox = cb;
        ctrl[i].quantity_lbl = qty_lbl;
        ctrl[i].quantity = 0;      // 初始数量0
        ctrl[i].selected = false;   // 初始未选中
    }

    // 底部总价标签
    total_label = lv_label_create(scr);
    lv_label_set_text(total_label, "Total: $0.00");
    lv_obj_set_style_text_font(total_label, &lv_font_montserrat_18, 0);
    lv_obj_align(total_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    LOG_INFO("my_ui_init (DEEPSEEK) completed");
}
#endif
