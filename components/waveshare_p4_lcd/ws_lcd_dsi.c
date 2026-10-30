/*
 * MIPI DSI panel driver for ESP32-P4-WIFI6-Touch-LCD-4B
 * 4" 720x720 ST7703 IPS display via 2-lane MIPI DSI
 */

#include "ws_panel_config.h"
#include "ws_lcd.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_st7703.h"
#include "esp_ldo_regulator.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static const char *TAG = "ws_lcd_dsi";

static esp_lcd_panel_handle_t s_panel = NULL;
static esp_lcd_dsi_bus_handle_t s_dsi_bus = NULL;
static esp_lcd_panel_io_handle_t s_dbi_io = NULL;
static bool s_portrait = false;
static int s_lcd_w = WS_LCD_W;
static int s_lcd_h = WS_LCD_H;
int ws_lcd_w = WS_LCD_W;
int ws_lcd_h = WS_LCD_H;

/* LEDC PWM config for backlight */
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_SPEED_MODE         LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_TIMER_BIT          LEDC_TIMER_10_BIT
#define LEDC_DUTY_RES           ((1 << LEDC_TIMER_BIT) - 1)
#define LEDC_FREQ_HZ            5000

/* NVS calibration key */
#define NVS_CAL_KEY "touch_cal"
#define NVS_CAL_MAX_VER 1

static int s_cal_a, s_cal_b, s_cal_c, s_cal_d, s_cal_e, s_cal_f;
static bool s_cal_valid = false;

static esp_err_t ws_lcd_init_dsi(void)
{
    esp_err_t ret;

    gpio_config_t bk_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << WS_PIN_BL_EN),
    };
    ret = gpio_config(&bk_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config BL_EN failed: %s", esp_err_to_name(ret));
        return ret;
    }
    gpio_set_level(WS_PIN_BL_EN, WS_LCD_BL_ON_LEVEL);

    esp_ldo_channel_handle_t ldo_phy = NULL;
    esp_ldo_channel_config_t ldo_phy_cfg = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    ret = esp_ldo_acquire_channel(&ldo_phy_cfg, &ldo_phy);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MIPI DSI PHY LDO acquire failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_dsi_bus_config_t bus_config = ST7703_PANEL_BUS_DSI_2CH_CONFIG();
    ret = esp_lcd_new_dsi_bus(&bus_config, &s_dsi_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MIPI DSI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_dbi_io_config_t dbi_config = ST7703_PANEL_IO_DBI_CONFIG();
    ret = esp_lcd_new_panel_io_dbi(s_dsi_bus, &dbi_config, &s_dbi_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DBI IO init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_dpi_panel_config_t dpi_config =
        ST7703_720_720_PANEL_60HZ_DPI_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);
    dpi_config.num_fbs = 1;

    st7703_vendor_config_t vendor_config = {
        .flags = {
            .use_mipi_interface = 1,
        },
        .mipi_config = {
            .dsi_bus = s_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = WS_PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };

    ret = esp_lcd_new_panel_st7703(s_dbi_io, &panel_config, &s_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ST7703 panel init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_reset(s_panel);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Panel reset failed: %s", esp_err_to_name(ret));
    }

    ret = esp_lcd_panel_init(s_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_disp_on_off(s_panel, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel display on failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ws_lcd_w = s_lcd_w;
    ws_lcd_h = s_lcd_h;

    ESP_LOGI(TAG, "MIPI DSI panel initialized: %dx%d", s_lcd_w, s_lcd_h);
    return ESP_OK;
}

static esp_err_t ws_lcd_init_lec(void)
{
    static bool s_lec_inited = false;
    if (s_lec_inited) return ESP_OK;

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_SPEED_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_BIT,
        .freq_hz = LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) return ret;

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_SPEED_MODE,
        .channel = LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = WS_PIN_BL_PWM,
        .duty = 0,
        .hpoint = 0,
        .flags = {
            .output_invert = 1,
        },
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret == ESP_OK) {
        s_lec_inited = true;
    }
    return ret;
}

esp_err_t ws_lcd_init(void)
{
    esp_err_t ret = ws_lcd_init_dsi();
    if (ret != ESP_OK) return ret;

    ret = ws_lcd_init_lec();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LEDC init failed: %s", esp_err_to_name(ret));
    } else {
        ws_lcd_set_brightness(100);
    }

    nvs_handle_t nvs;
    ret = nvs_open("board", NVS_READWRITE, &nvs);
    if (ret == ESP_OK) {
        uint8_t ver;
        if (nvs_get_u8(nvs, "cal_ver", &ver) == ESP_OK && ver == NVS_CAL_MAX_VER) {
            int32_t cal[6];
            if (nvs_get_i32(nvs, NVS_CAL_KEY, cal) == ESP_OK) {
                s_cal_a = cal[0]; s_cal_b = cal[1]; s_cal_c = cal[2];
                s_cal_d = cal[3]; s_cal_e = cal[4]; s_cal_f = cal[5];
                s_cal_valid = true;
                ESP_LOGI(TAG, "Touch calibration loaded");
            }
        }
        nvs_close(nvs);
    }

    return ESP_OK;
}

esp_err_t ws_lcd_set_portrait(bool on)
{
    if (s_panel == NULL) return ESP_ERR_INVALID_STATE;
    s_portrait = on;
    esp_lcd_panel_swap_xy(s_panel, on);
    if (on) {
        s_lcd_w = WS_LCD_H;
        s_lcd_h = WS_LCD_W;
    } else {
        s_lcd_w = WS_LCD_W;
        s_lcd_h = WS_LCD_H;
    }
    ws_lcd_w = s_lcd_w;
    ws_lcd_h = s_lcd_h;
    return ESP_OK;
}

esp_err_t ws_lcd_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (s_panel == NULL) return ESP_ERR_INVALID_STATE;
    if (x < 0 || y < 0 || w <= 0 || h <= 0) return ESP_OK;
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    if (x1 >= ws_lcd_w) x1 = ws_lcd_w - 1;
    if (y1 >= ws_lcd_h) y1 = ws_lcd_h - 1;
    w = x1 - x + 1;
    h = y1 - y + 1;

    /* Allocate a static buffer for solid-color fill */
    static uint16_t *s_fill_buf = NULL;
    static int s_fill_sz = 0;
    int needed = w * h;
    if (s_fill_sz < needed) {
        if (s_fill_buf) free(s_fill_buf);
        s_fill_buf = (uint16_t *)malloc((size_t)needed * sizeof(uint16_t));
        if (!s_fill_buf) return ESP_ERR_NO_MEM;
        s_fill_sz = needed;
    }
    /* Fill the buffer with the solid color */
    for (int i = 0; i < needed; i++) s_fill_buf[i] = color;
    return esp_lcd_panel_draw_bitmap(s_panel, x, y, x + w, y + h, s_fill_buf);
}

esp_err_t ws_lcd_blit(int x, int y, int w, int h, const uint16_t *pix)
{
    if (s_panel == NULL || pix == NULL) return ESP_ERR_INVALID_ARG;
    if (x < 0 || y < 0 || w <= 0 || h <= 0) return ESP_OK;
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    if (x1 >= ws_lcd_w) x1 = ws_lcd_w - 1;
    if (y1 >= ws_lcd_h) y1 = ws_lcd_h - 1;
    w = x1 - x + 1;
    h = y1 - y + 1;
    return esp_lcd_panel_draw_bitmap(s_panel, x, y, x + w, y + h, pix);
}

bool ws_lcd_backlight_on(void)
{
    bool was_on = (gpio_get_level(WS_PIN_BL_EN) == WS_LCD_BL_ON_LEVEL);
    gpio_set_level(WS_PIN_BL_EN, WS_LCD_BL_ON_LEVEL);
    ws_lcd_set_brightness(100);
    return was_on;
}

void ws_lcd_backlight_off(void)
{
    ws_lcd_set_brightness(0);
    gpio_set_level(WS_PIN_BL_EN, WS_LCD_BL_OFF_LEVEL);
}

void ws_lcd_set_brightness(int pct)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    if (ws_lcd_init_lec() == ESP_OK) {
        int duty = (int)((uint64_t)LEDC_DUTY_RES * pct / 100);
        ledc_set_duty(LEDC_SPEED_MODE, LEDC_CHANNEL, duty);
        ledc_update_duty(LEDC_SPEED_MODE, LEDC_CHANNEL);
    }
}

void ws_touch_calibrate(int raw_x, int raw_y, int *px, int *py)
{
    if (s_cal_valid) {
        int64_t vx = (int64_t)s_cal_a * raw_x + (int64_t)s_cal_b * raw_y + s_cal_c;
        int64_t vy = (int64_t)s_cal_d * raw_x + (int64_t)s_cal_e * raw_y + s_cal_f;
        *px = (int)(vx / 1000);
        *py = (int)(vy / 1000);
    } else {
        *px = raw_x;
        *py = raw_y;
    }
    if (*px < 0) *px = 0;
    if (*px >= ws_lcd_w) *px = ws_lcd_w - 1;
    if (*py < 0) *py = 0;
    if (*py >= ws_lcd_h) *py = ws_lcd_h - 1;
}

esp_err_t ws_touch_cal_save(const int points[][4], int n)
{
    if (n < 3) return ESP_ERR_INVALID_ARG;
    double sxx = 0, sxy = 0, syy = 0;
    double sxa = 0, sya = 0, sxb = 0, syb = 0;
    for (int i = 0; i < n; i++) {
        int rx = points[i][0], ry = points[i][1];
        int px = points[i][2], py = points[i][3];
        sxx += (double)rx * rx;
        sxy += (double)rx * ry;
        syy += (double)ry * ry;
        sxa += (double)rx * px;
        sya += (double)ry * px;
        sxb += (double)rx * py;
        syb += (double)ry * py;
    }
    double det = sxx * syy - sxy * sxy;
    if (fabs(det) < 1e-10) return ESP_ERR_INVALID_STATE;
    s_cal_a = (int)round((sxa * syy - sya * sxy) / det * 1000);
    s_cal_b = (int)round((sya * sxx - sxa * sxy) / det * 1000);
    s_cal_d = (int)round((sxb * syy - syb * sxy) / det * 1000);
    s_cal_e = (int)round((syb * sxx - sxb * sxy) / det * 1000);
    double cx = 0, cy = 0, cpx = 0, cpy = 0;
    for (int i = 0; i < n; i++) {
        cx += points[i][0]; cy += points[i][1];
        cpx += points[i][2]; cpy += points[i][3];
    }
    cx /= n; cy /= n; cpx /= n; cpy /= n;
    s_cal_c = (int)round(cpx * 1000 - s_cal_a * cx - s_cal_b * cy);
    s_cal_f = (int)round(cpy * 1000 - s_cal_d * cx - s_cal_e * cy);
    s_cal_valid = true;

    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("board", NVS_READWRITE, &nvs);
    if (ret != ESP_OK) return ret;
    nvs_set_u8(nvs, "cal_ver", NVS_CAL_MAX_VER);
    int32_t cal[6] = {s_cal_a, s_cal_b, s_cal_c, s_cal_d, s_cal_e, s_cal_f};
    ret = nvs_set_blob(nvs, NVS_CAL_KEY, cal, sizeof(cal));
    nvs_commit(nvs);
    nvs_close(nvs);
    ESP_LOGI(TAG, "Calibration saved");
    return ret;
}
