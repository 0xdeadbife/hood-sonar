#pragma once
#include "esp_err.h"
#include <stdbool.h>

// Initialize the Wi-Fi scanner (STA mode, no connection).
// Sets up netif, event loop, wifi, and registers the scan-complete callback.
esp_err_t scanner_wifi_init(void);

// Start an async active scan. Non-blocking.
// Returns ESP_ERR_INVALID_STATE if a scan is already in progress.
esp_err_t scanner_wifi_scan_start(void);

// Is a scan in progress?
bool scanner_wifi_scanning(void);
