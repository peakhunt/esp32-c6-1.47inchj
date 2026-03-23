#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "imu.h"

#define IMU_CONFIG_MAGIC        0xdeadbeef
#define IMU_CONFIG_REVISION     1

typedef struct _imu_config_t
{
  uint32_t                  magic;
  uint16_t                  revision;
  imu_sensor_calib_data_t   sensor;
} imu_config_t;

extern void imu_config_init(void);
extern void imu_config_update_accel_calib(int16_t off[3], int16_t scale[3]);
extern void imu_config_update_gyro_calib(int16_t off[3]);
extern void imu_config_update_mag_calib(int16_t bias[3], int16_t scale[3]);
extern void imu_config_get_sensor_config(imu_sensor_calib_data_t* cfg);
