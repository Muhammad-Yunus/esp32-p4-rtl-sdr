/*
 * Touch calibration helper for GT911 capacitive touch.
 * Saves/restores an affine transform mapping the touch controller's
 * reported coordinates to panel pixel coordinates.
 */

#include "touch_cal.h"
#include "gt911_touch.h"
#include "ws_lcd.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "touch_cal";
#define CAL_NVS_NAMESPACE  "touch"
#define CAL_NVS_KEY        "cal"
#define CAL_NVS_VERSION    2

void touch_cal_default(touch_cal_t *cal)
{
    cal->a = 1;
    cal->b = 0;
    cal->c = 0;
    cal->d = 0;
    cal->e = 1;
    cal->f = 0;
}

/* Solve 2x3 affine system from N calibration pairs.
 *   px = a*rx + b*ry + c
 *   py = d*rx + e*ry + f
 * Uses least-squares normal equations (N >= 3). */
static void solve_affine(const uint16_t rx[], const uint16_t ry[],
                         const int px[], const int py[], int N,
                         touch_cal_t *cal)
{
    float mx = 0, my = 0, mp = 0, mq = 0;
    for (int i = 0; i < N; i++) {
        mx += rx[i];
        my += ry[i];
        mp += px[i];
        mq += py[i];
    }
    mx /= N; my /= N; mp /= N; mq /= N;

    float Sxx = 0, Sxy = 0, Syy = 0, Sxp = 0, Syp = 0, Sxq = 0, Syq = 0;
    for (int i = 0; i < N; i++) {
        float dx = rx[i] - mx, dy = ry[i] - my;
        float dp = px[i] - mp, dq = py[i] - mq;
        Sxx += dx * dx;  Sxy += dx * dy;  Syy += dy * dy;
        Sxp += dx * dp;  Syp += dy * dp;
        Sxq += dx * dq;  Syq += dy * dq;
    }

    float det = Sxx * Syy - Sxy * Sxy;
    if (fabsf(det) < 1e-6f) {
        touch_cal_default(cal);
        return;
    }
    cal->a = (Sxp * Syy - Syp * Sxy) / det;
    cal->b = (Syp * Sxx - Sxp * Sxy) / det;
    cal->c = mp - cal->a * mx - cal->b * my;
    cal->d = (Sxq * Syy - Syq * Sxy) / det;
    cal->e = (Syq * Sxx - Sxq * Sxy) / det;
    cal->f = mq - cal->d * mx - cal->e * my;
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
        uint16_t rx[] = {  50,  670,  670,   50,  360 };
        uint16_t ry[] = {  50,   50,  670,  670,  360 };
        int      px[] = {  50,  670,  670,   50,  360 };
        int      py[] = {  50,   50,  670,  670,  360 };
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

static void draw_target(int x, int y)
{
    ws_lcd_fill_rect(0, 0, LCD_W, LCD_H, 0x0000);
    ws_lcd_fill_rect(x - 20, y - 1, 40, 3, 0xFFFF);
    ws_lcd_fill_rect(x - 1, y - 20, 3, 40, 0xFFFF);
}

esp_err_t touch_cal_run(touch_cal_t *cal, float *max_err_px)
{
    uint16_t raw_rx[5], raw_ry[5];
    int m = LCD_W / 12;
    int scr_px[5] = { m, LCD_W - m, LCD_W - m, m, LCD_W / 2 };
    int scr_py[5] = { m, m, LCD_H - m, LCD_H - m, LCD_H / 2 };

    /* The screen is typically held to enter calibration; wait for release first. */
    int64_t start = esp_timer_get_time() + 3000000;
    while (esp_timer_get_time() < start) {
        ws_touch_t pts[5];
        if (ws_touch_read(pts, 5) <= 0) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    for (int i = 0; i < 5; i++) {
        draw_target(scr_px[i], scr_py[i]);
        raw_rx[i] = scr_px[i];
        raw_ry[i] = scr_py[i];
        bool got = false;
        int64_t deadline = esp_timer_get_time() + 15000000;
        while (!got && esp_timer_get_time() < deadline) {
            ws_touch_t pts[5];
            if (ws_touch_read(pts, 5) > 0) {
                raw_rx[i] = pts[0].x;
                raw_ry[i] = pts[0].y;
                got = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        if (!got) {
            ESP_LOGW(TAG, "calibration point %d not touched: using 1:1 mapping", i + 1);
            touch_cal_default(cal);
            if (max_err_px) *max_err_px = -1;
            return ESP_OK;
        }
        int64_t release = esp_timer_get_time() + 3000000;
        while (esp_timer_get_time() < release) {
            ws_touch_t pts[5];
            if (ws_touch_read(pts, 5) <= 0) break;
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    solve_affine(raw_rx, raw_ry, scr_px, scr_py, 5, cal);

    float err_max = 0;
    for (int i = 0; i < 5; i++) {
        int cx, cy;
        touch_cal_map(cal, raw_rx[i], raw_ry[i], &cx, &cy);
        float dx = cx - scr_px[i], dy = cy - scr_py[i];
        float e = sqrtf(dx * dx + dy * dy);
        if (e > err_max) err_max = e;
    }
    touch_cal_save(cal, err_max);
    if (max_err_px) *max_err_px = err_max;
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
