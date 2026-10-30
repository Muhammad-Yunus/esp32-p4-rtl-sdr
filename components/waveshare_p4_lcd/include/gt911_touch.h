/*
 * GT911 capacitive touch driver header
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "ws_lcd.h"

esp_err_t ws_touch_init_i2c(void);
int ws_touch_read(ws_touch_t out[], int max_points);
