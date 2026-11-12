#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner_table.h"
#include "scanner_proto.h"

// Diagnostics go to stderr; stdout carries exactly one JSON line (the snap)
// so the JSON round-trip check `... | python3 -c 'json.loads(stdin)'` works.

static int fails = 0;
#define CHECK(cond, msg) do { \
    int ok = (cond); \
    fprintf(stderr, "%s  %s\n", ok ? "PASS" : "FAIL", msg); \
    if (!ok) fails++; \
} while (0)

static int count_str(const char *hay, const char *needle) {
    int n = 0;
    const char *p = hay;
    while ((p = strstr(p, needle)) != NULL) { n++; p++; }
    return n;
}

int main(void) {
    char buf[65536];

    scanner_table_init();
    const uint8_t macA[6] = {0x3C,0x5A,0xB5,0x01,0x02,0x03};
    const uint8_t macB[6] = {0x00,0x16,0x6C,0x04,0x05,0x06};
    scanner_table_add(macA, SRC_WIFI, "Hikvision-01", "Hikvision", CAT_CAMERA, -52, 6, AUTH_WPA2, 1234);
    scanner_table_add(macB, SRC_BLE, "Hue Bulb", "Hue", CAT_LED, -60, 0, AUTH_OPEN, 2345);

    // basic shape: d first, honest n at the end
    int len = scanner_proto_snap_to_buf(buf, sizeof(buf));
    CHECK(len > 0, "snap_to_buf returns >0");
    CHECK(strncmp(buf, "{\"t\":\"snap\",\"d\":[", 17) == 0, "header ok");
    CHECK(count_str(buf, "\"mac\":") == 2, "two devices in snap");
    CHECK(strstr(buf, "],\"n\":2}") != NULL, "n=2 tail ok");

    // escaping: quotes/backslash in the name must stay valid JSON
    scanner_table_init();
    const uint8_t macQ[6] = {0xDE,0xAD,0xBE,0xEF,0x00,0x09};
    scanner_table_add(macQ, SRC_BLE, "Bad\"Name\\x", "V", CAT_BLE, -60, 0, AUTH_OPEN, 10);
    len = scanner_proto_snap_to_buf(buf, sizeof(buf));
    CHECK(len > 0 && strstr(buf, "Bad\\\"Name\\\\x") != NULL, "quote/backslash escaped");

    // fill the table with worst-case 31-char names: must still emit all 128
    scanner_table_init();
    for (int i = 0; i < MAX_DEVICES; i++) {
        uint8_t m[6] = {0x02,0x00,0x00,(uint8_t)(i>>8),(uint8_t)i,0x77};
        char nm[NAME_LEN + 1];  // worst-case 31-char names
        int off = snprintf(nm, sizeof(nm), "PAD%04d-", i);
        if (off < 0 || off > NAME_LEN - 1) off = NAME_LEN - 1;
        memset(nm + off, 'x', (size_t)(NAME_LEN - off));
        nm[NAME_LEN] = '\0';
        scanner_table_add(m, SRC_WIFI, nm, "Vend0r", CAT_IOT, -50, 1, AUTH_WPA2, (uint32_t)(100 + i));
    }
    len = scanner_proto_snap_to_buf(buf, sizeof(buf));
    CHECK(len > 0, "full-table snap fits the 64 KB test buffer");
    CHECK(count_str(buf, "\"mac\":") == MAX_DEVICES, "all 128 devices emitted");
    CHECK(strstr(buf, "],\"n\":128}") != NULL, "n=128 tail ok");

    // tiny buffer: partial emission instead of a failed snap
    len = scanner_proto_snap_to_buf(buf, 1024);
    CHECK(len > 0 && len < 1024, "small buffer: partial snap, no error");
    int emitted = count_str(buf, "\"mac\":");
    char tail[32];
    snprintf(tail, sizeof(tail), "],\"n\":%d}", emitted);
    CHECK(strstr(buf, tail) != NULL, "n matches emitted devices");

    // stat line shape
    len = scanner_proto_stat_to_buf(buf, sizeof(buf), 123456, 107944, 8, 1, 0);
    CHECK(len > 0 && strcmp(buf,
        "{\"t\":\"stat\",\"up\":123456,\"heap\":107944,\"devs\":8,\"wifi\":1,\"ble\":0}") == 0,
        "stat line format ok");

    // stdout: exactly one JSON line, from a fresh 2-device table
    scanner_table_init();
    scanner_table_add(macA, SRC_WIFI, "Hikvision-01", "Hikvision", CAT_CAMERA, -52, 6, AUTH_WPA2, 1234);
    scanner_table_add(macB, SRC_BLE, "Hue Bulb", "Hue", CAT_LED, -60, 0, AUTH_OPEN, 2345);
    scanner_proto_snap();

    fprintf(stderr, fails ? "TEST_PROTO FAIL (%d)\n" : "TEST_PROTO OK\n", fails);
    return fails ? 1 : 0;
}
