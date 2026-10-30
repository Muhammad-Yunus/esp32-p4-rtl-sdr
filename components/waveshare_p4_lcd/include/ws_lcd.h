/*
 * Waveshare ESP32-P4-WIFI6-Touch-LCD-4B display driver
 * 4" 720x720 MIPI DSI + GT911 I2C capacitive touch
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "ws_panel_config.h"

/* Resolution (runtime set from config or ws_panel_config.h defaults) */
#ifndef WS_LCD_W
#define WS_LCD_W          CONFIG_WAVESHARE_P4_LCD_PANEL_WIDTH
#define WS_LCD_H          CONFIG_WAVESHARE_P4_LCD_PANEL_HEIGHT
#define WS_LCD_LONG       CONFIG_WAVESHARE_P4_LCD_PANEL_WIDTH
#define WS_LCD_SHORT      CONFIG_WAVESHARE_P4_LCD_PANEL_HEIGHT
#endif

/* 720x720 square panel: portrait orientation is the default layout */
#define LCD_PORTRAIT      1

/* RGB565 colour macro — needed by all views */
#ifndef RGB565
#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
#endif

/* Backlight pins */
#define WS_PIN_BL_EN      CONFIG_WAVESHARE_P4_LCD_PIN_BL_EN
#define WS_PIN_BL_PWM     CONFIG_WAVESHARE_P4_LCD_PIN_BL_PWM
#define WS_LCD_BL_ON_LEVEL    1
#define WS_LCD_BL_OFF_LEVEL   0

/* Pin assignments */
#define WS_PIN_LCD_RST    CONFIG_WAVESHARE_P4_LCD_PIN_RST
#define WS_PIN_TP_SDA     CONFIG_WAVESHARE_P4_LCD_PIN_TP_SDA
#define WS_PIN_TP_SCL     CONFIG_WAVESHARE_P4_LCD_PIN_TP_SCL
#define WS_PIN_TP_RST     CONFIG_WAVESHARE_P4_LCD_PIN_TP_RST

/* Orientation */
extern int ws_lcd_w, ws_lcd_h;
#define LCD_W ws_lcd_w
#define LCD_H ws_lcd_h

/* Touch point */
typedef struct {
    int x, y;
    int id;
    bool valid;
} ws_touch_t;

esp_err_t ws_lcd_init(void);
esp_err_t ws_lcd_set_portrait(bool on);
esp_err_t ws_lcd_fill_rect(int x, int y, int w, int h, uint16_t color);
esp_err_t ws_lcd_blit(int x, int y, int w, int h, const uint16_t *pix);
int ws_touch_read(ws_touch_t out[], int max_points);
void ws_touch_calibrate(int raw_x, int raw_y, int *px, int *py);
esp_err_t ws_touch_cal_save(const int points[][4], int n);
bool ws_lcd_backlight_on(void);
void ws_lcd_backlight_off(void);
void ws_lcd_set_brightness(int pct);
