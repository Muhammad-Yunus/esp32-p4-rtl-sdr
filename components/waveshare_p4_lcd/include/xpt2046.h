#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* Backward-compat: wraps GT911 I2C touch as if it were XPT2046 SPI.
 * Raw coordinates are mapped to 0..4095 range (matching original 12-bit). */
esp_err_t xpt2046_init(void);
bool xpt2046_pressed(void);
bool xpt2046_read_raw(uint16_t *x, uint16_t *y, uint16_t *z);
