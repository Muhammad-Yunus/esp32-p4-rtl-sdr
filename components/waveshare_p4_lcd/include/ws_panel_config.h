#pragma once
/* Panel and touch pin map for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B (720x720 MIPI DSI + GT911 I2C).
 *
 * LCD: 2-lane MIPI DSI, 720x720 IPS ST7703 (or equivalent)
 * Touch: GT911 5-point capacitive I2C
 *
 * Pin table (from Waveshare 4B schematic / docs):
 *   MIPI DSI: dedicated MIPI DSI pins (hardware fixed, no GPIO config needed)
 *     RESET     GPIO27
 *     BL_EN     GPIO33
 *     BL_PWM    GPIO26
 *
 *   GT911 Touch (I2C):
 *     SDA       GPIO7
 *     SCL       GPIO8
 *     RST       GPIO23
 *     TP_INT    TP2 (test point, not GPIO)
 *
 * This header is included by ws_lcd.h which provides the panel dimension macros.
 * The Kconfig-driven CONFIG_ values used here require sdkconfig.h to be pre-included.
 */

/* MIPI DSI bus index (ESP32-P4 has DSI0) */
#define WS_LCD_DSI_BUS       0
#define WS_LCD_DSI_LANES     2

/* Pin assignments from Waveshare 4B schematic (Kconfig-overridable) */
#define WS_PIN_LCD_RST       CONFIG_WAVESHARE_P4_LCD_PIN_RST
#define WS_PIN_BL_EN         CONFIG_WAVESHARE_P4_LCD_PIN_BL_EN
#define WS_PIN_BL_PWM        CONFIG_WAVESHARE_P4_LCD_PIN_BL_PWM
#define WS_PIN_TP_SDA        CONFIG_WAVESHARE_P4_LCD_PIN_TP_SDA
#define WS_PIN_TP_SCL        CONFIG_WAVESHARE_P4_LCD_PIN_TP_SCL
#define WS_PIN_TP_RST        CONFIG_WAVESHARE_P4_LCD_PIN_TP_RST

#define WS_LCD_BL_ON_LEVEL   1
#define WS_LCD_BL_OFF_LEVEL  0

/* I2C bus ID for GT911 */
#define WS_TP_I2C_PORT       0
#define WS_TOUCH_I2C_FREQ    400000
#define GT911_I2C_ADDR       0x5D

#ifndef WS_TOUCH_I2C_FREQ
#define WS_TOUCH_I2C_FREQ    400000
#endif

/* Touch INT pin — not wired on P4 4B; define as -1 for legacy pins.h compat */
#ifndef WS_PIN_TP_INT
#define WS_PIN_TP_INT        (-1)
#endif
