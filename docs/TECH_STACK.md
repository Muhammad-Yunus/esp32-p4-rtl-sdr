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

## Pin Mapping (Target Board)

    Display (MIPI DSI):
      DSI_DATA lanes:  GPIO0..GPIO7 (2-lane x 8 bits = 16-bit RGB565)
      DSI_CLK:         GPIO8
      LCD_RST:         GPIO27
      BL_EN:           GPIO33 (active HIGH)
      BL_PWM:          GPIO26

    Touch (I2C):
      I2C1_SDA:        GPIO7
      I2C1_SCL:        GPIO8
      TOUCH_RST:       GPIO23

    Audio (I2S + I2C):
      I2S_DATA:        GPIO9
      I2S_BCLK:        GPIO10
      I2S_LRC:         GPIO11
      I2S_MCLK:        GPIO12
      I2S_SDIN:        GPIO13
      I2C0_SDA:        GPIO7  <-- SHARED with GT911
      I2C0_SCL:        GPIO8  <-- SHARED with GT911
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

    | Library             | Purpose                                         |
    |---------------------|--------------------------------------------------|
    | librtlsdr           | RTL-SDR USB dongle driver (FT231XS for V3)      |
    | esp_lcd             | MIPI DSI and ST7703 panel drivers               |
    | esp_lcd_mipi_dsi    | MIPI DSI bus abstraction                          |
    | esp_lcd_st7703      | ST7703 panel controller                           |
    | esp_hosted          | ESP32-C6 co-processor for Wi-Fi/BLE             |
    | esp_codec_dev       | Audio codec device abstraction (ES8311)         |

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
    +-- driver/           # gpio, i2c, i2s, spi, ledc
    +-- esp_hid/          # BLE HID (if used)
    +-- esp_wifi/         # Wi-Fi scanner
    +-- bt/               # BLE scanner
    +-- nvs_flash/        # Non-volatile storage
    +-- fatfs/            # SD card filesystem
