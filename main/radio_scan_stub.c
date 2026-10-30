#include "radio_scan.h"

bool radio_scan_start(int kind, const char **why) { return false; }
void radio_scan_poll(void) {}
int radio_scan_get_ap(scan_ap_t *out, int max, int *sweeps) { return 0; }
int radio_scan_get_ble(scan_ble_t *out, int max) { return 0; }
void radio_scan_stop(void) {}
