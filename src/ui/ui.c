#include "ui.h"

void my_ui_init(void)
{
  printf("enter ui ...\r\ninit success\n");

    //获取硬件整个屏幕
  lv_obj_t * scr = lv_scr_act();

  //创建一个新的屏幕
  lv_obj_t * obj = lv_obj_create(scr);
  lv_obj_set_size(obj,(MY_SCREEN_WIDTH)/4,(MY_SCREEN_HEIGHT)/2);
  lv_obj_set_align(obj,LV_ALIGN_CENTER);
  //lv_obj_set_style_bg_color(obj,lv_color_hex(0xFF8000),0);

  //创建label 显示"这是一个练习ui界面"
  lv_obj_t * label = lv_label_create(obj);
  lv_label_set_text(label,"this is a ui practice example!\n yes my lor!");
  lv_obj_set_align(label,LV_ALIGN_TOP_MID);

  //创建一个按钮
  lv_obj_t *btn = lv_button_create(obj);
  lv_obj_set_size(btn,MY_SCREEN_WIDTH/40,MY_SCREEN_HEIGHT/40);
  //lv_obj_center(btn);
  lv_obj_set_style_bg_color(btn,lv_color_hex(0x008000),0);
  lv_obj_set_align(btn,LV_ALIGN_BOTTOM_MID);

  //创建一个滑块
  lv_obj_t * slider = lv_slider_create(obj);
  lv_obj_set_size(slider,MY_SCREEN_WIDTH/10,MY_SCREEN_HEIGHT/40);
  lv_obj_center(slider);


  printf("enter ui ...\r\ninit success\n");

  lv_obj_t * scr = lv_scr_act();

  /* 创建主容器 */
  lv_obj_t * obj = lv_obj_create(scr);
  lv_obj_set_size(obj, MY_SCREEN_WIDTH/2, MY_SCREEN_HEIGHT/2);
  lv_obj_set_align(obj, LV_ALIGN_CENTER);
  lv_obj_set_style_pad_all(obj, 10, 0);

  /* 菜单项数据 */
  enum { NUM_ITEMS = 6 };
  static const char * names[NUM_ITEMS] = {"T-Shirt","Jacket","Pants","Dress","Hat","Socks"};
  static const int prices_arr[NUM_ITEMS] = {10, 50, 30, 45, 8, 3};
  static lv_obj_t * cbs[NUM_ITEMS];
  static int prices[NUM_ITEMS];
  for (int i = 0; i < NUM_ITEMS; i++) prices[i] = prices_arr[i];

  /* 标题 */
  lv_obj_t * title = lv_label_create(obj);
  lv_label_set_text(title, "Clothing Menu");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
  lv_obj_set_align(title, LV_ALIGN_TOP_MID);

  /* 从上到下创建复选框和价格标签 */
  int y_off = 40;
  for (int i = 0; i < NUM_ITEMS; i++) {
    cbs[i] = lv_checkbox_create(obj);
    lv_checkbox_set_text(cbs[i], names[i]);
    lv_obj_set_pos(cbs[i], 10, y_off);
    lv_obj_set_width(cbs[i], lv_obj_get_width(obj) / 2);

    char buf[32];
    snprintf(buf, sizeof(buf), "$%d", prices[i]);
    lv_obj_t * price_lbl = lv_label_create(obj);
    lv_label_set_text(price_lbl, buf);
    lv_obj_set_pos(price_lbl, lv_obj_get_width(obj) - 60, y_off);

    y_off += 36;
  }

  /* 总价标签（初始为0） */
  static lv_obj_t * total_label = NULL;
  total_label = lv_label_create(obj);
  lv_label_set_text(total_label, "Total: $0");
  lv_obj_set_align(total_label, LV_ALIGN_BOTTOM_MID);

  /* 事件回调：每次复选框状态改变时重新计算总价 */
  void checkbox_event_cb(lv_event_t * e)
  {
    int sum = 0;
    for (int i = 0; i < NUM_ITEMS; i++) {
      if (lv_obj_has_state(cbs[i], LV_STATE_CHECKED)) sum += prices[i];
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "Total: $%d", sum);
    lv_label_set_text(total_label, buf);
  }

  /* 绑定事件 */
  for (int i = 0; i < NUM_ITEMS; i++) {
    lv_obj_add_event_cb(cbs[i], checkbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  }




  //lv_obj_set_align(obj,LV_ALIGN_CENTER);






}
