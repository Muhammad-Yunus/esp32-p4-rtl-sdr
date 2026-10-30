# Conventions

## Code Style

    ### C Code
    - Indentation: 4 spaces (no tabs)
    - Line length: Maximum 100 characters
    - Braces: K&R style (opening brace on same line)
    - Naming:
      - Functions: snake_case (e.g., rpi_lcd_init, gt911_read_touch)
      - Variables: snake_case with s_ prefix for static/module-level
      - Constants/macros: UPPER_SNAKE_CASE (e.g., LCD_W, LCD_H)
      - Enums: UPPER_SNAKE_CASE (e.g., TOUCH_EVENT_TAP)
    - Includes: System includes first, then project includes, sorted alphabetically
    - Error handling: Use ESP_RETURN_ON_ERROR macro from esp_check.h

    ### Example
      static esp_err_t gt911_init(void)
      {
          esp_lcd_panel_io_handle_t io;
          esp_lcd_panel_io_i2c_config_t cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
          cfg.scl_speed_hz = WS_TOUCH_I2C_FREQ;
          esp_err_t ret = esp_lcd_new_panel_io_i2c(ws_i2c_bus(), &cfg, &io);
          if (ret != ESP_OK) return ret;
          return ESP_OK;
      }

## File Organization

    ### Component Structure
      components/waveshare_p4_lcd/
      +-- CMakeLists.txt
      +-- Kconfig
      +-- idf_component.yml     # st7703 + esp_lcd_touch_gt911 deps
      +-- include/
      |   +-- ws_lcd.h
      |   +-- ws_i2c.h
      |   +-- ws_panel_config.h
      |   +-- gt911_touch.h
      |   +-- xpt2046.h
      |   +-- touch_cal.h
      |   +-- rpi_canvas.h
      |   +-- rpi_text.h
      |   +-- font8x16.h
      +-- ws_lcd_dsi.c          # ST7703 MIPI DSI DISPLAY DRIVER
      +-- ws_i2c.c              # SHARED i2c_master BUS
      +-- gt911_touch.c         # OFFICIAL GT911 COMPONENT WRAPPER
      +-- xpt2046.c             # LEGACY API SHIM -> GT911
      +-- touch_cal.c           # OPTIONAL 5-POINT AFFINE CALIBRATION
      +-- rpi_canvas.c          # DRAWING PRIMITIVES
      +-- rpi_text.c            # TEXT RENDERING

      components/rpi_display/
      +-- CMakeLists.txt
      +-- include/
      |   +-- rpi_lcd.h
      |   +-- pins.h
      +-- rpi_lcd.c             # THIN COMPATIBILITY WRAPPER

    ### Main Directory
      main/
      +-- CMakeLists.txt
      +-- idf_component.yml
      +-- main.c          # APP ENTRY POINT
      +-- ui.c            # MAIN EVENT LOOP
      +-- view.c          # DISPLAY DRAWING
      +-- panel.c         # CONTROLS PANEL
      +-- sdr.c           # SDR CONTROLLER
      +-- audio.c         # AUDIO CODEC
      +-- [other .c/.h]   # (no change)

## Display Abstraction API

    The rpi_lcd module must maintain these function signatures for compatibility
    with the UI layer (view.c, ui.c):

      // Initialize display (called once at startup)
      esp_err_t rpi_lcd_init(void);

      // Fill a rectangle with a color (RGB565)
      esp_err_t rpi_lcd_fill_rect(int x, int y, int w, int h, uint16_t col);

      // Blit pixel data to screen (RGB565, little-endian)
      esp_err_t rpi_lcd_blit(int x, int y, int w, int h, const uint16_t *pix);

      // Set window and push big-endian pixel bytes (for DMA efficiency)
      esp_err_t rpi_lcd_set_window(int x0, int y0, int x1, int y1);
      esp_err_t rpi_lcd_push_pixels_be(const uint8_t *be_bytes, size_t nbytes);

      // Toggle portrait mode (swap width/height for touch calibration)
      esp_err_t rpi_lcd_set_portrait(bool on);

      // Get display dimensions
      int  rpi_lcd_width(void);
      int  rpi_lcd_height(void);

## Touch Abstraction API

    The UI keeps using the legacy shim; the real API is GT911-specific.
    Always acquire the bus through ws_i2c_bus(); never create a second
    legacy i2c bus on the same port.

      // Shared bus (GT911 + ES8311)
      esp_err_t ws_i2c_init(void);
      i2c_master_bus_handle_t ws_i2c_bus(void);

      // Initialize GT911 via esp_lcd_touch_gt911
      esp_err_t ws_touch_init_i2c(void);

      // Read active points in panel pixels
      // Returns: number of touched points (0-5)
      int ws_touch_read(ws_touch_t out[], int max_points);

      // Legacy shim used by ui.c (identity calibration by default)
      esp_err_t xpt2046_init(void);
      bool xpt2046_pressed(void);
      bool xpt2046_read_raw(uint16_t *x, uint16_t *y, uint16_t *z);

      // Optional calibration
      void touch_cal_default(touch_cal_t *cal);
      esp_err_t touch_cal_load(touch_cal_t *cal, float *max_err_px);
      esp_err_t touch_cal_save(const touch_cal_t *cal, float max_err_px);
      esp_err_t touch_cal_run(touch_cal_t *cal, float *max_err_px);
      void touch_cal_map(const touch_cal_t *cal, uint16_t rx, uint16_t ry,
                         int *px, int *py);

## Screen Coordinate System

    - Origin: Top-left corner (0, 0)
    - X axis: Increases to the right
    - Y axis: Increases downward
    - Resolution: 720 x 720 (square)
    - Portrait mode: 720 wide x 720 tall (same as square)
    - Landscape mode: 720 wide x 720 tall (same -- screen is square!)

    NOTE: Since the screen is square, portrait/landscape rotation has no
    visual effect on content dimensions. The rpi_lcd_set_portrait() function
    still exists for backward compatibility but may be a no-op.

## Color Format

    All colors are RGB565 (16-bit):
    - Bits 15-11: Red (5 bits)
    - Bits 10-5:  Green (6 bits)
    - Bits 4-0:  Blue (5 bits)

    Helper macro (existing):
    #define RGB565(r, g, b) (((r) & 0xF8) << 8 | ((g) & 0xFC) << 3 | ((b) & 0xF8) >> 3)

## Git Conventions

    ### Branch Naming
      feature/esp32-p4-wifi6-lcd4b    # Migration feature branch
      fix/display-init                # Bug fixes

    ### Commit Message Format
      <type>(<scope>): <subject>

      <body> (optional)

      <footer> (optional)

    Types: feat, fix, docs, refactor, test, chore

    Examples:
      feat(display): add ST7703 MIPI DSI driver

      - Initialize MIPI DSI 2-lane bus
      - Add ST7703 reset and init sequence
      - Implement blit and fill operations

      Refs: ESP-IDF esp_lcd component

## Documentation Conventions

    - All new code must have doxygen-style comments for public functions
    - Markdown files in docs/ use ATX headers (#, ##, ###)
    - Tables use pipe syntax with alignment
    - File paths use forward slashes (components/rpi_display/rpi_lcd.c)
