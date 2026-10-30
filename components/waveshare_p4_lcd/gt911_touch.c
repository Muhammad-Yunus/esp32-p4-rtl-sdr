/*
 * GT911 capacitive touch driver for ESP32-P4-WIFI6-Touch-LCD-4B
 * I2C (400kHz) to GT911 address 0x5D, polling mode.
 *
 * GT911 registers (0x814E base):
 *   0x00 - Touch status (bit[3:0] = number of active touch points)
 *   0x01-0x05 - Point 1 data block:
 *               0x01-0x02: X (12-bit, high nibble in 0x01, low 8 in 0x02)
 *               0x03-0x04: Y (12-bit)
 *               0x05: event (0=in, 1=out, 2=up, 3=down)
 *   0x06-0x0A - Point 2 data block, ... up to 5 points
 *
 * Native GT911 resolution is ~800x480; we report values scaled to
 * 0..4095 (matching the original XPT2046 12-bit range) so the saved
 * calibration coefficients remain valid after the migration.
 */
#include "ws_lcd.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "gt911";
#define GT911_REG_STATUS  0x814E   /* touch status + first point block */
#define GT911_REGS_PER_PT 5
#define GT911_MAX_PT      5

/* Scale: GT911 native ~800x480 -> 12-bit equiv 0..4095 */
#define GT911_SCALE_X     ((float)4095.0f / 800.0f)
#define GT911_SCALE_Y     ((float)4095.0f / 480.0f)

static i2c_port_t s_i2c_port = I2C_NUM_0;
static bool s_inited = false;

static void read_gt911(uint8_t *dst, size_t n)
{
    uint8_t reg = (uint8_t)(GT911_REG_STATUS & 0xFF);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_write(cmd, &reg, 1, true);
    i2c_master_read(cmd, dst, n, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
}

static uint8_t read_status(void)
{
    uint8_t buf[2] = {0};
    read_gt911(buf, 2);
    return buf[0];
}

static bool wait_for_ready(int timeout_ms)
{
    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    while (esp_timer_get_time() < deadline) {
        uint8_t st = read_status();
        if ((st & 0x80) == 0) return true;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return false;
}

esp_err_t ws_touch_init_i2c(void)
{
    if (s_inited) return ESP_OK;

    /* GT911 reset via GPIO */
    gpio_config_t rst_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << WS_PIN_TP_RST),
    };
    esp_err_t ret = gpio_config(&rst_cfg);
    if (ret != ESP_OK) return ret;

    gpio_set_level(WS_PIN_TP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(WS_PIN_TP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* I2C init */
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = WS_PIN_TP_SDA,
        .scl_io_num = WS_PIN_TP_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = WS_TOUCH_I2C_FREQ,
    };
    ret = i2c_param_config(I2C_NUM_0, &cfg);
    if (ret != ESP_OK) return ret;
    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) return ret;

    /* Verify GT911 responds */
    uint8_t id_buf[16];
    memset(id_buf, 0, sizeof(id_buf));
    read_gt911(id_buf, 16);
    uint8_t st = id_buf[0];
    ESP_LOGI(TAG, "GT911 status=0x%02X after init", st);
    if (st & 0x80) {
        ESP_LOGW(TAG, "GT911 may not be responding (STATUS bit set)");
    }

    s_inited = true;
    ESP_LOGI(TAG, "GT911 ready on I2C%d (SDA=%d, SCL=%d)", s_i2c_port, WS_PIN_TP_SDA, WS_PIN_TP_SCL);
    return ESP_OK;
}

int ws_touch_read(ws_touch_t out[], int max_points)
{
    if (!s_inited) return 0;
    if (max_points <= 0) return 0;

    uint8_t status = read_status();
    int count = status & 0x0F;
    if (count <= 0) return 0;
    if (count > max_points) count = max_points;

    /* Read full point blocks (status + up to 5*5 bytes) */
    uint8_t buf[1 + GT911_MAX_PT * GT911_REGS_PER_PT];
    memset(buf, 0, sizeof(buf));
    read_gt911(buf, 1 + count * GT911_REGS_PER_PT);

    int idx = 0;
    for (int i = 0; i < count; i++) {
        int off = 1 + i * GT911_REGS_PER_PT;
        uint16_t raw_x = ((uint16_t)(buf[off] >> 4)) | (((uint16_t)buf[off + 1]) << 4);
        uint16_t raw_y = ((uint16_t)(buf[off + 2] >> 4)) | (((uint16_t)buf[off + 3]) << 4);
        (void)buf[off + 4];  /* event byte */

        /* Scale to 12-bit equivalent range (0..4095) */
        int16_t sx = (int16_t)((float)raw_x * GT911_SCALE_X);
        int16_t sy = (int16_t)((float)raw_y * GT911_SCALE_Y);

        /* Map to logical pixel coordinates */
        int px = (int)((int64_t)sx * LCD_W / 4095);
        int py = (int)((int64_t)sy * LCD_H / 4095);
        if (px < 0) px = 0;
        if (px >= LCD_W) px = LCD_W - 1;
        if (py < 0) py = 0;
        if (py >= LCD_H) py = LCD_H - 1;

        out[idx].x = px;
        out[idx].y = py;
        out[idx].id = i;
        out[idx].valid = true;
        idx++;
    }
    return idx;
}
