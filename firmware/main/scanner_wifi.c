#include "scanner_wifi.h"
#include "scanner_table.h"
#include "scanner_classify.h"
#include "scanner_common.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <string.h>

// Max APs processed per scan (the table evicts on overflow anyway).
#define MAX_RESULTS_PER_SCAN 32

static bool scanning = false;
// Note: when Bluetooth is enabled, the C3 coexistence path rejects custom
// active scan times and warns; leaving scan_time at 0 uses the driver's
// own defaults, which is what ends up being applied anyway.
static wifi_scan_config_t scan_cfg = {
    .ssid = NULL,
    .bssid = NULL,
    .channel = 0,
    .show_hidden = false,
    .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    .scan_time = { .active = { .min = 0, .max = 0 }, .passive = 0 },
    .home_chan_dwell_time = 0,
    .channel_bitmap = { .ghz_2_channels = 0, .ghz_5_channels = 0 },
};

static auth_t map_auth(wifi_auth_mode_t m) {
    switch (m) {
        case WIFI_AUTH_OPEN:          return AUTH_OPEN;
        case WIFI_AUTH_WEP:           return AUTH_WEP;
        case WIFI_AUTH_WPA_PSK:       return AUTH_WPA;
        case WIFI_AUTH_WPA2_PSK:      return AUTH_WPA2;
        case WIFI_AUTH_WPA_WPA2_PSK:  return AUTH_WPA2_WPA3;
        case WIFI_AUTH_WPA3_PSK:      return AUTH_WPA3;
        case WIFI_AUTH_WPA2_WPA3_PSK: return AUTH_WPA2_WPA3;
        default:                      return AUTH_OPEN;
    }
}

// Runs in the default event loop task (small stack): fetch one record at a
// time (no big arrays on the stack) and always drain/free the rest of the
// list, even if we stop early at the cap.
static void scan_cb(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg; (void)base; (void)id;
    wifi_event_sta_scan_done_t *res = (wifi_event_sta_scan_done_t *)data;
    scanning = false;  // the scan finished (successfully or not)
    if (res->status != 0) return;  // failed/aborted scan: no usable records

    wifi_ap_record_t ap;
    uint32_t now = scanner_now_ms();
    int taken = 0;
    bool drained = false;
    while (taken < MAX_RESULTS_PER_SCAN) {
        if (esp_wifi_scan_get_ap_record(&ap) != ESP_OK) { drained = true; break; }
        if (ap.rssi < -85) continue;  // filter weak APs
        const char *ssid = (const char *)ap.ssid;
        const char *vendor = NULL;
        cat_t cat = CAT_UNKNOWN;
        scanner_classify(ap.bssid, ssid, NULL, 0, &vendor, &cat);
        scanner_table_add(ap.bssid, SRC_WIFI, ssid, vendor, cat,
                          (int8_t)ap.rssi, ap.primary, map_auth(ap.authmode), now);
        taken++;
    }
    if (!drained) esp_wifi_clear_ap_list();  // free the records we didn't fetch
}

esp_err_t scanner_wifi_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) return err;

    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, scan_cb, NULL);
    if (err != ESP_OK) return err;

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) return err;

    return esp_wifi_start();
}

esp_err_t scanner_wifi_scan_start(void) {
    if (scanning) return ESP_ERR_INVALID_STATE;
    // Async scan: a blocking call would stall the main loop for seconds.
    esp_err_t err = esp_wifi_scan_start(&scan_cfg, false);
    if (err != ESP_OK) return err;  // scanning stays false on failure
    scanning = true;
    return ESP_OK;
}

bool scanner_wifi_scanning(void) {
    return scanning;
}
