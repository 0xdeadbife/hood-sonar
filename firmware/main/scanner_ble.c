#include "scanner_ble.h"
#include "scanner_table.h"
#include "scanner_classify.h"
#include "scanner_common.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "host/ble_gap.h"
#include <string.h>

static bool scanning = false;
static bool want_scan = false;  // start discovery as soon as the host syncs

static int scan_cb(struct ble_gap_event *event, void *arg);

static void ble_on_reset(int reason) {
    (void)reason;
    scanning = false;  // the host dropped any running discovery procedure
}

static void start_disc(void) {
    if (scanning) return;
    struct ble_gap_disc_params disc = {
        .itvl = 0x0012,
        .window = 0x0012,
        .filter_duplicates = 0,  // keep count/last-seen alive (see scanner_ble.h)
        .passive = 0,
    };
    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc, scan_cb, NULL);
    scanning = (rc == 0);
}

/* The host only accepts GAP commands once it is synchronized with the
 * controller. Starting discovery here is the canonical pattern: it also
 * (re)starts scanning after a controller reset, and picks up scanner_ble_
 * scan_start() calls made before sync (BLE_HS_ENOTSYNCED). */
static void ble_on_sync(void) {
    if (want_scan) start_disc();
}

static void ble_host_task(void *param) {
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static int scan_cb(struct ble_gap_event *event, void *arg) {
    (void)arg;
    if (event->type == BLE_GAP_EVENT_DISC) {
        struct ble_gap_disc_desc *d = &event->disc;
        // Scan responses arrive as separate DISC events (same address): the
        // table dedup updates the entry with the name once it appears.
        char name[NAME_LEN] = {0};
        ble_adv_get_name(d->data, d->length_data, name, sizeof(name));
        const uint8_t *addr = d->addr.val;
        const char *vendor = NULL;
        cat_t cat = CAT_BLE;
        scanner_classify(addr, name, NULL, 0, &vendor, &cat);
        scanner_table_add(addr, SRC_BLE, name, vendor, cat,
                          (int8_t)d->rssi, 0, AUTH_OPEN, scanner_now_ms());
    }
    return 0;
}

esp_err_t scanner_ble_init(void) {
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) return ret;

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;

    nimble_port_freertos_init(ble_host_task);
    return ESP_OK;
}

esp_err_t scanner_ble_scan_start(void) {
    if (scanning) return ESP_ERR_INVALID_STATE;
    want_scan = true;
    start_disc();  // if the host is not synced yet, ble_on_sync() retries
    return scanning ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t scanner_ble_scan_stop(void) {
    want_scan = false;
    int rc = ble_gap_disc_cancel();
    if (rc == 0) {
        scanning = false;
        return ESP_OK;
    }
    return ESP_FAIL;
}

bool scanner_ble_scanning(void) {
    return scanning;
}
