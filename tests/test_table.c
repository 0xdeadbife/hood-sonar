#include <stdio.h>
#include <string.h>
#include "scanner_table.h"

static int fails = 0;
#define CHECK(cond, msg) do { \
    int ok = (cond); \
    printf("%s  %s\n", ok ? "PASS" : "FAIL", msg); \
    if (!ok) fails++; \
} while (0)

int main(void) {
    scanner_table_init();

    const uint8_t macA[6] = {0x3C,0x5A,0xB5,0x01,0x02,0x03};
    const uint8_t macB[6] = {0x00,0x16,0x6C,0x04,0x05,0x06};
    const uint8_t macC[6] = {0xDE,0xAD,0xBE,0xEF,0x00,0x01};

    // add A
    scanner_table_add(macA, SRC_WIFI, "Apple-Phone", "Apple", CAT_PHONE, -50, 6, AUTH_WPA2, 1000);
    CHECK(scanner_table_count() == 1, "count==1 after add A");
    CHECK(scanner_table_find(macA, SRC_WIFI) >= 0, "find A");

    // add A again (dedup)
    scanner_table_add(macA, SRC_WIFI, "Apple-Phone", "Apple", CAT_PHONE, -55, 6, AUTH_WPA2, 2000);
    CHECK(scanner_table_count() == 1, "count==1 after re-add A (dedup)");
    const device_t *a = scanner_table_get(scanner_table_find(macA, SRC_WIFI));
    CHECK(a->count == 2, "A.count==2");
    CHECK(a->rssi == -50, "A.rssi==-50 (best seen, -50 > -55)");
    CHECK(a->rssi_last == -55, "A.rssi_last==-55");
    CHECK(a->last_ms == 2000, "A.last_ms==2000");

    // refresh policy: empty name / generic cat must not clobber good values
    scanner_table_add(macA, SRC_WIFI, "Renamed-Cam", "NewVendor", CAT_CAMERA, -60, 11, AUTH_WPA3, 2500);
    a = scanner_table_get(scanner_table_find(macA, SRC_WIFI));
    CHECK(a->name != NULL && strcmp(a->name, "Renamed-Cam") == 0, "A.name refreshed with new name");
    CHECK(a->vendor != NULL && strcmp(a->vendor, "NewVendor") == 0, "A.vendor refreshed");
    CHECK(a->cat == CAT_CAMERA, "A.cat refreshed");
    scanner_table_add(macA, SRC_WIFI, "", "", CAT_UNKNOWN, -60, 11, AUTH_WPA3, 2600);
    a = scanner_table_get(scanner_table_find(macA, SRC_WIFI));
    CHECK(a->name != NULL && strcmp(a->name, "Renamed-Cam") == 0, "A.name kept on empty update");
    CHECK(a->vendor != NULL && strcmp(a->vendor, "NewVendor") == 0, "A.vendor kept on empty update");
    CHECK(a->cat == CAT_CAMERA, "A.cat kept on CAT_UNKNOWN update");
    CHECK(a->rssi == -50, "A.rssi still best seen (-50 > -60)");
    CHECK(a->count == 4, "A.count==4 after all updates");

    // add B
    scanner_table_add(macB, SRC_WIFI, "Samsung", "Samsung", CAT_PHONE, -60, 1, AUTH_OPEN, 3000);
    CHECK(scanner_table_count() == 2, "count==2 after add B");

    // same MAC but different source (BLE) -> separate entry
    scanner_table_add(macB, SRC_BLE, "Samsung-BLE", "Samsung", CAT_BLE, -40, 0, AUTH_OPEN, 4000);
    CHECK(scanner_table_count() == 3, "count==3 (same MAC, BLE source separate)");

    // add C
    scanner_table_add(macC, SRC_BLE, "Hue Bulb", "Hue", CAT_LED, -70, 0, AUTH_OPEN, 5000);
    CHECK(scanner_table_count() == 4, "count==4 after add C");

    // eviction: fill to MAX_DEVICES then add one more
    scanner_table_init();
    for (int i = 0; i < MAX_DEVICES; i++) {
        uint8_t m[6] = {0x00,0x11,0x22,(uint8_t)(i>>8),(uint8_t)i,0x55};
        scanner_table_add(m, SRC_WIFI, "dev", "X", CAT_IOT, -50, 1, AUTH_OPEN, (uint32_t)(i*10));
    }
    CHECK(scanner_table_count() == MAX_DEVICES, "count==MAX after fill");
    // add one more -> evict oldest (smallest last_ms = i=0)
    uint8_t mNew[6] = {0xFF,0xFF,0xFF,0x01,0x02,0x03};
    scanner_table_add(mNew, SRC_WIFI, "new", "X", CAT_IOT, -50, 1, AUTH_OPEN, 999999);
    CHECK(scanner_table_count() == MAX_DEVICES, "count==MAX after eviction");
    CHECK(scanner_table_find(mNew, SRC_WIFI) >= 0, "new device present after eviction");
    // oldest (i=0) should be evicted
    uint8_t mOld[6] = {0x00,0x11,0x22,0x00,0x00,0x55};
    CHECK(scanner_table_find(mOld, SRC_WIFI) < 0, "oldest evicted");

    // clear
    scanner_table_clear();
    CHECK(scanner_table_count() == 0, "count==0 after clear");

    printf(fails ? "TEST_TABLE FAIL (%d)\n" : "TEST_TABLE OK\n", fails);
    return fails ? 1 : 0;
}
