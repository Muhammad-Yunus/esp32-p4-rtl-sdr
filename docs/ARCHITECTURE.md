# Architecture — ESP32-P4 RTL-SDR Migration to ESP32-P4-WIFI6-Touch-LCD-4B

## 1. System Overview

The ESP32-P4 RTL-SDR project is a software-defined radio receiver application
that combines a custom UI framework with an ESP32-P4 MCU, an RTL-SDR USB dongle,
and a display/touch system. This document describes the architecture as-is and
the changes required for migration to the Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.

    +----------------------------------------------------------------------+
    |                     Application Layer (main/)                        |
    |  +----------+ +-----------+ +-----------+ +-----------+ +----------+  |
    |  | spectrum | | waterfall | | controls  | | amap/     | | ADS-B    |  |
    |  | view.c   | | view.c    | | panel.c   | | rtl_wifi  | | adsb_*   |  |
    |  +-----+----+ +-----+-----+ +-----+-----+ +-----+-----+ +----+-----+  |
    |        |               |               |               |             |
    |        +---------------+---------------+---------------+             |
    |                          |                                            |
    |                    +-----v------+                                    |
    |                    |  ui.c      |  <- main event loop & touch       |
    |                    |  main.c    |     handling & mode switching     |
    |                    +-----+------+                                    |
    +------------------------------|---------------------------------------+
    |                     Component Layer (components/)                      |
    |  +---------------+ +------------+ +-----------+ +-------------+       |
    |  |  rtl_usb      | |   sdr.c    | |   audio.c | |  sdcard.c   |       |
    |  |  (libusb)     | | + demod.c  | |  + rds.c  | |  + sdlog.c  |       |
    |  +-------+-------+ +-----+------+ +----+------+ +------+------+       |
    |           |                |             |              |             |
    |           +----------------+-------------+--------------+             |
    |                          |                                            |
    |                    +-----v------+                                    |
    |              +-----+------+   |                                    |
    |              | rpi_display |   |                                    |
    |              | (abstraction|   |                                    |
    |              |  layer)    |   |                                    |
    |              +-----+------+   |                                    |
    +--------------------|-----------+                                    |
    |                     Hardware Abstraction Layer                       |
    |                                                                      |
    |  +--------------------+    +--------------------+    +----------+    |
    |  | ESP32-P4 MCU      |    | USB OTG (P4)       |    | SD Card  |    |
    |  | + ESP32-C6        |    | -> RTL-SDR V3      |    | (GPIO)   |    |
    |  | (hosted Wi-Fi)    |    |                    |    |          |    |
    |  +--------------------+    +--------------------+    +----------+    |
    |                                                                      |
    |  +--------------------+    +--------------------+    +----------+    |
    |  | CURRENT BOARD:     |    | TARGET BOARD:      |    | AUDIO    |    |
    |  |                    |    |                    |    | unchanged|    |
    |  | ILI9486 SPI        | --> | ST7703 MIPI DSI    |    |          |    |
    |  | 480x320 portrait   |    | 720x720 square     |    | ES8311   |    |
    |  | XPT2046 SPI        |    | GT911 I2C          |    | + NS415 |    |
    |  | GPIO-based init    |    | I2C addr 0x5D      |    |          |    |
    |  +--------------------+    +--------------------+    +----------+    |
    +----------------------------------------------------------------------+

## 2. Module Decomposition

### 2.1 Display Abstraction (rpi_display/)
The display module provides a hardware-agnostic interface used by the UI layer.

**Current implementation (ILI9486 / 16-bit SPI):**
    rpi_lcd.c        -- SPI bus init, ILI9486 init sequence, blit/fill operations
    rpi_canvas.c     -- Software drawing primitives (fill rect, rounded rect, text)
    rpi_text.c       -- Text rendering using font8x16.h bitmap font
    xpt2046.c        -- SPI touch controller driver (raw ADC values)
    touch_cal.c      -- 5-point affine calibration: raw-to-screen coordinates
    pins.h           -- Board-specific pin definitions

**Target implementation (ST7703 / MIPI DSI 2-lane):**
    rpi_lcd.c        -- Rewrite: esp_lcd + MIPI DSI init, blit via DMA
    rpi_canvas.c     -- No change needed (resolution-agnostic)
    rpi_text.c       -- No change needed
    gt911_touch.c    -- New: I2C GT911 driver (replaces xpt2046.c)
                       (no calibration needed -- capacitive touch is pixel-accurate)
    gt911.h          -- New: touch event types (TAP, HOLD, SWIPE)
    pins.h           -- Update: new pin definitions

### 2.2 UI Layer (main/ui.c, main/view.c, main/panel.c)
- ui.c -- Main event loop: IQ processing -> spectrum -> view -> touch handling
- view.c -- All drawing logic for spectrum, waterfall, frequency readout, buttons
- panel.c -- Controls panel overlay (gain, volume, filter, squelch, zoom)

**Key constraints for 720x720 square screen:**
- Current layout assumes 480x320 (portrait, 3:2 ratio) or 320x480 (landscape)
- New screen is 720x720 (square, 1:1 ratio) -- layout must be redesigned
- Waterfall height, spectrum height, header/button dimensions all need recalculation
- Touch input zone mapping changes (GT911 provides direct screen coordinates)

### 2.3 SDR Layer (main/sdr.c, components/rtl_usb/)
- Handles RTL-SDR USB dongle communication via librtlsdr
- **RTL-SDR V3 only**: FT231XS chip (already supported by librtlsdr)
- No changes expected -- USB device enumeration is chip-independent

### 2.4 Audio Layer (main/audio.c, main/demod.c, main/rds.c)
- I2S interface to ES8311 codec
- AM/FM/WFM demodulation
- RDS decoding
- **Pin mapping unchanged**: GPIO7/8 (I2C), GPIO9-13 (I2S), GPIO53 (amp)
- **Note**: I2C bus shared with GT911 touch (different addresses, no conflict)

### 2.5 Wi-Fi/BLE Scanner (main/amap.c)
- Uses esp_hosted (ESP32-C6 co-processor over SDIO)
- **No changes** -- pin mapping identical between boards

### 2.6 ADS-B (main/adsb.c, main/adsb_view.c)
- 1090MHz squawk decoder via SDR
- **No changes** -- purely signal processing + UI

### 2.7 SD Card (main/sdcard.c, main/sdlog.c)
- SDIO 4-bit interface
- **No changes** -- pin mapping identical

## 3. Data Flow

    RTL-SDR USB --> IQ stream --> FFT --> dB values --> spectrum buffer
                                         |
                                         v
                                  rpi_canvas (off-screen)
                                         |
                                         v
                                  esp_lcd_panel_draw_bitmap
                                         |
                                         v
                                  ST7703 / MIPI DSI --> Display

    Touch:
    GT911 (I2C) --> x,y coordinates --> ui.c event handler --> action

    Audio:
    I2S In --> demod.c --> I2S Out --> ES8311 --> Speaker

## 4. Migration Changes Matrix

    | Module             | Current Board          | Target Board              | Change Level |
    |--------------------|------------------------|---------------------------|--------------|
    | Display driver     | ILI9486, 16-bit SPI    | ST7703, MIPI DSI 2-lane   | MAJOR        |
    | Touch driver       | XPT2046, SPI (raw ADC) | GT911, I2C (pixel coords) | MAJOR        |
    | Touch cal          | 5-point affine         | Not needed (capacitive)   | REMOVE       |
    | Canvas/drawing     | 480x320                | 720x720                   | ADAPT        |
    | UI layout          | Portrait 480x320       | Square 720x720            | REDESIGN     |
    | Audio (I2S/I2C)    | GPIO7/8/9-13/53        | GPIO7/8/9-13/53           | None         |
    | SD Card (SDIO)     | GPIO39-45              | GPIO39-45                 | None         |
    | USB OTG            | Via CH334F hub         | Direct to P4              | Minor (no hub)|
    | RTL-SDR V3         | FT231XS                | FT231XS                   | None         |
    | Wi-Fi/BLE          | ESP32-C6 hosted        | ESP32-C6 hosted           | None         |
    | Flash memory       | 16 MB                  | 32 MB                     | Improvement  |

## 5. Memory Budget (Estimated)

    | Resource      | Current            | Target             | Notes                    |
    |---------------|--------------------|--------------------|--------------------------|
    | PSRAM (32 MB) | 480x320x2B = 307KB | 720x720x2B = 1MB   | Framebuffer larger       |
    | IQ buffer     | 24K x 10 frames    | Same               | Unchanged                |
    | Spectrum bufs | ~200KB             | ~200KB             | Unchanged                |
    | Stack/heap    | Adequate           | Adequate           | 32MB PSRAM sufficient    |
    | Flash         | 16 MB              | 32 MB              | Headroom for growth      |

## 6. Key Risks

    1. MIPI DSI initialization: ST7703 requires precise reset sequence and
       DSI PHY configuration (LDO channel 3, 2.5V). Must follow Waveshare BSP.

    2. I2C bus contention: GT911 (0x5D) and ES8311 (0x18) share I2C.
       Addresses don't conflict, but timing must be verified.

    3. Layout redesign: 720x720 square vs 480x320 portrait requires
       recalculating all coordinate math in view.c and panel.c.

    4. Touch behavior change: Capacitive (GT911) vs resistive (XPT2046):
       - No press-and-hold-to-repeat -- need gesture-based alternative
       - Tap detection is different (interrupt-driven vs polling)

    5. USB OTG without hub: RTL-SDR connects directly to P4 USB.
       Should work, but physical connector may differ (USB-C vs micro-USB).
