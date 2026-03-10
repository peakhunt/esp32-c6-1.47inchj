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

#define NUM_ELEM(x) (sizeof(x) / sizeof((x)[0]))

LV_IMG_DECLARE(page0_ui)
LV_IMG_DECLARE(glow)
LV_IMG_DECLARE(img5)

typedef struct my_lvgl_app_page my_lvgl_app_page_t;

struct my_lvgl_app_page
{
  lv_obj_t*   top;
  void (*page_init)(my_lvgl_app_page_t* page);
  void (*page_activate)(my_lvgl_app_page_t* page);
  void (*page_deactivate)(my_lvgl_app_page_t* page);

  union
  {
    struct
    {
      lv_obj_t* lbl_c1;
      lv_obj_t* lbl_c2;
      lv_obj_t* lbl_c3;
      lv_obj_t* lbl_c4;
      lv_obj_t* lbl_a1;
      lv_obj_t* lbl_a2;
    } p0;
    struct
    {
      lv_obj_t* lbl_total;
    } p1;
    struct
    {
    } p2;
  };
};

static void my_lvgl_app_page0_init(my_lvgl_app_page_t* page);
static void my_lvgl_app_page0_activate(my_lvgl_app_page_t* page);
static void my_lvgl_app_page0_deactivate(my_lvgl_app_page_t* page);

static void my_lvgl_app_page1_init(my_lvgl_app_page_t* page);
static void my_lvgl_app_page1_activate(my_lvgl_app_page_t* page);
static void my_lvgl_app_page1_deactivate(my_lvgl_app_page_t* page);

static void my_lvgl_app_page2_init(my_lvgl_app_page_t* page);
static void my_lvgl_app_page2_activate(my_lvgl_app_page_t* page);
static void my_lvgl_app_page2_deactivate(my_lvgl_app_page_t* page);

static lv_style_t style_white_font_20;
static lv_style_t style_white_font_42;

my_lvgl_app_page_t app_pages[] = 
{
  // Page 0
  {
    .page_init = my_lvgl_app_page0_init,
    .page_activate = my_lvgl_app_page0_activate,
    .page_deactivate = my_lvgl_app_page0_deactivate,
  },
  // Page 1
  {
    .page_init = my_lvgl_app_page1_init,
    .page_activate = my_lvgl_app_page1_activate,
    .page_deactivate = my_lvgl_app_page1_deactivate,
  },
  // Page 2
  {
    .page_init = my_lvgl_app_page2_init,
    .page_activate = my_lvgl_app_page2_activate,
    .page_deactivate = my_lvgl_app_page2_deactivate,
  },
};

static uint8_t _current_page = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Page 0
///
//////////////////////////////////////////////////////////////////////////////////////////////////
static void
my_lvgl_app_page0_init(my_lvgl_app_page_t* page)
{
  #define LABEL_NUM_WIDTH     70

  lv_obj_t * bg = lv_img_create(lv_scr_act());
  lv_img_set_src(bg, &page0_ui);
  lv_obj_set_pos(bg, 0, 0);

  lv_obj_t* lbl_c1 = lv_label_create(bg);
  lv_label_set_text(lbl_c1, "0 W");
  lv_obj_add_style(lbl_c1, &style_white_font_20, 0);
  lv_obj_set_width(lbl_c1, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_c1, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_c1, 5, 67);

  lv_obj_t* lbl_c2 = lv_label_create(bg);
  lv_label_set_text(lbl_c2, "0 W");
  lv_obj_add_style(lbl_c2, &style_white_font_20, 0);
  lv_obj_set_width(lbl_c2, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_c2, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_c2, 91, 67);

  lv_obj_t* lbl_c3 = lv_label_create(bg);
  lv_label_set_text(lbl_c3, "0 W");
  lv_obj_add_style(lbl_c3, &style_white_font_20, 0);
  lv_obj_set_width(lbl_c3, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_c3, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_c3, 5, 162);

  lv_obj_t* lbl_c4 = lv_label_create(bg);
  lv_label_set_text(lbl_c4, "0 W");
  lv_obj_add_style(lbl_c4, &style_white_font_20, 0);
  lv_obj_set_width(lbl_c4, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_c4, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_c4, 91, 162);

  lv_obj_t* lbl_a1 = lv_label_create(bg);
  lv_label_set_text(lbl_a1, "0 W");
  lv_obj_add_style(lbl_a1, &style_white_font_20, 0);
  lv_obj_set_width(lbl_a1, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_a1, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_a1, 5, 256);

  lv_obj_t* lbl_a2 = lv_label_create(bg);
  lv_label_set_text(lbl_a2, "0 W");
  lv_obj_add_style(lbl_a2, &style_white_font_20, 0);
  lv_obj_set_width(lbl_a2, LABEL_NUM_WIDTH);
  lv_obj_set_style_text_align(lbl_a2, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(lbl_a2, 91, 256);

  page->top = bg;
  page->p0.lbl_c1 = lbl_c1;
  page->p0.lbl_c2 = lbl_c2;
  page->p0.lbl_c3 = lbl_c3;
  page->p0.lbl_c4 = lbl_c4;
  page->p0.lbl_a1 = lbl_a1;
  page->p0.lbl_a2 = lbl_a2;
}

static void
my_lvgl_app_page0_activate(my_lvgl_app_page_t* page)
{
  // nothing to do
}

static void
my_lvgl_app_page0_deactivate(my_lvgl_app_page_t* page)
{
  // nothing to do
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Page 1
///
//////////////////////////////////////////////////////////////////////////////////////////////////
static void
my_lvgl_app_page1_init(my_lvgl_app_page_t* page)
{
  lv_obj_t * bg = lv_img_create(lv_scr_act());
  lv_img_set_src(bg, &glow);
  lv_obj_set_pos(bg, 0, 80);

  lv_obj_t* lbl_total = lv_label_create(bg);
  lv_label_set_text(lbl_total, "0");
  lv_obj_add_style(lbl_total, &style_white_font_42, 0);
  lv_obj_set_width(lbl_total, 172);
  lv_obj_set_style_text_align(lbl_total, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(lbl_total, 0, 52);
  
  lv_obj_t* lbl_w = lv_label_create(bg);
  lv_label_set_text(lbl_w, "W");
  lv_obj_add_style(lbl_w, &style_white_font_20, 0);
  lv_obj_set_width(lbl_w, 172);
  lv_obj_set_style_text_align(lbl_w, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(lbl_w, 0, 100);

  page->top = bg;
  page->p1.lbl_total = lbl_total;
}

static void
my_lvgl_app_page1_activate(my_lvgl_app_page_t* page)
{
}

static void
my_lvgl_app_page1_deactivate(my_lvgl_app_page_t* page)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Page 2
///
//////////////////////////////////////////////////////////////////////////////////////////////////
static void
my_lvgl_app_page2_init(my_lvgl_app_page_t* page)
{
  lv_obj_t * bg = lv_img_create(lv_scr_act());
  lv_img_set_src(bg, &img5);
  lv_obj_set_pos(bg, 0, 0);

  page->top = bg;
}

static void
my_lvgl_app_page2_activate(my_lvgl_app_page_t* page)
{
}

static void
my_lvgl_app_page2_deactivate(my_lvgl_app_page_t* page)
{
}

static void
my_lvgl_app_show_page(uint8_t page)
{
  lvgl_port_lock(0);

  if(_current_page != page)
  {
    lv_obj_add_flag(app_pages[_current_page].top, LV_OBJ_FLAG_HIDDEN);
  }

  _current_page = page;
  lv_obj_clear_flag(app_pages[_current_page].top, LV_OBJ_FLAG_HIDDEN);

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
  int total = w1 + w2 + w3 + w4 + w5 + w6;

  sprintf(buf, "%d W", w1);
  lv_label_set_text(app_pages[0].p0.lbl_c1, buf);

  sprintf(buf, "%d W", w2);
  lv_label_set_text(app_pages[0].p0.lbl_c2, buf);

  sprintf(buf, "%d W", w3);
  lv_label_set_text(app_pages[0].p0.lbl_c3, buf);

  sprintf(buf, "%d W", w4);
  lv_label_set_text(app_pages[0].p0.lbl_c4, buf);

  sprintf(buf, "%d W", w5);
  lv_label_set_text(app_pages[0].p0.lbl_a1, buf);

  sprintf(buf, "%d W", w6);
  lv_label_set_text(app_pages[0].p0.lbl_a2, buf);

  sprintf(buf, "%d", total);
  lv_label_set_text(app_pages[1].p1.lbl_total, buf);

  lvgl_port_unlock();
}

void
my_lvgl_app_user_btn_pressed(void)
{
  int num_pages = NUM_ELEM(app_pages);
  uint8_t page = _current_page;
  page = (page + 1) % num_pages;

  my_lvgl_app_show_page(page);
}

void
my_lvgl_app_init(void)
{
  lvgl_port_lock(0);

  lv_obj_t * scr = lv_scr_act();  // get the active screen
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  lv_style_init(&style_white_font_20);
  lv_style_set_text_color(&style_white_font_20, lv_color_white());
  lv_style_set_text_font(&style_white_font_20, &lv_font_montserrat_20);

  lv_style_init(&style_white_font_42);
  lv_style_set_text_color(&style_white_font_42, lv_color_white());
  lv_style_set_text_font(&style_white_font_42, &lv_font_montserrat_42);

  for(int i = 0; i < NUM_ELEM(app_pages); i++)
  {
    app_pages[i].page_init(&app_pages[i]);
    lv_obj_add_flag(app_pages[i].top, LV_OBJ_FLAG_HIDDEN);
  }

  my_lvgl_app_show_page(0);

  /* Seed random generator */
  srand(time(NULL));

  /* Create a timer to update every second */
  lv_timer_create(watt_simulator_cb, 1000, NULL);

  lvgl_port_unlock();
}
