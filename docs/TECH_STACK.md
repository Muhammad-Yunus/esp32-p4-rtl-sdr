# Tech Stack

## Core Framework

    | Item        | Version / Path                                     |
    |-------------|----------------------------------------------------|
    | ESP-IDF     | C:\esp\v6.1\esp-idf (v6.1)                         |
    | Toolchain   | ESP-IDF bundled (xtensa-esp32s4-elf-gcc)           |
    | Build Sys   | CMake (idf.py)                                     |
    | Flash tool  | esptool.py (via IDF)                               |

## Target Hardware: ESP32-P4-WIFI6-Touch-LCD-4B

    | Component     | Specification                                        |
    |---------------|------------------------------------------------------|
    | MCU           | ESP32-P4 (dual-core Xtensa LX7, 480 MHz)             |
    | Display       | ST7703 driver IC, 720x720 pixels, MIPI DSI 2-lane   |
    | Touch         | Goodix GT911, I2C address 0x5D                       |
    | Audio Codec   | ES8311 (I2C + I2S)                                   |
    | Audio Amp     | NS4150B (GPIO53)                                     |
    | Wi-Fi/BLE     | ESP32-C6-MINI-1U-H8 (hosted via SDIO)                |
    | Flash         | 32 MB (SPI Nor)                                      |
    | PSRAM         | 32 MB                                                |
    | USB           | USB 2.0 High-Speed OTG (native P4, no hub)           |
    | SD Card       | SDIO 3.0 TF slot                                     |
    | Backlight PWM | GPIO26                                               |
    | Backlight EN  | GPIO33                                               |
    | LCD Reset     | GPIO27                                               |
    | Touch Reset   | GPIO23                                               |
    | Touch INT     | Not connected (GPIO_NC)                              |

## Pin Mapping (Target Board)

    Display (MIPI DSI):
      DSI lanes:       dedicated MIPI DSI pins (no GPIO configuration)
      LCD_RST:         GPIO27
      BL_EN:           GPIO33 (active HIGH)
      BL_PWM:          GPIO26

    Shared I2C bus (board net labeled I2C1, firmware I2C port 0):
      SDA:             GPIO7  <-- GT911 touch + ES8311 codec
      SCL:             GPIO8  <-- GT911 touch + ES8311 codec
      TOUCH_RST:       GPIO23
      TOUCH_INT:       not connected

    Audio (I2S + shared I2C):
      I2S_DATA:        GPIO9
      I2S_BCLK:        GPIO10
      I2S_LRC:         GPIO11
      I2S_MCLK:        GPIO12
      I2S_SDIN:        GPIO13
      AMP_ENABLE:      GPIO53

    SD Card (SDIO):
      SD_CMD:          GPIO44
      SD_CLK:          GPIO43
      SD_DATA0-3:      GPIO39-42
      SD_PWR:          GPIO45

    USB OTG:
      USB_DM:          GPIO19
      USB_DP:          GPIO20

    Hosted Wi-Fi (ESP32-C6 SDIO):
      C6_SDIO_CLK:     GPIO14
      C6_SDIO_CMD:     GPIO16
      C6_SDIO_DATA0:   GPIO18
      C6_SDIO_DATA1:   GPIO22

## External Dependencies

    | Library                       | Version | Purpose                              |
    |-------------------------------|---------|--------------------------------------|
    | librtlsdr                     | in-tree | RTL-SDR USB driver (FT231XS for V3)  |
    | waveshare/esp_lcd_st7703      | ^2.0.0  | ST7703 MIPI DSI panel controller     |
    | espressif/esp_lcd_touch_gt911 | ^1.2.1  | GT911 capacitive touch driver        |
    | espressif/esp_lcd_touch       | 1.2.1   | Touch abstraction (transitive)       |
    | espressif/esp_codec_dev       | 1.6.*   | Audio codec abstraction (ES8311)     |
    | espressif/usb                 | *       | USB host stack for RTL-SDR           |
    | esp_hosted                    | TBD     | ESP32-C6 co-processor (not wired yet)|

## Build Commands

    # Set up ESP-IDF environment
    C:\esp\v6.1\esp-idf\export.bat

    # Build for target
    idf.py build

    # Flash (COM11)
    idf.py -p COM11 flash

    # Monitor
    idf.py -p COM11 monitor

    # Clean
    idf.py fullclean

## ESP-IDF Components Used

    esp-idf/components/
    +-- esp_lcd/          # MIPI DSI + ST7703 driver
    |   +-- dsi/          # esp_lcd_mipi_dsi.h
    |   +-- spi/          # esp_lcd_spi_panel.h
    |   +-- include/      # esp_lcd_panel_ops.h, esp_lcd_panel_io.h
    +-- driver/           # gpio, i2s, spi, ledc
    +-- esp_driver_i2c/   # i2c_master API (shared GT911 + ES8311 bus)
    +-- esp_hid/          # BLE HID (if used)
    +-- esp_wifi/         # Wi-Fi scanner
    +-- bt/               # BLE scanner
    +-- nvs_flash/        # Non-volatile storage
    +-- fatfs/            # SD card filesystem
