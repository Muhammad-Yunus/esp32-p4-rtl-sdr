# Third-party material

- `components/rtlsdr/blog/`: librtlsdr from the RTL-SDR Blog fork (rtlsdrblog/rtl-sdr-blog,
  commit aed0ea1), copied unmodified. GPL-2.0-or-later; see `blog/COPYING` and the file headers.
  The libusb shim in `components/rtlsdr/shim/` was written for this project.
- `components/waveshare_p4_lcd/`: display driver for ST7703 MIPI DSI + GT911 capacitive touch
  (MIT license), adapted from [waveshare/esp32-p4-4b-lcd](https://github.com/waveshare/esp32-p4-4b-lcd).
- `main/adsb.c`, position checks: local (single-frame) CPR decoding, the position jump check and
  suspect / consensus recovery are ported from ADS-B Scope by John Stockdale
  ([jstockdale/T-Display-P4](https://github.com/jstockdale/T-Display-P4), `adsb` branch,
  `main/examples/adsb_receiver/class_driver.c`), GPL-3.0, with his thresholds, and adapted to
  this decoder.
- `esp_hosted` stub and `esp_codec_dev` (Espressif, Apache-2.0) are fetched via component
  manager at build time.
- ESP-IDF (Apache-2.0) is not included. It is needed to build, and the prebuilt images link it.
- `tools/mapgen/make_map.py` downloads its data at run time; none is included here.
  Natural Earth is public domain. OpenStreetMap data is © OpenStreetMap contributors and
  available under the ODbL, so a `MAP.BIN` with OSM layers carries that attribution.
- `docs/images/ESP32-P4-Smart-86-Box.jpg`: product photo from Waveshare's ESP32-P4-WIFI6-Touch-LCD-4B,
  used only to identify the hardware. It is not covered by this repository's license.
  Other images in `docs/images` are renders from `tools/preview`.
