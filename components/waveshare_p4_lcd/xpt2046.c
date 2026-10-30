/*
 * XPT2046 -> GT911 compatibility shim
 *
 * The original code used an XPT2046 resistive touch controller (SPI).
 * This shim maps those calls to the GT911 capacitive touch driver (I2C)
 * and returns GT911 coordinates directly in panel pixels.
 */

#include "ws_lcd.h"
#include "gt911_touch.h"
#include "esp_log.h"

static const char *TAG = "xpt2046";
static bool s_inited = false;
static bool s_pressed = false;

esp_err_t xpt2046_init(void)
{
    if (s_inited) return ESP_OK;

    esp_err_t ret = ws_touch_init_i2c();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GT911 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Touch init done (GT911 I2C)");
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

    s_pressed = true;
    if (x) *x = (uint16_t)pts[0].x;
    if (y) *y = (uint16_t)pts[0].y;
    if (z) *z = 1024;
    return true;
}
