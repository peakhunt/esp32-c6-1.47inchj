/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "st7789_lcd.h"

#define GPIO_INPUT_IO_9     9
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_9))

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static led_strip_handle_t led_strip;

static uint32_t t = 0;  // time counter

static QueueHandle_t gpio_evt_queue = NULL;

static
void change_led(void)
{
  // Use sine waves with phase offsets to generate flamboyant colors
  uint8_t r = (uint8_t)(127.5 * (1 + sin(0.1 * t)));
  uint8_t g = (uint8_t)(127.5 * (1 + sin(0.1 * t + 2.0 * M_PI / 3.0)));
  uint8_t b = (uint8_t)(127.5 * (1 + sin(0.1 * t + 4.0 * M_PI / 3.0)));

  led_strip_set_pixel(led_strip, 0, r, g, b);
  led_strip_refresh(led_strip);

  t++;  // advance time
}

static void
configure_led(void)
{
  ESP_LOGI(TAG, "Example configured to blink addressable LED!");
  /* LED strip initialization with the GPIO and pixels number*/
  led_strip_config_t strip_config = {
    .strip_gpio_num = BLINK_GPIO,
    .max_leds = 1, // at least one LED on board
  };

  led_strip_rmt_config_t rmt_config = {
    .resolution_hz = 10 * 1000 * 1000, // 10MHz
    .flags.with_dma = false,
  };

  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  /* Set all LED off to clear all pixels */
  led_strip_clear(led_strip);
}

static void IRAM_ATTR
gpio_isr_handler(void* arg)
{
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void
configure_gpio_input(void)
{
  gpio_config_t io_conf = {};

  //interrupt of rising edge
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  //bit mask of the pins, use GPIO4/5 here
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
  //set as input mode
  io_conf.mode = GPIO_MODE_INPUT;
  //enable pull-up mode
  io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

  gpio_install_isr_service(0);
  gpio_isr_handler_add(GPIO_INPUT_IO_9, gpio_isr_handler, (void*) GPIO_INPUT_IO_9);
}

void
app_main(void)
{
  uint32_t  io_num;
  bool      key_pressed = false;
  uint32_t  io_val;
  uint8_t   offset = 0;

  /* Configure the peripheral according to the LED type */
  configure_gpio_input();
  configure_led();
  st7789_init_lcd();

  st7789_fill_screen_pat(0xF800, 0x07E0, 0x001F, 0x0000, offset); // red, green, blue, black

  while (1)
  {
    change_led();
    /* Toggle the LED state */
    //vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    if (xQueueReceive(gpio_evt_queue, &io_num, CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS))
    {
      io_val = gpio_get_level(io_num);
      if (key_pressed == false && io_val == 0)
      {
        ESP_LOGI(TAG, "key pressed...");
        key_pressed = true;
        offset = (offset + 1) % 4;
        st7789_fill_screen_pat(0xF800, 0x07E0, 0x001F, 0x0000, offset); // red, green, blue, black
      }
      else if(key_pressed == true && io_val == 1)
      {
        key_pressed = false;
        ESP_LOGI(TAG, "key released...");
      }
    }
  }
}
