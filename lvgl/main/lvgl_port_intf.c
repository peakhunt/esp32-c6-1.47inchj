#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "esp_lvgl_port.h"
#include "esp_check.h"
#include "st7789_lcd.h"
#include "lvgl_port_intf.h"

#define USE_DOUBLE_BUFFER       (1)

static const char* TAG = "lvgl_port_intf";

static lv_disp_t * lvgl_disp        = NULL;

LV_IMG_DECLARE(esp_logo)

void
app_rotate_screen(void)
{
  lvgl_port_lock(0);
  lv_disp_rotation_t rotation = lv_disp_get_rotation(lvgl_disp);
  rotation++;

  //
  // XXX
  // I don't know why we have to do this whenever we rotate screen.
  // And I'm too lazy to dig the source. I don't mind anymore
  //
  if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
  {
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(_panel_handle, 0, 34));     // XXX no idea why! I don't have panel spec
  }
  else
  {
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(_panel_handle, 34, 0));     // XXX no idea why! I don't have panel spec
  }

  if (rotation > LV_DISPLAY_ROTATION_270) {
    rotation = LV_DISPLAY_ROTATION_0;
  }

  /* LCD HW rotation */
  lv_disp_set_rotation(lvgl_disp, rotation);
  lvgl_port_unlock();
}

void
app_main_display(void)
{
  lvgl_port_lock(0);

  lv_obj_t *scr = lv_scr_act();


  /* Your LVGL objects code here .... */

  /* Create image */
  lv_obj_t *img_logo = lv_img_create(scr);
  lv_img_set_src(img_logo, &esp_logo);
  lv_obj_align(img_logo, LV_ALIGN_TOP_MID, 0, 20);

  /* Label */
  lv_obj_t *label = lv_label_create(scr);
  lv_obj_set_width(label, LCD_WIDTH);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
#if LVGL_VERSION_MAJOR == 8
  lv_label_set_recolor(label, true);
  lv_label_set_text(label,
      "#FF0000 "LV_SYMBOL_BELL" Hello world Espressif and LVGL "LV_SYMBOL_BELL"#\n#FF9400 "LV_SYMBOL_WARNING" For simplier initialization, use BSP "LV_SYMBOL_WARNING" #");
#else
  lv_label_set_text(label,
      LV_SYMBOL_BELL" Hello world Espressif and LVGL "LV_SYMBOL_BELL"\n "LV_SYMBOL_WARNING" For simplier initialization, use BSP "LV_SYMBOL_WARNING);
#endif
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 20);

  /* Task unlock */
  lvgl_port_unlock();
}

esp_err_t
lvgl_port_intf_init(void)
{
  const lvgl_port_cfg_t lvgl_cfg = 
  {
    .task_priority = 4,         /* LVGL task priority */
    .task_stack = 4096*2,         /* LVGL task stack size */
    .task_affinity = -1,        /* LVGL task pinned to core (-1 is no affinity) */
    .task_max_sleep_ms = 500,   /* Maximum sleep in LVGL task */
    .timer_period_ms = 5        /* LVGL timer tick period in ms */
  };
  ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

  const lvgl_port_display_cfg_t disp_cfg = 
  {
    .io_handle = _io_handle,
    .panel_handle = _panel_handle,
    .buffer_size = LCD_WIDTH * DRAW_BUFFER_HEIGHT,
    .double_buffer = USE_DOUBLE_BUFFER,
    .hres = LCD_WIDTH,
    .vres = LCD_HEIGHT,
    .monochrome = false,
    .color_format = LV_COLOR_FORMAT_RGB565,
    .rotation = {
      .swap_xy = false,
      .mirror_x = false,
      .mirror_y = false,
    },
    .flags = {
      .buff_dma = true,
      .swap_bytes = false,
    }
  };
  lvgl_disp = lvgl_port_add_disp(&disp_cfg);

  /* Add touch input (for selected screen) */
  //lv_disp_set_rotation(lvgl_disp, LV_DISPLAY_ROTATION_90);
  return ESP_OK;
}
