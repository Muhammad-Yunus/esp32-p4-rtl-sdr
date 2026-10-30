#include "gt911_touch.h"
#include "ws_i2c.h"
#include "esp_log.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"

static const char *TAG = "gt911";
static esp_lcd_panel_io_handle_t s_io;
static esp_lcd_touch_handle_t s_tp;

esp_err_t ws_touch_init_i2c(void)
{
    if (s_tp) return ESP_OK;

    esp_err_t ret = ws_i2c_init();
    if (ret != ESP_OK) return ret;

    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_cfg.scl_speed_hz = WS_TOUCH_I2C_FREQ;
    ret = esp_lcd_new_panel_io_i2c(ws_i2c_bus(), &io_cfg, &s_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C panel IO init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_W,
        .y_max = LCD_H,
        .rst_gpio_num = WS_PIN_TP_RST,
        .int_gpio_num = WS_PIN_TP_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    ret = esp_lcd_touch_new_i2c_gt911(s_io, &tp_cfg, &s_tp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GT911 init failed: %s", esp_err_to_name(ret));
        esp_lcd_panel_io_del(s_io);
        s_io = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "GT911 ready at 0x%02X on I2C%d", GT911_I2C_ADDR, WS_TP_I2C_PORT);
    return ESP_OK;
}

int ws_touch_read(ws_touch_t out[], int max_points)
{
    if (!s_tp || !out || max_points <= 0) return 0;

    if (esp_lcd_touch_read_data(s_tp) != ESP_OK) return 0;

    esp_lcd_touch_point_data_t pts[5];
    uint8_t count = 0;
    if (esp_lcd_touch_get_data(s_tp, pts, &count, 5) != ESP_OK) return 0;
    if (count > max_points) count = max_points;

    for (int i = 0; i < count; i++) {
        int x = pts[i].x;
        int y = pts[i].y;
        if (x < 0) x = 0;
        if (x >= LCD_W) x = LCD_W - 1;
        if (y < 0) y = 0;
        if (y >= LCD_H) y = LCD_H - 1;
        out[i].x = x;
        out[i].y = y;
        out[i].id = pts[i].track_id;
        out[i].valid = true;
    }
    return count;
}
