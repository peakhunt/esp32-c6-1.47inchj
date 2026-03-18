set(srcs "app_main.c"
         "st7789_lcd.c"
         "lvgl_port_intf.c"
         "my_lvgl_app.c"
         "imu_task.c"
         "wifi.c"
         "web_server.c"
         "mount.c")

set(includes ".")


idf_component_register(SRCS ${srcs}
                       REQUIRES imu esp_app_format esp_lcd
                                    esp_driver_spi esp_driver_tsens
                                    esp_wifi esp_eth esp-tls esp_netif esp_http_server nvs_flash
                                    spiffs fatfs
                       INCLUDE_DIRS ${includes})


set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../web-front")
file(GLOB_RECURSE WEB_FILES "${WEB_SRC_DIR}/dist/*")

if(EXISTS ${WEB_SRC_DIR}/dist)
  spiffs_create_partition_image(storage ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
  message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()
