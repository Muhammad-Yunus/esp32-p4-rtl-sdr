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

## ADR-002: Use GT911 I2C Touch Driver

    Status: Accepted
    Date: 2026-01-01

    Context:
      The current touch controller is XPT2046 (resistive, SPI, raw ADC values).
      The target board uses GT911 (capacitive, I2C, pixel coordinates).

    Decision:
      Implement a custom GT911 driver using ESP-IDF I2C master API.
      No calibration needed -- GT911 returns pixel coordinates directly.

    Consequences:
      + No calibration overhead
      + Faster touch response (interrupt-driven)
      - Different API from xpt2046.c -- UI layer needs update
      - Capacitive touch doesn't support press-and-hold the same way

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
