#pragma once
#include "scanner_common.h"

// Lock/unlock the table (for readers that span multiple calls, e.g. the snap).
// scanner_table_add() locks internally; use these around a full snapshot read.
void scanner_table_lock(void);
void scanner_table_unlock(void);

// Initialize (clear) the device table.
void scanner_table_init(void);

// Add or update a device (dedup by MAC + source).
// New  -> allocate a slot (or evict the oldest if full).
// Exist-> update best rssi, rssi_last, last_ms, count++.
//         Name/vendor/cat are refreshed only when the new value is meaningful
//         (non-empty name/vendor, cat != CAT_UNKNOWN) so a nameless adv or a
//         generic default never clobbers a better previous value.
void scanner_table_add(const uint8_t *mac, src_t src, const char *name,
                       const char *vendor, cat_t cat, int8_t rssi,
                       uint8_t channel, auth_t auth, uint32_t now_ms);

// Number of valid devices (0..MAX_DEVICES).
int scanner_table_count(void);

// Device in slot idx (0..MAX_DEVICES-1). NULL for empty slots / out of range.
const device_t *scanner_table_get(int idx);

// Find index by MAC + source. -1 if not found.
int scanner_table_find(const uint8_t *mac, src_t src);

// Clear all devices.
void scanner_table_clear(void);
