/*
 * Controls, drawn over the spectrum and waterfall in the current look.
 * 720x720 square panel (ESP32-P4-WIFI6-Touch-LCD-4B).
 * Built for a capacitive panel: big keys that fill the screen, a key lights up
 * under the finger and fires when the finger lifts, and the -/+ steppers fire
 * on press and repeat while held.
 *
 *   menu:   GAIN  RANGE  ZOOM  AUDIO / BAND  SCAN  LOOK  DONE
 *   GAIN:   -  value  +,  AGC / 15 dB / 30 dB / MAX
 *   RANGE:  top -/+, span -/+, AUTO
 *   ZOOM: four choices;  AUDIO: four filters, volume and squelch -/+
 *   BAND: eight presets;  SCAN: WIFI, BLE, SAVE, MEMORY;  LOOK: themes, TURN
 *   MEMORY: eight channel slots;  SAVE: tap a slot to store what's tuned
 * No ESP-IDF calls (tools/preview builds it).
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define PANEL_ZOOMS  4
#define PANEL_BWS    4
#define PANEL_BANDS  8
#define PANEL_MEMS   8

typedef struct {
    int gain_idx, gain_n;
    const char *(*gain_label)(int i);
    int (*gain_tenths)(int i);
    int (*agc_tenths)(void);
    bool range_auto;
    int ref_db, range_db;
    int zoom, bw, band;
    int vol, sql;
    int look;
    uint32_t freq;
    uint32_t mem_hz[PANEL_MEMS];
    int8_t mem_bw[PANEL_MEMS];
    int mem;
} ctl_t;

enum { CTL_GAIN = 1, CTL_RANGE = 2, CTL_ZOOM = 4, CTL_BW = 8, CTL_BAND = 16, CTL_CLOSE = 32,
       CTL_WIFI = 64, CTL_BLE = 128,
       CTL_LOOK = 256,
       CTL_TURN = 512,
       CTL_AUDIO = 1024,
       CTL_MEM_SAVE = 2048,
       CTL_MEM_TUNE = 4096,
       CTL_MEM_SCAN = 8192 };
enum { PANEL_PRESS, PANEL_MOVE, PANEL_RELEASE };

void panel_init(void);
void panel_open(const ctl_t *c);
void panel_redraw(const ctl_t *c);
void panel_open_memory(const ctl_t *c, bool save);
int panel_touch(ctl_t *c, int ev, int x, int y, int64_t now_us);

uint32_t panel_zoom_hz(int i);
uint32_t panel_bw_hz(int i);
const char *panel_bw_mode(int i);
const char *panel_band(int i, uint32_t *freq, int *bw, int *zoom);
