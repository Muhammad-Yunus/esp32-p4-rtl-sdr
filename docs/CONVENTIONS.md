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
          i2c_cmd_handle_t cmd = i2c_cmd_link_create();
          i2c_master_write_byte(cmd, GT911_ADDR << 1, I2C_MASTER_ACK_CHECK_EN);
          i2c_master_cmd_begin(I2C_NUM_1, cmd, pdMS_TO_TICKS(1000));
          i2c_cmd_link_delete(cmd);
          return ESP_OK;
      }

## File Organization

    ### Component Structure
      components/rpi_display/
      +-- CMakeLists.txt
      +-- include/
      |   +-- rpi_lcd.h
      |   +-- rpi_canvas.h
      |   +-- rpi_text.h
      |   +-- pins.h
      |   +-- gt911_touch.h    # NEW
      +-- rpi_lcd.c             # DISPLAY ABSTRACTION (rewrite for MIPI DSI)
      +-- rpi_canvas.c          # DRAWING PRIMITIVES (no change)
      +-- rpi_text.c            # TEXT RENDERING (no change)
      +-- gt911_touch.c         # TOUCH DRIVER (NEW)
      +-- [deleted]
          +-- xpt2046.c         # REMOVE
          +-- xpt2046.h         # REMOVE
          +-- touch_cal.c       # REMOVE
          +-- touch_cal.h       # REMOVE

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

    The touch driver must provide this interface (replacing xpt2046):

      // Initialize touch controller
      esp_err_t gt911_touch_init(void);

      // Read current touch state (non-blocking)
      // Returns: number of touched points (0-5)
      int gt911_touch_read(int *x, int *y, int *count);

      // Check if a tap event just occurred
      bool gt911_touch_was_tap(void);

      // Check if a hold event is active
      bool gt911_touch_is_holding(void);

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
