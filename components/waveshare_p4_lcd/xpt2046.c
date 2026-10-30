/*
 * XPT2046 -> GT911 compatibility shim
 *
 * The original code used an XPT2046 resistive touch controller (SPI).
 * This shim maps those calls to the GT911 capacitive touch driver (I2C).
 *
 * XPT2046 raw values are 12-bit (0..4095). GT911 is polled and scaled
 * to the same range so the saved calibration coefficients remain valid.
 */

#include "ws_lcd.h"
#include "gt911_touch.h"
#include "touch_cal.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "xpt2046";
static bool s_inited = false;
static bool s_pressed = false;
static int  s_px = 0, s_py = 0;
static touch_cal_t s_cal_cached;

esp_err_t xpt2046_init(void)
{
    if (s_inited) return ESP_OK;

    esp_err_t ret = ws_touch_init_i2c();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GT911 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Try to load saved calibration */
    float max_err = 0;
    touch_cal_load(&s_cal_cached, &max_err);
    ESP_LOGI(TAG, "Touch init done (GT911 I2C, cal err=%.0fpx)", max_err);
    s_inited = true;
    return ESP_OK;
}

bool xpt2046_pressed(void)
{
    return s_pressed;
}

bool xpt2046_read_raw(uint16_t *x, uint16_t *y, uint16_t *z)
{
    ws_touch_t pts[5];
    int n = ws_touch_read(pts, 5);

    if (n <= 0) {
        s_pressed = false;
        return false;
    }

    /* Scale GT911 pixel coords to XPT2046-style 12-bit range (0..4095) */
    uint16_t rx = (uint16_t)((int64_t)pts[0].x * 4095 / LCD_W);
    uint16_t ry = (uint16_t)((int64_t)pts[0].y * 4095 / LCD_H);

    s_px = pts[0].x;
    s_py = pts[0].y;
    s_pressed = true;
    if (x) *x = rx;
    if (y) *y = ry;
    if (z) *z = 1024;
    return true;
}
