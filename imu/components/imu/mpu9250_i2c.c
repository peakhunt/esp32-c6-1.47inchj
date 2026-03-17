#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "mpu9250_i2c.h"
#include "imu_util.h"

#define MPU9250_I2C_ADDR        0x68
#define AK8963_I2C_ADDR         0x0c

const static char* TAG = "mpu9250_i2c";

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t mpu9250_dev;
static i2c_master_dev_handle_t ak8963_dev;

void
mpu9250_i2c_init(void)
{
  i2c_master_bus_config_t bus_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = MPU9250_I2C_NUM,
    .sda_io_num = MPU9250_I2C_MASTER_SDA_IO,
    .scl_io_num = MPU9250_I2C_MASTER_SCL_IO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = false,   // already pulled up on the chinese mpu9255 breakout board
  };

  esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
  }

  // If bus is stuck, try a hardware-level reset of the I2C peripheral
  i2c_master_bus_reset(i2c_bus); 

  i2c_device_config_t dev_cfg =
  {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = MPU9250_I2C_ADDR,
    .scl_speed_hz = MPU9250_I2C_CLK_FREQ,
  };

  ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &mpu9250_dev);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to add MPU9250 device: %s", esp_err_to_name(ret));
  }

  i2c_device_config_t mag_cfg = 
  {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = AK8963_I2C_ADDR,
    .scl_speed_hz = MPU9250_I2C_CLK_FREQ,
  };

  ret = i2c_master_bus_add_device(i2c_bus, &mag_cfg, &ak8963_dev);
  if(ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to add AK8963 device: %s", esp_err_to_name(ret));
  }
}

bool
mpu9250_i2c_read(uint8_t reg, uint8_t *data, int len)
{
  if (data == NULL || len == 0)
  {
    ESP_LOGE(TAG, "i2c_read: invalid buffer/len");
    return false;
  }

  esp_err_t ret = i2c_master_transmit_receive(mpu9250_dev,
      &reg, 1,   // send register address
      data, len, // receive data
      IMU_MSEC_TO_TICKS(100));

  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "mpu9250_i2c_read failed: %s", esp_err_to_name(ret));
    return false;
  }
  return true;
}

bool
mpu9250_i2c_write(uint8_t reg, const uint8_t *data, int len)
{
  if (data == NULL || len == 0) {
    ESP_LOGE(TAG, "mpu9250_i2c_write: invalid buffer/len");
    return false;
  }

  // Limit payload size to something reasonable (e.g. 32 bytes)
  if (len > 32) {
    ESP_LOGE(TAG, "mpu9250_i2c_write: payload too large");
    return false;
  }

  uint8_t buf[33]; // 1 byte for reg + up to 32 bytes payload

  buf[0] = reg;
  memcpy(&buf[1], data, len);

  esp_err_t ret = i2c_master_transmit(mpu9250_dev,
      buf, len + 1,
      IMU_MSEC_TO_TICKS(100));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu9250_i2c_write failed: %s", esp_err_to_name(ret));
    return false;
  }
  return true;
}

bool
ak8963_i2c_read(uint8_t reg, uint8_t *data, int len)
{
  if (data == NULL || len == 0)
  {
    ESP_LOGE(TAG, "ak8963_i2c_read invalid buffer/len");
    return false;
  }

  esp_err_t ret = i2c_master_transmit_receive(ak8963_dev,
      &reg, 1,   // send register address
      data, len, // receive data
      IMU_MSEC_TO_TICKS(100));

  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "ak8963_i2c_read failed: %s", esp_err_to_name(ret));
    return false;
  }
  return true;
}

bool
ak8963_i2c_write(uint8_t reg, const uint8_t *data, int len)
{
  if (data == NULL || len == 0) {
    ESP_LOGE(TAG, "ak8963_i2c_write: invalid buffer/len");
    return false;
  }

  // Limit payload size to something reasonable (e.g. 32 bytes)
  if (len > 32) {
    ESP_LOGE(TAG, "ak8963_i2c_write: payload too large");
    return false;
  }

  uint8_t buf[33]; // 1 byte for reg + up to 32 bytes payload

  buf[0] = reg;
  memcpy(&buf[1], data, len);

  esp_err_t ret = i2c_master_transmit(ak8963_dev,
      buf, len + 1,
      IMU_MSEC_TO_TICKS(100));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "ak8963_i2c_write failed: %s", esp_err_to_name(ret));
    return false;
  }
  return true;
}
