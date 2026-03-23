#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mpu9250.h"
#include "imu.h"
#include "imu_task.h"
#include "web_server.h"
#include "imu_config.h"

static const char *TAG = "imu_task";

static mpu9250_t              _mpu9250;
static imu_t                  _imu;
static SemaphoreHandle_t      _mutex;

volatile uint32_t             _sample_rate = 0;

static int64_t                      _cal_start_time = 0;
static int64_t                      _cal_duration = 0;
static const int64_t                CAL_DURATION_US = 60 * 1000000; // 60 seconds in microseconds
static const int64_t                CAL_DURATION_ACCEL_US = 10 * 1000000; // 60 seconds in microseconds
static imu_task_calib_complete_cb   _calib_cb = NULL;
static void*                        _calib_cb_arg = NULL;


static inline void
measure_sample_rate(void)
{
  static bool first = true;
  static uint32_t sample_count = 0;
  static uint64_t last_time = 0;
  uint64_t now;

  if(first)
  {
    first = false;
    last_time = esp_timer_get_time(); // microsecond
  }
  sample_count++;
  now = esp_timer_get_time();

  if (now - last_time >= 1000000ULL)
  {
    _sample_rate = (uint32_t)((float)sample_count / ((now - last_time) / 1000000.0f));
    // ESP_LOGI(TAG, "Sampling rate: %ld Hz", _sample_rate);
    sample_count = 0;
    last_time = now;
  }
}

static inline void
throttled_imu_data_send(void)
{
  static int skip_count = 0;

  // IMU task's target is 500Mz.
  // send data at 50Hz
  if (++skip_count >= 10)
  { 
    imu_telemetry_pkt_t pkt;
    extern volatile uint32_t cpu_usage;

    pkt.roll        = _imu.data.orientation[0];
    pkt.pitch       = _imu.data.orientation[1];
    pkt.yaw         = _imu.data.orientation[2];

    pkt.gx          = _imu.data.gyro[0];
    pkt.gy          = _imu.data.gyro[1];
    pkt.gz          = _imu.data.gyro[2];

    pkt.ax          = _imu.data.accel[0];
    pkt.ay          = _imu.data.accel[1];
    pkt.az          = _imu.data.accel[2];

    pkt.mx          = _imu.data.mag[0];
    pkt.my          = _imu.data.mag[1];
    pkt.mz          = _imu.data.mag[2];

    pkt.cpu_usage   = cpu_usage;
    pkt.sample_rate = _sample_rate;
    pkt.i2c_tx      = _mpu9250.num_transactions; 
    pkt.i2c_fail    = _mpu9250.num_failed;

    ws_broadcast_imu_update(&pkt);
    skip_count = 0;
  }
}

static void
imu_task_start_calib_timer(uint64_t duration)
{
  _cal_duration = duration;
  _cal_start_time = esp_timer_get_time();
}

static bool
imu_task_handle_calibration_timer(void)
{
  int64_t current_time = esp_timer_get_time();
  int64_t elapsed = current_time - _cal_start_time;

  if(elapsed >= _cal_duration)
  {
    switch(_imu.mode)
    {
    case imu_mode_mag_calibrating:
      imu_mag_calibration_finish(&_imu);
      ESP_LOGI(TAG, "finishing mag calibration %d %d %d %d %d %d",
          _imu.cal.mag_bias[0],
          _imu.cal.mag_bias[1],
          _imu.cal.mag_bias[2],
          _imu.cal.mag_scale[0],
          _imu.cal.mag_scale[1],
          _imu.cal.mag_scale[2]);
      imu_config_update_mag_calib(_imu.cal.mag_bias, _imu.cal.mag_scale);
      break;

    case imu_mode_gyro_calibrating:
      imu_gyro_calibration_finish(&_imu);
      ESP_LOGI(TAG, "finishing gyro calibration %d %d %d",
          _imu.cal.gyro_off[0],
          _imu.cal.gyro_off[1],
          _imu.cal.gyro_off[2]);
      imu_config_update_gyro_calib(_imu.cal.gyro_off);
      break;

    case imu_mode_accel_calibrating:
      imu_accel_calibration_step_stop(&_imu);
      ESP_LOGI(TAG, "stopping accel sensor step calibration");
      break;

    default:
      break;
    }
    return true;
  }
  return false;
}

static void
imu_task_init_all(void)
{
  imu_engine_config_t e_cfg;

  imu_config_get_imu_engine_config(&e_cfg);

  xSemaphoreTake(_mutex, portMAX_DELAY);

  imu_init(&_imu, &e_cfg, 500);
  mpu9250_init(&_mpu9250, MPU9250_Accelerometer_8G, MPU9250_Gyroscope_1000s, &_imu.lsb); 

  imu_config_get_sensor_config(&_imu.cal);

  ESP_LOGI(TAG, "ahrs alg: %s", e_cfg.ahrs_mode == IMU_AHRS_MODE_MAHONY ? "mahony" : "madgwick");
  ESP_LOGI(TAG, "beta: %.3f", e_cfg.madgwick_beta);
  ESP_LOGI(TAG, "twoKp: %.3f, twoKi: %.3f", e_cfg.mahony_kp, e_cfg.mahony_ki);
  ESP_LOGI(TAG, "accel_off %d, %d, %d", _imu.cal.accel_off[0], _imu.cal.accel_off[1], _imu.cal.accel_off[2]);
  ESP_LOGI(TAG, "accel_scale %d, %d, %d", _imu.cal.accel_scale[0], _imu.cal.accel_scale[1], _imu.cal.accel_scale[2]);
  ESP_LOGI(TAG, "gyro_off %d, %d, %d", _imu.cal.gyro_off[0], _imu.cal.gyro_off[1], _imu.cal.gyro_off[2]);
  ESP_LOGI(TAG, "mag_bias %d, %d, %d", _imu.cal.mag_bias[0], _imu.cal.mag_bias[1], _imu.cal.mag_bias[2]);
  ESP_LOGI(TAG, "mag_scale %d, %d, %d", _imu.cal.mag_scale[0], _imu.cal.mag_scale[1], _imu.cal.mag_scale[2]);
  ESP_LOGI(TAG, "mag_declination %f", _imu.cal.mag_declination);

  xSemaphoreGive(_mutex);
}

static void
imu_task(void *arg)
{
  ESP_LOGI(TAG, "Starting IMU task...");

  imu_task_init_all();

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(2); // 2 ms → 500 Hz
  bool shouldNotify = false;

  imu_task_calib_complete_cb   calib_cb;
  void*                        calib_cb_arg;

  while (1)
  {
    mpu9250_read_all(&_mpu9250, &_imu.raw);

    xSemaphoreTake(_mutex, portMAX_DELAY);
    imu_update(&_imu);

    if(_imu.mode !=imu_mode_normal)
    {
      if(imu_task_handle_calibration_timer() == true)
      {
        calib_cb      = _calib_cb;
        calib_cb_arg  = _calib_cb_arg;
        shouldNotify  = true;
      }
    }
    xSemaphoreGive(_mutex);

    if(shouldNotify)
    {
      shouldNotify = false;
      calib_cb(calib_cb_arg);
    }

    measure_sample_rate();

    throttled_imu_data_send();

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void
imu_task_init(void)
{
  _mutex = xSemaphoreCreateMutex();
  xTaskCreate(imu_task, "imu_task", 4096, NULL, 15, NULL);
}

void
imu_task_get_data(imu_data_t* data)
{
  xSemaphoreTake(_mutex, portMAX_DELAY);
  memcpy(data, &_imu.data, sizeof(imu_data_t));
  xSemaphoreGive(_mutex);
}

void
imu_task_get_attitude(float* roll, float* pitch, float* yaw)
{
  xSemaphoreTake(_mutex, portMAX_DELAY);
  *roll = _imu.data.orientation[0];
  *pitch = _imu.data.orientation[1];
  *yaw = _imu.data.orientation[2];
  xSemaphoreGive(_mutex);
}

void
imu_task_start_gyro_calibration(imu_task_calib_complete_cb cb, void* data)
{
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _calib_cb = cb;
  _calib_cb_arg = data;
  imu_gyro_calibration_start(&_imu);
  imu_task_start_calib_timer(CAL_DURATION_US);
  ESP_LOGI(TAG, "starting gyro calibration");
  xSemaphoreGive(_mutex);
}

void
imu_task_get_gyro_calibration(float offset[3])
{
  xSemaphoreTake(_mutex, portMAX_DELAY);
  imu_gyro_get_calibration(&_imu, offset);
  xSemaphoreGive(_mutex);
}

void
imu_task_start_mag_calibration(imu_task_calib_complete_cb cb, void* data)
{
  xSemaphoreTake(_mutex, portMAX_DELAY);
  _calib_cb = cb;
  _calib_cb_arg = data;
  imu_mag_calibration_start(&_imu);
  imu_task_start_calib_timer(CAL_DURATION_US);
  ESP_LOGI(TAG, "starting mag calibration");
  xSemaphoreGive(_mutex);
}

void
imu_task_get_mag_calibration(float bias[3], float scale[3])
{
  xSemaphoreTake(_mutex, portMAX_DELAY);
  
  imu_mag_get_calibration(&_imu, bias, scale);
  
  xSemaphoreGive(_mutex);
}

void
imu_task_start_accel_calibration(bool reset, imu_task_calib_complete_cb cb, void* data)
{
  xSemaphoreTake(_mutex, portMAX_DELAY);

  _calib_cb = cb;
  _calib_cb_arg = data;

  if(reset)
  {
    ESP_LOGI(TAG, "resetting accel sensor calibration");
    imu_accel_calibration_init(&_imu);
  }

  imu_accel_calibration_step_start(&_imu);
  ESP_LOGI(TAG, "starting accel sensor step calibration");
  imu_task_start_calib_timer(CAL_DURATION_ACCEL_US);

  xSemaphoreGive(_mutex);
}

bool
imu_task_finish_accel_calibration(void)
{
  bool status;
  xSemaphoreTake(_mutex, portMAX_DELAY);
  status = imu_accel_calibration_finish(&_imu);
  ESP_LOGI(TAG, "finishing accel sensor calibration, status %d", status);
  if (status)
  {
    ESP_LOGI(TAG, "finishing accel calibration %d %d %d %d %d %d",
        _imu.cal.accel_off[0],
        _imu.cal.accel_off[1],
        _imu.cal.accel_off[2],
        _imu.cal.accel_scale[0],
        _imu.cal.accel_scale[1],
        _imu.cal.accel_scale[2]);
    imu_config_update_accel_calib(_imu.cal.accel_off, _imu.cal.accel_scale);
  }
  xSemaphoreGive(_mutex);
  return status;
}

void
imu_task_get_accel_calibration(float offset[3], float scale[3])
{
  xSemaphoreTake(_mutex, portMAX_DELAY);
  
  imu_accel_get_calibration(&_imu, offset, scale);
  
  xSemaphoreGive(_mutex);
}

void
imu_task_get_mag_dec(float* dec)
{
  xSemaphoreTake(_mutex, portMAX_DELAY);
  
  *dec = _imu.cal.mag_declination;
  
  xSemaphoreGive(_mutex);
}

void
imu_task_config_ahrs(imu_engine_config_t* cfg, float mag_dec)
{
  xSemaphoreTake(_mutex, portMAX_DELAY);
  
  _imu.cal.mag_declination = mag_dec;
  imu_config_engine(&_imu, cfg);

  imu_config_update_ahrs(cfg, mag_dec);
  
  xSemaphoreGive(_mutex);
}
