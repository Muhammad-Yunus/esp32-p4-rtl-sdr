# Product Requirements Document (PRD) -- Board Migration

## 1. Project Context

    Source Board: Waveshare ESP32-P4-Module-DEV-KIT
      Display: ILI9486, 480x320, 16-bit SPI
      Touch: XPT2046 (resistive), SPI
      USB: ESP32-P4 -> CH334F hub -> USB OTG

    Target Board: Waveshare ESP32-P4-WIFI6-Touch-LCD-4B
      Display: ST7703, 720x720, MIPI DSI 2-lane
      Touch: GT911 (capacitive), I2C
      USB: ESP32-P4 native USB OTG (no hub)
      Flash: 32 MB (upgrade from 16 MB)

    Goal: Port the existing RTL-SDR application from the source board to the target
    board while preserving all existing functionality (spectrum display, waterfall,
    controls, FM/AM/WFM, RDS, Wi-Fi scanner, BLE scanner, ADS-B, SD logging).

## 2. Constraints

    | Constraint    | Detail                                           |
    |---------------|--------------------------------------------------|
    | Screen shape  | 720x720 square (vs 480x320 portrait/landscape)  |
    | Drawing       | Custom software rendering (NO LVGL) -- per user decision |
    | RTL-SDR ver   | V3 only (FT231XS chip)                           |
    | Framework     | ESP-IDF v6.1 (C:\esp\v6.1\esp-idf)              |
    | Toolchain     | ESP-IDF build system (CMake + idf.py)            |

## 3. Functional Requirements

    ### 3.1 Display (NEW -- replaces rpi_lcd.c)
      [ ] FR-DISPLAY-01: Initialize ST7703 panel via MIPI DSI 2-lane
      [ ] FR-DISPLAY-02: Support RGB565 pixel format (16-bit per pixel)
      [ ] FR-DISPLAY-03: Blit rectangular regions to screen (full or partial)
      [ ] FR-DISPLAY-04: Fill rectangular regions with solid color
      [ ] FR-DISPLAY-05: Backlight control via GPIO33 (enable) + GPIO26 (PWM dim)
      [ ] FR-DISPLAY-06: Display reset via GPIO27
      [ ] FR-DISPLAY-07: Handle screen rotation (portrait/landscape modes)

    ### 3.2 Touch (NEW -- replaces xpt2046.c + touch_cal.c)
      [ ] FR-TOUCH-01: Initialize GT911 via I2C (address 0x5D, GPIO7/8)
      [ ] FR-TOUCH-02: Read touch coordinates (x, y, touch count)
      [ ] FR-TOUCH-03: Provide tap, hold, and swipe gesture detection
      [ ] FR-TOUCH-04: No calibration required (capacitive = pixel-accurate)
      [ ] FR-TOUCH-05: Report touch events at >=30 Hz polling rate

    ### 3.3 UI Layout (ADAPT -- view.c, panel.c)
      [ ] FR-UI-01: Redesign layout for 720x720 square screen
      [ ] FR-UI-02: Maintain all existing UI elements (spectrum, waterfall,
                    frequency readout, buttons, controls panel)
      [ ] FR-UI-03: Support portrait and landscape orientations
      [ ] FR-UI-04: Maintain responsive touch targets (min 40x40 px for buttons)
      [ ] FR-UI-05: Preserve all theme/color definitions from existing code

    ### 3.4 SDR (NO CHANGE -- sdr.c, rtl_usb/)
      [ ] FR-SDR-01: RTL-SDR V3 (FT231XS) device enumeration and control
      [ ] FR-SDR-02: Frequency tuning (50 kHz -- 1.75 GHz)
      [ ] FR-SDR-03: Sample rate configuration
      [ ] FR-SDR-04: Gain control (manual and AGC)

    ### 3.5 Audio (NO CHANGE -- audio.c, demod.c, rds.c)
      [ ] FR-AUDIO-01: I2S audio output to ES8311 (GPIO9-13)
      [ ] FR-AUDIO-02: Volume control via ES8311
      [ ] FR-AUDIO-03: Speaker amp enable via GPIO53
      [ ] FR-AUDIO-04: FM/AM/WFM demodulation
      [ ] FR-AUDIO-05: RDS decoding and display

    ### 3.6 Wi-Fi/BLE Scanner (NO CHANGE -- amap.c)
      [ ] FR-NET-01: Wi-Fi network scanning via ESP32-C6 hosted
      [ ] FR-NET-02: BLE device scanning via ESP32-C6 hosted
      [ ] FR-NET-03: Display scan results on screen

    ### 3.7 ADS-B (NO CHANGE -- adsb.c, adsb_view.c)
      [ ] FR-ADSB-01: Decode 1090MHz ADS-B messages
      [ ] FR-ADSB-02: Display aircraft information

    ### 3.8 SD Card (NO CHANGE -- sdcard.c, sdlog.c)
      [ ] FR-SD-01: SD card mount and filesystem access
      [ ] FR-SD-02: Log IQ data to SD card

## 4. Non-Functional Requirements

    | ID    | Requirement                                          |
    |-------|------------------------------------------------------|
    | NFR-01| Display refresh rate: >=30 FPS at 720x720           |
    | NFR-02| Touch latency: <=50 ms from physical touch to screen|
    | NFR-03| Boot time: <5 seconds from power-on to functional UI|
    | NFR-04| Memory usage: <25 MB of PSRAM (32 MB available)     |
    | NFR-05| All existing features must work identically after migration |

## 5. Implementation Phases

    ### Phase 1: Display Driver Foundation
      Goal: Get a blank screen with backlight working.
      1. Create rpi_lcd.c using ESP-IDF esp_lcd + MIPI DSI APIs
      2. Follow pattern from 07_color_panel example in Waveshare ESP-IDF
      3. Initialize ST7703 with reset sequence
      4. Enable backlight (GPIO33 HIGH, GPIO26 PWM)
      5. Test: solid color fill across entire 720x720 screen

    ### Phase 2: Touch Driver
      Goal: Read touch coordinates from GT911.
      1. Create gt911_touch.c using I2C driver
      2. Implement coordinate reading and event detection
      3. Replace xpt2046.c + touch_cal.c with GT911 driver
      4. Test: tap on screen reports correct coordinates

    ### Phase 3: Canvas and Drawing
      Goal: Render basic shapes on screen.
      1. Verify rpi_canvas.c works with new resolution (no changes needed)
      2. Verify rpi_text.c works with new resolution (no changes needed)
      3. Test: draw colored rectangles, circles, text at 720x720

    ### Phase 4: UI Layout Adaptation
      Goal: All UI elements visible and functional at 720x720.
      1. Recalculate all coordinates in view.c for 720x720
      2. Recalculate layout in panel.c for 720x720
      3. Adjust touch event handling in ui.c for capacitive touch
      4. Test: all screens (spectrum, waterfall, controls, scanner, ADS-B)

    ### Phase 5: Integration and Testing
      Goal: Full system functional.
      1. Connect RTL-SDR V3, verify frequency tuning
      2. Test audio output (FM radio, amplitude detection)
      3. Test Wi-Fi and BLE scanning
      4. Test SD card logging
      5. Test ADS-B reception (if antenna available)
      6. Long-duration stability test (>=1 hour)

## 6. File Changes Summary

    ### New Files
      components/rpi_display/
        gt911_touch.c       # GT911 I2C touch driver
        gt911_touch.h       # GT911 driver header
      docs/
        ARCHITECTURE.md     # This migration architecture document
        TECH_STACK.md       # Hardware and software stack reference
        PRD.md              # This requirements document
        CONVENTIONS.md      # Code and project conventions

    ### Modified Files
      components/rpi_display/
        rpi_lcd.c           # Complete rewrite: MIPI DSI instead of SPI
        rpi_lcd.h           # Update function signatures if needed
        include/pins.h      # Update pin definitions for target board
      main/
        main.c              # Update display init sequence
        ui.c                # Update touch handling (remove calibration)
        view.c              # Redesign layout for 720x720
        panel.c             # Redesign layout for 720x720
        Kconfig             # Update display config options
      sdkconfig.defaults    # Update for new board (flash, PSRAM, MIPI DSI)

    ### Deleted Files
      components/rpi_display/
        xpt2046.c           # Resistive touch driver -- no longer needed
        xpt2046.h
        touch_cal.c         # 5-point calibration -- not needed for capacitive
        touch_cal.h

## 7. Success Criteria

  - [ ] Board boots and displays RTL-SDR UI within 5 seconds
  - [ ] Spectrum display updates in real-time at >=30 FPS
  - [ ] Waterfall renders smoothly
  - [ ] All touch interactions work (tap, hold, swipe)
  - [ ] Frequency tuning responds to touch
  - [ ] Controls panel opens/closes correctly
  - [ ] Audio plays through speaker
  - [ ] Wi-Fi scanner detects networks
  - [ ] BLE scanner detects devices
  - [ ] SD card can log and playback IQ data
  - [ ] No memory leaks after 1 hour of operation
  - [ ] All themes render correctly at new resolution
