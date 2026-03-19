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

# Define the source directory clearly
set(WEB_DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../web-front/dist")

# Force the SPIFFS image to wait for the npm build target
# We remove the 'if(EXISTS)' because 'web_ui_build' will CREATE it
spiffs_create_partition_image(storage ${WEB_DIST_DIR} 
                              FLASH_IN_PROJECT 
                              DEPENDS web_ui_build)
