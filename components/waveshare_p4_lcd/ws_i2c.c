#include "ws_i2c.h"
#include "ws_lcd.h"
#include "esp_log.h"

static const char *TAG = "i2c";
static i2c_master_bus_handle_t s_bus;

esp_err_t ws_i2c_init(void)
{
    if (s_bus) return ESP_OK;

    i2c_master_bus_config_t cfg = {
        .i2c_port = WS_TP_I2C_PORT,
        .sda_io_num = WS_PIN_TP_SDA,
        .scl_io_num = WS_PIN_TP_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&cfg, &s_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C%d init failed: %s", WS_TP_I2C_PORT, esp_err_to_name(ret));
        s_bus = NULL;
        return ret;
    }
    ESP_LOGI(TAG, "I2C%d ready (SDA=%d, SCL=%d)", WS_TP_I2C_PORT, WS_PIN_TP_SDA, WS_PIN_TP_SCL);
    return ESP_OK;
}

i2c_master_bus_handle_t ws_i2c_bus(void)
{
    return s_bus;
}
