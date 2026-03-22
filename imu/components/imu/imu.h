#ifndef __IMU_DEF_H__
#define __IMU_DEF_H__

#include <stdint.h>
#include <stdbool.h>
#include "madgwick.h"
#include "mahony.h"
#include "imu_common.h"

#define USE_MADGWICK                  1
#define IMU_STORE_UNCALIBRATED_DATA   0
#define IMU_USE_MAG_CALIB_SOFT_IRON   1


typedef enum
{
  imu_mode_normal,
  imu_mode_accel_calibrating,
  imu_mode_gyro_calibrating,
  imu_mode_mag_calibrating,
} imu_mode_t;

typedef enum
{
  imu_sensor_align_cw_0,
  imu_sensor_align_cw_90,
  imu_sensor_align_cw_180,
  imu_sensor_align_cw_270,
  imu_sensor_align_cw_0_flip,
  imu_sensor_align_cw_90_flip,
  imu_sensor_align_cw_180_flip,
  imu_sensor_align_cw_270_flip,
  imu_sensor_align_special,
  imu_sensor_align_special2
} imu_sensor_align_t;

typedef struct
{
  int16_t   accel_off[3];
  int16_t   accel_scale[3];
  int16_t   gyro_off[3];
  int16_t   mag_bias[3];
#if IMU_USE_MAG_CALIB_SOFT_IRON == 1
  int16_t   mag_scale[3];
#endif
  float     mag_declination;
} imu_sensor_calib_data_t;

typedef struct
{
  float     accel[3];             // in G
  float     gyro[3];              // in degrees per sec  (not radian)
  float     mag[3];               // in uT
#if IMU_STORE_UNCALIBRATED_DATA == 1
  float     u_accel[3];           // uncalibrated in G
  float     u_gyro[3];            // uncalibrated in degrees per sec  (not radian)
  float     u_mag[3];             // uncalibrated in uT
#endif
  float     temp;                 // in celcius
  float     orientation[3];       // AHRS output
  float     quaternion[4];
} imu_data_t;

typedef struct
{
  imu_mode_t                mode;

  imu_sensor_data_t         raw;
  imu_sensor_data_t         adjusted;

  imu_sensor_calib_data_t   cal;

  imu_sensor_align_t         accel_align;
  imu_sensor_align_t         gyro_align;
  imu_sensor_align_t         mag_align;

  imu_raw_to_real_t         lsb;

  imu_data_t                data;

#if USE_MADGWICK == 1
  madgwick_t                filter;
#else
  mahony_t                  filter;
#endif
  float                     update_rate;
} imu_t;

extern void imu_init(imu_t* imu, float hz);
extern void imu_update(imu_t* imu);

extern void imu_gyro_calibration_start(imu_t* imu);
extern void imu_gyro_calibration_finish(imu_t* imu);
extern void imu_gyro_get_calibration(imu_t* imu, float data[3]);

extern void imu_mag_calibration_start(imu_t* imu);
extern void imu_mag_calibration_finish(imu_t* imu);
extern void imu_mag_get_calibration(imu_t* imu, float bias[3], float scale[3]);

extern void imu_accel_calibration_init(imu_t* imu);
extern void imu_accel_calibration_step_start(imu_t* imu);
extern void imu_accel_calibration_step_stop(imu_t* imu);
extern bool imu_accel_calibration_finish(imu_t* imu);
extern void imu_accel_get_calibration(imu_t* imu, float off[3], float scale[3]);

#endif /* !__IMU_DEF_H__ */
