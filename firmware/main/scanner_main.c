#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "scanner_wifi.h"
#include "scanner_ble.h"
#include "scanner_table.h"
#include "scanner_proto.h"

static const char *TAG = "scanner";

void app_main(void) {
    ESP_LOGI(TAG, "ESP32-C3 Urban Scanner boot");

    // NVS (needed by Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Device table (also creates its mutex before any concurrent add)
    scanner_table_init();

    // Wi-Fi scanner. Tolerant to failure: the board keeps running (BLE-only).
    esp_err_t werr = scanner_wifi_init();
    bool wifi_ok = (werr == ESP_OK);
    if (wifi_ok) {
        if (scanner_wifi_scan_start() != ESP_OK) {
            ESP_LOGW(TAG, "Wi-Fi scan start failed (will retry)");
        } else {
            ESP_LOGI(TAG, "Wi-Fi scan started");
        }
    } else {
        ESP_LOGE(TAG, "Wi-Fi init failed: %s", esp_err_to_name(werr));
    }

    // BLE scanner (NimBLE). Tolerant to failure too.
    esp_err_t berr = scanner_ble_init();
    bool ble_ok = (berr == ESP_OK);
    if (ble_ok) {
        vTaskDelay(pdMS_TO_TICKS(200));  // let the NimBLE host task start
        if (scanner_ble_scan_start() != ESP_OK) {
            ESP_LOGW(TAG, "BLE not synced yet; discovery will start on sync");
        } else {
            ESP_LOGI(TAG, "BLE scan started");
        }
    } else {
        ESP_LOGE(TAG, "BLE init failed: %s", esp_err_to_name(berr));
    }

    // Read MAC (runtime, not hardcoded)
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Hello (device identity + real capabilities)
    int heap = (int)heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    scanner_proto_hello(mac, heap, wifi_ok ? 1 : 0, ble_ok ? 1 : 0);

    // Main loop: start a new Wi-Fi scan every ~2.5 s, snap every ~2 s,
    // board-health stat every ~10 s.
    TickType_t last_snap = xTaskGetTickCount();
    TickType_t last_scan = xTaskGetTickCount();
    TickType_t last_stat = xTaskGetTickCount();
    const TickType_t SNAP_PERIOD = pdMS_TO_TICKS(2000);
    const TickType_t SCAN_PERIOD = pdMS_TO_TICKS(2500);
    const TickType_t STAT_PERIOD = pdMS_TO_TICKS(10000);

    while (1) {
        TickType_t now = xTaskGetTickCount();
        if (wifi_ok && !scanner_wifi_scanning() && (now - last_scan) >= SCAN_PERIOD) {
            last_scan = now;
            if (scanner_wifi_scan_start() != ESP_OK) {
                ESP_LOGW(TAG, "Wi-Fi scan start failed (will retry)");
            }
        }
        if ((now - last_snap) >= SNAP_PERIOD) {
            last_snap = now;
            scanner_proto_snap();  // locks the table only while serializing
        }
        if ((now - last_stat) >= STAT_PERIOD) {
            last_stat = now;
            scanner_table_lock();
            int devs = scanner_table_count();
            scanner_table_unlock();
            scanner_proto_stat((uint32_t)(esp_timer_get_time() / 1000),
                               (int)heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
                               devs, wifi_ok, ble_ok);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
