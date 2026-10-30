# Architecture Decision Records (ADR)

## ADR-001: Use ESP-IDF esp_lcd + MIPI DSI for Display Driver

    Status: Accepted
    Date: 2026-01-01

    Context:
      The current display driver (rpi_lcd.c) uses raw SPI to drive an ILI9486
      panel. The target board uses a ST7703 panel with MIPI DSI 2-lane interface.

    Decision:
      Use ESP-IDF's esp_lcd component with esp_lcd_mipi_dsi and
      esp_lcd_st7703 subcomponents. Follow the pattern from
      07_color_panel example in the Waveshare ESP-IDF package.

    Consequences:
      + Hardware-accelerated display (DMA transfers)
      + Reference implementation available from Waveshare
      - Different API from current rpi_lcd.c -- requires wrapper
      - MIPI DSI initialization is complex (LDO, PHY config)

---

## ADR-002: Use Official GT911 Component on a Shared I2C Bus

    Status: Accepted (supersedes the custom-driver decision)
    Date: 2026-10-30

    Context:
      The current touch controller is XPT2046 (resistive, SPI, raw ADC values).
      The target board uses GT911 (capacitive, I2C, pixel coordinates).
      The ES8311 audio codec is on the same physical I2C bus (GPIO7/8).
      A custom GT911 driver using the legacy driver/i2c.h API could not coexist
      with the i2c_master bus created by the audio codec on the same port.

    Decision:
      Use espressif/esp_lcd_touch_gt911 (v1.2.1) with esp_lcd_panel_io_i2c.
      Create one shared i2c_master bus in ws_i2c.c and let both GT911 and
      ES8311 acquire devices from it. Return GT911 pixel coordinates directly;
      keep the 5-point affine calibration as an optional correction path with
      a 1:1 identity default.

    Consequences:
      + Maintained upstream driver; no custom register handling
      + Single I2C bus owner avoids legacy/new driver conflicts
      + Touch works out of the box without calibration
      - Adds esp_lcd_touch + esp_lcd_touch_gt911 component dependencies
      - xpt2046.c remains only as a thin compatibility shim for ui.c
      - Optional calibration still needs a hold-at-boot gesture

---

## ADR-003: Keep Custom Drawing Engine (No LVGL)

    Status: Accepted (per user decision)
    Date: 2026-01-01

    Context:
      LVGL was considered as an alternative to the custom drawing engine.
      The user explicitly chose to keep the custom approach.

    Decision:
      Continue using rpi_canvas.c and rpi_text.c for all drawing operations.
      These modules are resolution-agnostic and require minimal changes.

    Consequences:
      + Minimal code changes for drawing logic
      + Lightweight memory footprint
      - No widget library -- all UI must be hand-drawn
      - Touch handling must be implemented manually

---

## ADR-004: RTL-SDR V3 Only (FT231XS)

    Status: Accepted (per user decision)
    Date: 2026-01-01

    Context:
      The target hardware has RTL-SDR V3 dongle connected. V4 (VP2231) is
      not required for this migration.

    Decision:
      Focus on FT231XS chip support (already in librtlsdr). No changes needed
      to sdr.c or rtl_usb/.

    Consequences:
      + No USB chip driver changes needed
      - V4 dongles won't work (but out of scope)
