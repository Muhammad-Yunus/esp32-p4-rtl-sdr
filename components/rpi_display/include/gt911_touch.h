/*
 * GT911 capacitive touch driver header
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "ws_lcd.h"

/* Initialize GT911 touch controller via I2C */
esp_err_t ws_touch_init_i2c(void);

/* Read touch points. Returns count of active points. */
int ws_touch_read(ws_touch_t out[], int max_points);
