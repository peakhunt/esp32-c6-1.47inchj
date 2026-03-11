set(srcs "app_main.c"
         "st7789_lcd.c"
         "lvgl_port_intf.c"
         "my_lvgl_app.c")

set(includes ".")

idf_component_register(SRCS ${srcs}
                       REQUIRES esp_app_format esp_lcd esp_driver_spi esp_driver_tsens
                       INCLUDE_DIRS ${includes})

lvgl_port_create_c_image("images/page0_ui.png" "images/" "ARGB8888" "NONE")
lvgl_port_create_c_image("images/glow.png" "images/" "ARGB8888" "NONE")
lvgl_port_create_c_image("images/thunder.png" "images/" "ARGB8888" "NONE")
lvgl_port_create_c_image("images/power.png" "images/" "ARGB8888" "NONE")
lvgl_port_add_images(${COMPONENT_LIB} "images/")
