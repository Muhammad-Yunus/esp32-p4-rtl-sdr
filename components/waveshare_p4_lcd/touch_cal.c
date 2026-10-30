/*
 * Touch calibration helper for GT911 capacitive touch
 * Saves/restores an affine transform mapping XPT2046 raw coords (0..4095)
 * to panel pixel coordinates. The original XPT2046 driver produced 12-bit
 * values; the GT911 is scaled to the same range for compatibility.
 */

#include "touch_cal.h"
#include "gt911_touch.h"
#include "ws_lcd.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "touch_cal";
#define CAL_NVS_NAMESPACE  "touch"
#define CAL_NVS_KEY        "cal"
#define CAL_NVS_VERSION    1

/* Solve 2x3 affine system from N calibration pairs.
 *   px = a*rx + b*ry + c
 *   py = d*rx + e*ry + f
 * Uses least-squares normal equations (N >= 3). */
static void solve_affine(const uint16_t rx[], const uint16_t ry[],
                         const int px[], const int py[], int N,
                         touch_cal_t *cal)
{
    float Srx = 0, Sry = 0, Srx2 = 0, Sry2 = 0, Srxry = 0;
    float Spx = 0, Spy = 0, Srxp = 0, Sryp = 0, Srxpy = 0, Srypy = 0;
    for (int i = 0; i < N; i++) {
        float rxf = rx[i], ryf = ry[i];
        Srx  += rxf;  Sry  += ryf;
        Srx2 += rxf * rxf;  Sry2 += ryf * ryf;  Srxry += rxf * ryf;
        Spx  += px[i];  Spy  += py[i];
        Srxp += rxf * px[i];  Sryp += ryf * px[i];
        Srxpy += rxf * py[i];  Srypy += ryf * py[i];
    }
    float det = Srx2 * Sry2 - Srxry * Srxry;
    if (det == 0) { cal->a = 1; cal->b = 0; cal->c = 0; cal->d = 0; cal->e = 1; cal->f = 0; return; }
    cal->a = (Spx * Sry2 - Spy * Srxry) / (N * det) * N;  /* simplify: remove N from denom */
    cal->b = (Spy * Srx2 - Spx * Srxry) / det;
    cal->c = (Spx + cal->a * Srx + cal->b * Sry) / N;
    cal->d = (Spy * Srx2 - Srxp * Srxry) / det;
    cal->e = (Spx * Sry2 - Srxp * Srxry) / det;
    cal->f = (Spy + cal->d * Srx + cal->e * Sry) / N;
}

static void invert_affine(const touch_cal_t *c, float *a_out, float *b_out, float *c_out,
                           float *d_out, float *e_out, float *f_out)
{
    float det = c->a * c->e - c->b * c->d;
    *a_out = c->e / det;
    *b_out = -c->b / det;
    *c_out = (c->b * c->f - c->c * c->e) / det;
    *d_out = -c->d / det;
    *e_out = c->a / det;
    *f_out = (c->c * c->d - c->a * c->f) / det;
}

esp_err_t touch_cal_load(touch_cal_t *cal, float *max_err_px)
{
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open(CAL_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (ret != ESP_OK) {
        if (max_err_px) *max_err_px = -1;
        return ret;
    }
    uint8_t ver = 0;
    ret = nvs_get_u8(nvs, "ver", &ver);
    if (ret != ESP_OK || ver != CAL_NVS_VERSION) {
        nvs_close(nvs);
        if (max_err_px) *max_err_px = -1;
        return ESP_ERR_NOT_FOUND;
    }
    size_t len = sizeof(touch_cal_t);
    ret = nvs_get_blob(nvs, CAL_NVS_KEY, cal, &len);
    nvs_close(nvs);
    if (ret != ESP_OK) {
        if (max_err_px) *max_err_px = -1;
        return ret;
    }
    /* Compute max error on calibration points */
    if (max_err_px) {
        uint16_t rx[] = { 400, 3695, 3695, 400, 2048 };
        uint16_t ry[] = { 400, 400,  3695, 3695, 2048 };
        int      px[] = {  50,   670,   670,    50,  360 };
        int      py[] = {  50,    50,   670,   670,  360 };
        float err_max = 0;
        for (int i = 0; i < 5; i++) {
            int cx, cy;
            touch_cal_map(cal, rx[i], ry[i], &cx, &cy);
            float dx = cx - px[i], dy = cy - py[i];
            float e = sqrtf(dx * dx + dy * dy);
            if (e > err_max) err_max = e;
        }
        *max_err_px = err_max;
    }
    return ESP_OK;
}

esp_err_t touch_cal_save(const touch_cal_t *cal, float max_err_px)
{
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open(CAL_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (ret != ESP_OK) return ret;
    nvs_set_u8(nvs, "ver", CAL_NVS_VERSION);
    ret = nvs_set_blob(nvs, CAL_NVS_KEY, cal, sizeof(*cal));
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs);
    }
    nvs_close(nvs);
    return ret;
}

esp_err_t touch_cal_run(touch_cal_t *cal, float *max_err_px)
{
    /* 5-point calibration: ask user to tap centre of each cross.
     * Screen coords in pixels; raw XPT2046 values are stored and matched. */
    uint16_t raw_rx[5], raw_ry[5];
    int scr_px[5] = {  50,  670,  670,   50,  360 };
    int scr_py[5] = {  50,   50,  670,  670,  360 };

    for (int i = 0; i < 5; i++) {
        /* Show cross at (scr_px[i], scr_py[i]), wait for touch */
        for (int attempt = 0; attempt < 60; attempt++) {
            ws_touch_t pts[5];
            int n = ws_touch_read(pts, 5);
            if (n > 0) {
                int dx = pts[0].x - scr_px[i];
                int dy = pts[0].y - scr_py[i];
                if (dx * dx + dy * dy < 900) {  /* within 30px */
                    /* Convert pixel to raw XPT2046 (inverse scale) */
                    raw_rx[i] = (uint16_t)((int64_t)pts[0].x * 4095 / LCD_W);
                    raw_ry[i] = (uint16_t)((int64_t)pts[0].y * 4095 / LCD_H);
                    break;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    solve_affine(raw_rx, raw_ry, scr_px, scr_py, 5, cal);
    touch_cal_save(cal, 0);
    return ESP_OK;
}

void touch_cal_map(const touch_cal_t *cal, uint16_t rx, uint16_t ry, int *px, int *py)
{
    int cx = (int)(cal->a * rx + cal->b * ry + cal->c);
    int cy = (int)(cal->d * rx + cal->e * ry + cal->f);
    if (cx < 0) {
        cx = 0;
    } else if (cx >= LCD_W) {
        cx = LCD_W - 1;
    }
    if (cy < 0) {
        cy = 0;
    } else if (cy >= LCD_H) {
        cy = LCD_H - 1;
    }
    if (px) *px = cx;
    if (py) *py = cy;
}
