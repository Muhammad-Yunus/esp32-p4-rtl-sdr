# Legacy display shim: provides rpi_lcd.c + compatibility headers.
# All real display/touch code lives in waveshare_p4_lcd.
idf_component_register(
    SRCS "rpi_lcd.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer nvs_flash esp_driver_i2c waveshare_p4_lcd
)
