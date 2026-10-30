/*
 * Legacy pins.h — compatibility shim for rpi_display component
 *
 * The original XPT2046 driver referenced PIN_TP_IRQ, PIN_TP_CS, TP_SPI_HZ,
 * LCD_SPI_HOST, LCD_W, LCD_H, LCD_LONG, LCD_SHORT. These are now handled
 * by ws_lcd.h / ws_panel_config.h; this header defines compatible symbols
 * so the rpi_display code compiles unchanged.
 */
#pragma once
#include "ws_lcd.h"

/* I2C touch controller (GT911) — no SPI pins needed */
#define PIN_TP_IRQ     WS_PIN_TP_INT
#define PIN_TP_CS      (-1)
#define TP_SPI_HZ      (0)
#define LCD_SPI_HOST   (0)

/* Screen dimensions — match ws_lcd.h */
#define LCD_LONG       WS_LCD_LONG
#define LCD_SHORT      WS_LCD_SHORT
