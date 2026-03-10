#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "esp_lvgl_port.h"
#include "esp_check.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

LV_IMG_DECLARE(page1_ui)
LV_IMG_DECLARE(img4)
LV_IMG_DECLARE(img5)

static uint8_t _current_page = 0;

static lv_obj_t* _pages[] = 
{
  NULL,
  NULL,
  NULL,
};

//
// Page 0 Data
//
static lv_obj_t* page0_lbl_c1;
static lv_obj_t* page0_lbl_c2;
static lv_obj_t* page0_lbl_c3;
static lv_obj_t* page0_lbl_c4;
static lv_obj_t* page0_lbl_a1;
static lv_obj_t* page0_lbl_a2;

static void
my_lvgl_app_show_page(uint8_t page)
{
  lvgl_port_lock(0);

  if(_current_page != page)
  {
    lv_obj_add_flag(_pages[_current_page], LV_OBJ_FLAG_HIDDEN);
  }

  _current_page = page;
  lv_obj_clear_flag(_pages[_current_page], LV_OBJ_FLAG_HIDDEN);

  lvgl_port_unlock();
}

//
// XXX
// this is called from LVGL task context
// 
static void
watt_simulator_cb(lv_timer_t * timer)
{
  lvgl_port_lock(0);

  char buf[16];

  int w1 = rand() % 141;  // 0–140
  int w2 = rand() % 141;
  int w3 = rand() % 141;
  int w4 = rand() % 141;
  int w5 = rand() % 141;
  int w6 = rand() % 141;

  sprintf(buf, "%d W", w1);
  lv_label_set_text(page0_lbl_c1, buf);

  sprintf(buf, "%d W", w2);
  lv_label_set_text(page0_lbl_c2, buf);

  sprintf(buf, "%d W", w3);
  lv_label_set_text(page0_lbl_c3, buf);

  sprintf(buf, "%d W", w4);
  lv_label_set_text(page0_lbl_c4, buf);

  sprintf(buf, "%d W", w5);
  lv_label_set_text(page0_lbl_a1, buf);

  sprintf(buf, "%d W", w6);
  lv_label_set_text(page0_lbl_a2, buf);

  lvgl_port_unlock();
}

static void
my_lvgl_app_init_page2(void)
{
  lv_obj_t * bg = lv_img_create(lv_scr_act());
  lv_img_set_src(bg, &img5);
  lv_obj_set_pos(bg, 0, 0);
  _pages[2] = bg;
  lv_obj_add_flag(_pages[2], LV_OBJ_FLAG_HIDDEN);
}

static void
my_lvgl_app_init_page1(void)
{
  lv_obj_t * bg = lv_img_create(lv_scr_act());
  lv_img_set_src(bg, &img4);
  lv_obj_set_pos(bg, 0, 0);
  _pages[1] = bg;
  lv_obj_add_flag(_pages[1], LV_OBJ_FLAG_HIDDEN);
}

static void
my_lvgl_app_init_page0(void)
{
#define LABEL_NUM_WIDTH     70

  lv_obj_t * bg = lv_img_create(lv_scr_act());
  lv_img_set_src(bg, &page1_ui);
  lv_obj_set_pos(bg, 0, 0);

  /* Create a style for white text */
  static lv_style_t style_white;
  lv_style_init(&style_white);
  lv_style_set_text_color(&style_white, lv_color_white());
  lv_style_set_text_font(&style_white, &lv_font_montserrat_20);

  lv_obj_t* lbl_c1 = lv_label_create(bg);
  lv_label_set_text(lbl_c1, "0 W");
  lv_obj_add_style(lbl_c1, &style_white, 0);
  lv_obj_set_width(lbl_c1, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_c1, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_c1, 5, 67);

  lv_obj_t* lbl_c2 = lv_label_create(bg);
  lv_label_set_text(lbl_c2, "0 W");
  lv_obj_add_style(lbl_c2, &style_white, 0);
  lv_obj_set_width(lbl_c2, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_c2, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_c2, 91, 67);

  lv_obj_t* lbl_c3 = lv_label_create(bg);
  lv_label_set_text(lbl_c3, "0 W");
  lv_obj_add_style(lbl_c3, &style_white, 0);
  lv_obj_set_width(lbl_c3, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_c3, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_c3, 5, 162);

  lv_obj_t* lbl_c4 = lv_label_create(bg);
  lv_label_set_text(lbl_c4, "0 W");
  lv_obj_add_style(lbl_c4, &style_white, 0);
  lv_obj_set_width(lbl_c4, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_c4, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_c4, 91, 162);

  lv_obj_t* lbl_a1 = lv_label_create(bg);
  lv_label_set_text(lbl_a1, "0 W");
  lv_obj_add_style(lbl_a1, &style_white, 0);
  lv_obj_set_width(lbl_a1, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_a1, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_a1, 5, 256);

  lv_obj_t* lbl_a2 = lv_label_create(bg);
  lv_label_set_text(lbl_a2, "0 W");
  lv_obj_add_style(lbl_a2, &style_white, 0);
  lv_obj_set_width(lbl_a2, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_a2, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_a2, 91, 256);

  _pages[0] = bg;
  page0_lbl_c1 = lbl_c1;
  page0_lbl_c2 = lbl_c2;
  page0_lbl_c3 = lbl_c3;
  page0_lbl_c4 = lbl_c4;
  page0_lbl_a1 = lbl_a1;
  page0_lbl_a2 = lbl_a2;

  lv_obj_add_flag(_pages[0], LV_OBJ_FLAG_HIDDEN);
}

void
my_lvgl_app_user_btn_pressed(void)
{
  int num_pages = sizeof(_pages)/sizeof(lv_obj_t*);
  uint8_t page = _current_page;
  page = (page + 1) % num_pages;

  my_lvgl_app_show_page(page);
}

void
my_lvgl_app_init(void)
{
  lvgl_port_lock(0);

  my_lvgl_app_init_page0();
  my_lvgl_app_init_page1();
  my_lvgl_app_init_page2();

  my_lvgl_app_show_page(0);

  /* Seed random generator */
  srand(time(NULL));

  /* Create a timer to update every second */
  lv_timer_create(watt_simulator_cb, 1000, NULL);

  lvgl_port_unlock();
}
