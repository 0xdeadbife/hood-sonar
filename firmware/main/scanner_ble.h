#pragma once
#include "esp_err.h"
#include <stdbool.h>

// Initialize the BLE scanner (NimBLE central role).
// Starts the NimBLE host task.
esp_err_t scanner_ble_init(void);

// Start continuous scanning (active, no duplicate filter).
esp_err_t scanner_ble_scan_start(void);

// Stop scanning.
esp_err_t scanner_ble_scan_stop(void);

// Is scanning active?
bool scanner_ble_scanning(void);
