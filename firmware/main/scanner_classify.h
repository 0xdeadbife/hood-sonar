#pragma once
#include "scanner_common.h"

// Classify a device from its MAC, name (SSID or BLE adv name) and optional
// BLE service UUIDs.
//
// Priority (most specific wins):
//   1. name keyword  (SSID / BLE adv name)
//   2. BLE service UUID
//   3. OUI vendor default
//
// *vendor is set from the OUI table ("" if unknown).
// *cat is the final category.
void scanner_classify(const uint8_t *mac, const char *name,
                      const uint16_t *uuids, int n_uuids,
                      const char **vendor, cat_t *cat);
