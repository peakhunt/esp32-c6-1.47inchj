#include <stdio.h>
#include <math.h>
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

#include "esp_chip_info.h"
#include "esp_idf_version.h"
#include "esp_system.h"
#include "esp_app_desc.h"
#include "driver/temperature_sensor.h"
#include "esp_heap_caps.h"
#include "imu_task.h"
#include "st7789_lcd.h"

#define NUM_ELEM(x) (sizeof(x) / sizeof((x)[0]))

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
      lv_obj_t* ax;
      lv_obj_t* ay;
      lv_obj_t* az;
      lv_obj_t* gx;
      lv_obj_t* gy;
      lv_obj_t* gz;
      lv_obj_t* mx;
      lv_obj_t* my;
      lv_obj_t* mz;
    } sensor_data;
    struct
    {
      lv_obj_t* roll;
      lv_obj_t* pitch;
      lv_obj_t* yaw;
      lv_obj_t* cpu;
      lv_obj_t* sample_rate;
      lv_obj_t* temp;
    } attitude;
  };
};

static void my_lvgl_app_page_sensor_data_init(my_lvgl_app_page_t* page);
static void my_lvgl_app_page_sensor_data_activate(my_lvgl_app_page_t* page);
static void my_lvgl_app_page_sensor_data_deactivate(my_lvgl_app_page_t* page);

static void my_lvgl_app_page_attitude_init(my_lvgl_app_page_t* page);
static void my_lvgl_app_page_attitude_activate(my_lvgl_app_page_t* page);
static void my_lvgl_app_page_attitude_deactivate(my_lvgl_app_page_t* page);

static lv_style_t style_white_font_20;
static lv_style_t style_font_20;
static lv_style_t style_white_font_42;

static temperature_sensor_handle_t temp_handle = NULL;

enum
{
  my_lvgl_app_page_attitude = 0,
  my_lvgl_app_page_sensor_data,
};

my_lvgl_app_page_t app_pages[] = 
{
  // attitude
  {
    .page_init = my_lvgl_app_page_attitude_init,
    .page_activate = my_lvgl_app_page_attitude_activate,
    .page_deactivate = my_lvgl_app_page_attitude_deactivate,
  },
  // sensor data
  {
    .page_init = my_lvgl_app_page_sensor_data_init,
    .page_activate = my_lvgl_app_page_sensor_data_activate,
    .page_deactivate = my_lvgl_app_page_sensor_data_deactivate,
  },
};

static uint8_t _current_page = 0;
static bool _screen_off = false;

//////////////////////////////////////////////////////////////////////////////////////////////////
///
/// sensor data page 
///
//////////////////////////////////////////////////////////////////////////////////////////////////
static void
my_lvgl_app_page_sensor_data_init(my_lvgl_app_page_t* page)
{
  lv_obj_t* top = lv_obj_create(lv_scr_act());
  lv_obj_set_size(top, LV_PCT(100), LV_PCT(100));
  lv_obj_set_layout(top, LV_LAYOUT_GRID);
  lv_obj_set_scrollbar_mode(top, LV_SCROLLBAR_MODE_OFF);
  lv_obj_center(top);

  // 2x9 grid
  static lv_coord_t col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc[] = {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(top, col_dsc, row_dsc);
  lv_obj_set_grid_align(top, LV_GRID_ALIGN_CENTER, LV_GRID_ALIGN_CENTER);

  lv_obj_t* lbl_acc_x_n = lv_label_create(top);
  lv_obj_add_style(lbl_acc_x_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_acc_x_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_label_set_text(lbl_acc_x_n, "ACC X");

  lv_obj_t* lbl_acc_x_v = lv_label_create(top);
  lv_obj_add_style(lbl_acc_x_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_acc_x_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_label_set_text(lbl_acc_x_v, "0");

  lv_obj_t* lbl_acc_y_n = lv_label_create(top);
  lv_obj_add_style(lbl_acc_y_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_acc_y_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_label_set_text(lbl_acc_y_n, "ACC Y");

  lv_obj_t* lbl_acc_y_v = lv_label_create(top);
  lv_obj_add_style(lbl_acc_y_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_acc_y_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_label_set_text(lbl_acc_y_v, "0");

  lv_obj_t* lbl_acc_z_n = lv_label_create(top);
  lv_obj_add_style(lbl_acc_z_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_acc_z_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_label_set_text(lbl_acc_z_n, "ACC Z");

  lv_obj_t* lbl_acc_z_v = lv_label_create(top);
  lv_obj_add_style(lbl_acc_z_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_acc_z_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_label_set_text(lbl_acc_z_v, "0");

  lv_obj_t* lbl_gyro_x_n = lv_label_create(top);
  lv_obj_add_style(lbl_gyro_x_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_gyro_x_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
  lv_label_set_text(lbl_gyro_x_n, "GYRO X");

  lv_obj_t* lbl_gyro_x_v = lv_label_create(top);
  lv_obj_add_style(lbl_gyro_x_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_gyro_x_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 3, 1);
  lv_label_set_text(lbl_gyro_x_v, "0");

  lv_obj_t* lbl_gyro_y_n = lv_label_create(top);
  lv_obj_add_style(lbl_gyro_y_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_gyro_y_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_label_set_text(lbl_gyro_y_n, "GYRO Y");

  lv_obj_t* lbl_gyro_y_v = lv_label_create(top);
  lv_obj_add_style(lbl_gyro_y_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_gyro_y_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_label_set_text(lbl_gyro_y_v, "0");

  lv_obj_t* lbl_gyro_z_n = lv_label_create(top);
  lv_obj_add_style(lbl_gyro_z_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_gyro_z_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 5, 1);
  lv_label_set_text(lbl_gyro_z_n, "GYRO Z");

  lv_obj_t* lbl_gyro_z_v = lv_label_create(top);
  lv_obj_add_style(lbl_gyro_z_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_gyro_z_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 5, 1);
  lv_label_set_text(lbl_gyro_z_v, "0");

  lv_obj_t* lbl_mag_x_n = lv_label_create(top);
  lv_obj_add_style(lbl_mag_x_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_mag_x_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_label_set_text(lbl_mag_x_n, "MAG X");

  lv_obj_t* lbl_mag_x_v = lv_label_create(top);
  lv_obj_add_style(lbl_mag_x_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_mag_x_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_label_set_text(lbl_mag_x_v, "0");

  lv_obj_t* lbl_mag_y_n = lv_label_create(top);
  lv_obj_add_style(lbl_mag_y_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_mag_y_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 7, 1);
  lv_label_set_text(lbl_mag_y_n, "MAG Y");

  lv_obj_t* lbl_mag_y_v = lv_label_create(top);
  lv_obj_add_style(lbl_mag_y_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_mag_y_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 7, 1);
  lv_label_set_text(lbl_mag_y_v, "0");

  lv_obj_t* lbl_mag_z_n = lv_label_create(top);
  lv_obj_add_style(lbl_mag_z_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_mag_z_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 8, 1);
  lv_label_set_text(lbl_mag_z_n, "MAG Z");

  lv_obj_t* lbl_mag_z_v = lv_label_create(top);
  lv_obj_add_style(lbl_mag_z_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_mag_z_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 8, 1);
  lv_label_set_text(lbl_mag_z_v, "0");

  page->top = top;
  page->sensor_data.ax = lbl_acc_x_v;
  page->sensor_data.ay = lbl_acc_y_v;
  page->sensor_data.az = lbl_acc_z_v;
  page->sensor_data.gx = lbl_gyro_x_v;
  page->sensor_data.gy = lbl_gyro_y_v;
  page->sensor_data.gz = lbl_gyro_z_v;
  page->sensor_data.mx = lbl_mag_x_v;
  page->sensor_data.my = lbl_mag_y_v;
  page->sensor_data.mz = lbl_mag_z_v;
}

static void
my_lvgl_app_page_sensor_data_activate(my_lvgl_app_page_t* page)
{
  // nothing to do
}

static void
my_lvgl_app_page_sensor_data_deactivate(my_lvgl_app_page_t* page)
{
  // nothing to do
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///
/// attitude Page
///
//////////////////////////////////////////////////////////////////////////////////////////////////
static void
my_lvgl_app_page_attitude_init(my_lvgl_app_page_t* page)
{
  lv_obj_t* top = lv_obj_create(lv_scr_act());
  lv_obj_set_size(top, LV_PCT(100), LV_PCT(100));
  lv_obj_set_layout(top, LV_LAYOUT_GRID);
  lv_obj_set_scrollbar_mode(top, LV_SCROLLBAR_MODE_OFF);
  lv_obj_center(top);

  // 2x  grid
  static lv_coord_t col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc[] = {
    LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
    LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
#if 0
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_FR(1), LV_GRID_FR(1),
#endif
    LV_GRID_TEMPLATE_LAST};

  lv_obj_set_grid_dsc_array(top, col_dsc, row_dsc);
  lv_obj_set_grid_align(top, LV_GRID_ALIGN_START, LV_GRID_ALIGN_START);

  lv_obj_t* lbl_roll_n = lv_label_create(top);
  lv_obj_add_style(lbl_roll_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_roll_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
  lv_label_set_text(lbl_roll_n, "Roll");

  lv_obj_t* lbl_roll_v = lv_label_create(top);
  lv_obj_add_style(lbl_roll_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_roll_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);
  lv_label_set_text(lbl_roll_v, "0");

  lv_obj_t* lbl_pitch_n = lv_label_create(top);
  lv_obj_add_style(lbl_pitch_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_pitch_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 1, 1);
  lv_label_set_text(lbl_pitch_n, "Pitch");

  lv_obj_t* lbl_pitch_v = lv_label_create(top);
  lv_obj_add_style(lbl_pitch_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_pitch_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 1, 1);
  lv_label_set_text(lbl_pitch_v, "0");

  lv_obj_t* lbl_yaw_n = lv_label_create(top);
  lv_obj_add_style(lbl_yaw_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_yaw_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 2, 1);
  lv_label_set_text(lbl_yaw_n, "Yaw");

  lv_obj_t* lbl_yaw_v = lv_label_create(top);
  lv_obj_add_style(lbl_yaw_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_yaw_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 2, 1);
  lv_label_set_text(lbl_yaw_v, "0");

  lv_obj_t* lbl_cpu_n = lv_label_create(top);
  lv_obj_add_style(lbl_cpu_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_cpu_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 3, 1);
  lv_label_set_text(lbl_cpu_n, "CPU");

  lv_obj_t* lbl_cpu_v = lv_label_create(top);
  lv_obj_add_style(lbl_cpu_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_cpu_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 3, 1);
  lv_label_set_text(lbl_cpu_v, "0 %");

  lv_obj_t* lbl_sample_n = lv_label_create(top);
  lv_obj_add_style(lbl_sample_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_sample_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 4, 1);
  lv_label_set_text(lbl_sample_n, "Rate");

  lv_obj_t* lbl_sample_v = lv_label_create(top);
  lv_obj_add_style(lbl_sample_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_sample_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 4, 1);
  lv_label_set_text(lbl_sample_v, "0 Hz");

  lv_obj_t* lbl_temp_n = lv_label_create(top);
  lv_obj_add_style(lbl_temp_n, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_temp_n, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 5, 1);
  lv_label_set_text(lbl_temp_n, "Temp");

  lv_obj_t* lbl_temp_v = lv_label_create(top);
  lv_obj_add_style(lbl_temp_v, &style_font_20, 0);
  lv_obj_set_grid_cell(lbl_temp_v, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 5, 1);
  lv_label_set_text(lbl_temp_v, "0 °C");

  page->top = top;
  page->attitude.roll   = lbl_roll_v;
  page->attitude.pitch  = lbl_pitch_v;
  page->attitude.yaw    = lbl_yaw_v;
  page->attitude.cpu    = lbl_cpu_v;
  page->attitude.sample_rate = lbl_sample_v;
  page->attitude.temp = lbl_temp_v;
}

static void
my_lvgl_app_page_attitude_activate(my_lvgl_app_page_t* page)
{
}

static void
my_lvgl_app_page_attitude_deactivate(my_lvgl_app_page_t* page)
{
  lvgl_port_lock(0);
  lvgl_port_unlock();
}

static void
my_lvgl_app_show_page(uint8_t page)
{
  lvgl_port_lock(0);

  if(_current_page != page)
  {
    lv_obj_add_flag(app_pages[_current_page].top, LV_OBJ_FLAG_HIDDEN);
    app_pages[_current_page].page_deactivate(&app_pages[_current_page]);
  }

  _current_page = page;
  lv_obj_clear_flag(app_pages[_current_page].top, LV_OBJ_FLAG_HIDDEN);
  app_pages[_current_page].page_activate(&app_pages[_current_page]);

  lvgl_port_unlock();
}

//
// XXX
// this is called from LVGL task context
// 
static void
watt_simulator_cb(lv_timer_t * timer)
{
  char buf[16];

  float tsens_out = 0;
  imu_data_t    imu_data;

  imu_task_get_data(&imu_data);

  ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_handle, &tsens_out));

  if(_screen_off) return;

  lvgl_port_lock(0);

  extern volatile uint32_t cpu_usage;

  snprintf(buf, 16, "%.1f", imu_data.accel[0]);
  lv_label_set_text(app_pages[my_lvgl_app_page_sensor_data].sensor_data.ax, buf);
  snprintf(buf, 16, "%.1f", imu_data.accel[1]);
  lv_label_set_text(app_pages[my_lvgl_app_page_sensor_data].sensor_data.ay, buf);
  snprintf(buf, 16, "%.1f", imu_data.accel[2]);
  lv_label_set_text(app_pages[my_lvgl_app_page_sensor_data].sensor_data.az, buf);

  snprintf(buf, 16, "%.1f", imu_data.gyro[0]);
  lv_label_set_text(app_pages[my_lvgl_app_page_sensor_data].sensor_data.gx, buf);
  snprintf(buf, 16, "%.1f", imu_data.gyro[1]);
  lv_label_set_text(app_pages[my_lvgl_app_page_sensor_data].sensor_data.gy, buf);
  snprintf(buf, 16, "%.1f", imu_data.gyro[2]);
  lv_label_set_text(app_pages[my_lvgl_app_page_sensor_data].sensor_data.gz, buf);

  snprintf(buf, 16, "%.1f", imu_data.mag[0]);
  lv_label_set_text(app_pages[my_lvgl_app_page_sensor_data].sensor_data.mx, buf);
  snprintf(buf, 16, "%.1f", imu_data.mag[1]);
  lv_label_set_text(app_pages[my_lvgl_app_page_sensor_data].sensor_data.my, buf);
  snprintf(buf, 16, "%.1f", imu_data.mag[2]);
  lv_label_set_text(app_pages[my_lvgl_app_page_sensor_data].sensor_data.mz, buf);

  snprintf(buf, 16, "%.1f", imu_data.orientation[0]);
  lv_label_set_text(app_pages[my_lvgl_app_page_attitude].attitude.roll, buf);
  snprintf(buf, 16, "%.1f", imu_data.orientation[1]);
  lv_label_set_text(app_pages[my_lvgl_app_page_attitude].attitude.pitch, buf);
  snprintf(buf, 16, "%.1f", imu_data.orientation[2]);

  lv_label_set_text(app_pages[my_lvgl_app_page_attitude].attitude.yaw, buf);
  lv_label_set_text_fmt(app_pages[my_lvgl_app_page_attitude].attitude.cpu,  "%d %%", (int)cpu_usage);
  lv_label_set_text_fmt(app_pages[my_lvgl_app_page_attitude].attitude.sample_rate,  "%ld Hz", _sample_rate);

  snprintf(buf, 16, "%.1f°C", tsens_out);
  lv_label_set_text(app_pages[my_lvgl_app_page_attitude].attitude.temp, buf);

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
my_lvgl_app_user_btn_long_pressed(void)
{
  if(_screen_off)
  {
    _screen_off = false;
    st7789_disp_on_off(true);
    st7789_backlight_on_off(true);
  }
  else
  {
    _screen_off = true;
    st7789_disp_on_off(false);
    st7789_backlight_on_off(false);
  }
}

void
my_lvgl_app_init(void)
{
  // configure temperature sensor
  temperature_sensor_config_t temp_sensor = TEMPERATURE_SENSOR_CONFIG_DEFAULT(0, 80);
  ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor, &temp_handle));
  ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));

  lvgl_port_lock(0);

  lv_obj_t * scr = lv_scr_act();  // get the active screen
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  lv_style_init(&style_white_font_20);
  lv_style_set_text_color(&style_white_font_20, lv_color_white());
  lv_style_set_text_font(&style_white_font_20, &lv_font_montserrat_20);

  lv_style_init(&style_font_20);
  lv_style_set_text_font(&style_font_20, &lv_font_montserrat_20);

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
  lv_timer_create(watt_simulator_cb, 200, NULL);

  lvgl_port_unlock();
}
