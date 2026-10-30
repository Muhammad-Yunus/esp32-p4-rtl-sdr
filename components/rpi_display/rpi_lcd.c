/*
 * rpi_lcd compatibility layer — wraps ws_lcd_dsi for the rpi_display API
 * Used by both rpi_text.c and the touch_cal shim.
 */
#include "rpi_lcd.h"
#include "ws_lcd.h"

esp_err_t rpi_lcd_init(void)
{
    return ws_lcd_init();
}

esp_err_t rpi_lcd_fill_rect(int x, int y, int w, int h, uint16_t rgb565)
{
    return ws_lcd_fill_rect(x, y, w, h, rgb565);
}

esp_err_t rpi_lcd_blit(int x, int y, int w, int h, const uint16_t *pix)
{
    return ws_lcd_blit(x, y, w, h, pix);
}

esp_err_t rpi_lcd_set_window(int x0, int y0, int x1, int y1)
{
    (void)x0; (void)y0; (void)x1; (void)y1;
    return ESP_OK;
}

esp_err_t rpi_lcd_push_pixels_be(const uint8_t *be_bytes, size_t nbytes)
{
    (void)be_bytes; (void)nbytes;
    return ESP_OK;
}

spi_host_device_t rpi_lcd_spi_host(void)
{
    return (spi_host_device_t)0;
}

esp_err_t rpi_lcd_set_portrait(bool on)
{
    return ws_lcd_set_portrait(on);
}
