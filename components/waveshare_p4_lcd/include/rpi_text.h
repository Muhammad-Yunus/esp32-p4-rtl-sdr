#pragma once
#include <stdint.h>

int rpi_lcd_text(int x, int y, const char *s, uint16_t fg, uint16_t bg, int scale);
int rpi_lcd_textf(int x, int y, uint16_t fg, uint16_t bg, int scale, const char *fmt, ...);
