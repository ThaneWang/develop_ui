//lvgl_ui_practice分支，练习ui界面设计
#include "ui.h" // 包含 UI 头文件，声明接口和依赖

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
}

void checkbox_event_cb(lv_event_t * e) // 复选框事件回调（值改变时调用）
{
    LV_UNUSED(e); // 避免未使用参数的警告
    update_total(); // 当复选框值变化时重新计算总价
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
}
#endif
