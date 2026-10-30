# Waveshare ESP32-P4-WIFI6-Touch-LCD-4B display driver
# 4" 720x720 MIPI DSI (ST7703) + GT911 capacitive touch (I2C)
idf_component_register(
    SRCS "ws_lcd_dsi.c" "ws_i2c.c" "gt911_touch.c" "touch_cal.c" "rpi_text.c" "rpi_canvas.c" "xpt2046.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_lcd_st7703 nvs_flash esp_driver_ledc esp_driver_i2c esp_hw_support esp_timer
)
