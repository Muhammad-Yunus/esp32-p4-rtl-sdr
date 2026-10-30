#pragma once
#include <stdint.h>
/* Off-screen RGB565 drawing (native byte order), pushed to the panel with rpi_lcd_blit.
 * Everything is clipped to the canvas. Text is transparent: only glyph pixels are written. */
typedef struct { uint16_t *px; int w, h; } rpi_canvas_t;

void rpi_cv_fill(const rpi_canvas_t *c, int x, int y, int w, int h, uint16_t col);
/* Rounded rectangle, radius 0..8. */
void rpi_cv_rrect(const rpi_canvas_t *c, int x, int y, int w, int h, int r, uint16_t col);
/* 8x16 font, scale 1 or 2. Returns x after the last glyph. */
int  rpi_cv_text(const rpi_canvas_t *c, int x, int y, const char *s, uint16_t fg, int scale);
int  rpi_cv_text_w(const char *s, int scale);
uint16_t rpi_mix565(uint16_t a, uint16_t b, int t);
