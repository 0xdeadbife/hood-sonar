#pragma once
#include <stdint.h>

// Print the hello message (device identity + capabilities) to stdout (USB CDC).
void scanner_proto_hello(const uint8_t *mac, int heap, int wifi, int ble);

// Serialize the device table to a JSON buffer. Returns bytes written, or -1 on
// a format error. If the buffer is too small, a partial snapshot is written
// instead ("n" counts the devices actually emitted). Key order: d first, then
// n (the count is only known after the walk).
int scanner_proto_snap_to_buf(char *buf, int buf_size);

// Serialize the table and print it to stdout (USB CDC). Takes the table lock
// only while serializing; the (long) print happens outside the lock so Wi-Fi
// and BLE callbacks are not blocked mid-snapshot.
void scanner_proto_snap(void);

// Serialize a board-health stat line (uptime/heap/device count/radios) to a
// buffer. Returns bytes written.
int scanner_proto_stat_to_buf(char *buf, int buf_size, uint32_t up_ms,
                              int heap, int devs, int wifi, int ble);

// Serialize the stat and print it to stdout (USB CDC).
void scanner_proto_stat(uint32_t up_ms, int heap, int devs, int wifi, int ble);
