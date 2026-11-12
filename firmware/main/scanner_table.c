#include "scanner_table.h"
#include <string.h>

static device_t table[MAX_DEVICES];

#ifdef CONFIG_IDF_TARGET
// IDF build: FreeRTOS mutex (multi-task: Wi-Fi/BLE callbacks + main loop).
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
static SemaphoreHandle_t table_mutex = NULL;
static void ensure_mutex(void) {
    if (!table_mutex) table_mutex = xSemaphoreCreateMutex();
}
void scanner_table_lock(void) {
    ensure_mutex();
    if (table_mutex) xSemaphoreTake(table_mutex, portMAX_DELAY);
}
void scanner_table_unlock(void) {
    if (table_mutex) xSemaphoreGive(table_mutex);
}
#else
// Host test (gcc): single-threaded, no-op locks.
void scanner_table_lock(void) {}
void scanner_table_unlock(void) {}
#endif

static void copy_name(char *dst, const char *src, int len) {
    if (src && src[0]) {
        strncpy(dst, src, len - 1);
        dst[len - 1] = 0;
    } else {
        dst[0] = 0;
    }
}

void scanner_table_init(void) {
    memset(table, 0, sizeof(table));
#ifdef CONFIG_IDF_TARGET
    ensure_mutex();  // create once, before any concurrent add
#endif
}

void scanner_table_clear(void) {
    memset(table, 0, sizeof(table));
}

int scanner_table_count(void) {
    int c = 0;
    for (int i = 0; i < MAX_DEVICES; i++) if (table[i].valid) c++;
    return c;
}

const device_t *scanner_table_get(int idx) {
    if (idx < 0 || idx >= MAX_DEVICES) return NULL;
    return table[idx].valid ? &table[idx] : NULL;
}

int scanner_table_find(const uint8_t *mac, src_t src) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (table[i].valid && table[i].src == src &&
            memcmp(table[i].mac, mac, 6) == 0) {
            return i;
        }
    }
    return -1;
}

void scanner_table_add(const uint8_t *mac, src_t src, const char *name,
                       const char *vendor, cat_t cat, int8_t rssi,
                       uint8_t channel, auth_t auth, uint32_t now_ms) {
    scanner_table_lock();
    int idx = scanner_table_find(mac, src);
    if (idx >= 0) {
        device_t *d = &table[idx];
        if (rssi > d->rssi) d->rssi = rssi;   // best seen
        d->rssi_last = rssi;
        d->last_ms = now_ms;
        if (d->count < 0xFFFF) d->count++;
        // Refresh only with meaningful values: an adv without a name or a
        // generic "unknown" must not clobber a good name/classification.
        if (name && name[0])   copy_name(d->name, name, NAME_LEN);
        if (vendor && vendor[0]) copy_name(d->vendor, vendor, VENDOR_LEN);
        if (cat != CAT_UNKNOWN) d->cat = cat;
        d->channel = channel;
        d->auth = auth;
        scanner_table_unlock();
        return;
    }

    // find a free slot
    int free_idx = -1;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!table[i].valid) { free_idx = i; break; }
    }
    // if full, evict the oldest (smallest last_ms)
    if (free_idx < 0) {
        int oldest = 0;
        for (int i = 1; i < MAX_DEVICES; i++) {
            if (table[i].last_ms < table[oldest].last_ms) oldest = i;
        }
        free_idx = oldest;
    }

    device_t *d = &table[free_idx];
    memset(d, 0, sizeof(*d));
    memcpy(d->mac, mac, 6);
    d->src = src;
    d->cat = cat;
    copy_name(d->name, name, NAME_LEN);
    copy_name(d->vendor, vendor, VENDOR_LEN);
    d->rssi = rssi;
    d->rssi_last = rssi;
    d->channel = channel;
    d->auth = auth;
    d->first_ms = now_ms;
    d->last_ms = now_ms;
    d->count = 1;
    d->valid = true;
    scanner_table_unlock();
}
