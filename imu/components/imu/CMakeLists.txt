set(srcs "mahony.c"
         "madgwick.c"
         "mpu9250.c"
         "mpu9250_i2c.c"
         "imu.c"
         "sensor_calib.c"
         "gyro_calibration.c"
         "mag_calibration.c"
         "accel_calibration.c")

set(includes ".")

idf_component_register(
    SRCS ${srcs}
    REQUIRES esp_driver_i2c esp_driver_gpio
    INCLUDE_DIRS ${includes}
)
