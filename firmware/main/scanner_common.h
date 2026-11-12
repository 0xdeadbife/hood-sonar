#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// ---- Device categories (urban / building-surface focus) ----
typedef enum {
    CAT_UNKNOWN = 0,
    CAT_CAMERA,       // IP cam / CCTV
    CAT_PHONE,        // mobile phone
    CAT_PRINTER,      // printer
    CAT_HOTSPOT,      // MiFi / mobile / public hotspot
    CAT_LED,          // smart lights / LED (indoor)
    CAT_SIGNAGE,      // LED banners / signs / digital signage
    CAT_FACADE,       // building surface: media facade, video wall, billboard
    CAT_SCREEN,       // TV / display / monitor
    CAT_TRAFFIC,      // semaforo / traffic light
    CAT_STREETLIGHT,  // alumbrado publico / outdoor luminaire
    CAT_PARKING,      // parking sensor / meter
    CAT_ROUTER,       // Wi-Fi router / AP / small cell
    CAT_IOT,          // generic IoT / sensor
    CAT_BLE,          // generic BLE device / beacon
} cat_t;

// ---- Signal source ----
typedef enum {
    SRC_NONE = 0,
    SRC_WIFI,
    SRC_BLE,
} src_t;

// ---- Wi-Fi auth (simplified) ----
typedef enum {
    AUTH_OPEN = 0,
    AUTH_WEP,
    AUTH_WPA,
    AUTH_WPA2,
    AUTH_WPA3,
    AUTH_WPA2_WPA3,
} auth_t;

// ---- A tracked device ----
typedef struct {
    uint8_t  mac[6];
    src_t    src;
    cat_t    cat;
    char     name[32];    // SSID or BLE adv name
    char     vendor[16];  // from OUI
    int8_t   rssi;        // dBm (best seen)
    int8_t   rssi_last;   // dBm (last seen)
    uint8_t  channel;     // Wi-Fi channel (0 for BLE)
    auth_t   auth;        // Wi-Fi auth (AUTH_OPEN for BLE)
    uint32_t first_ms;    // first seen (ms since boot)
    uint32_t last_ms;     // last seen (ms since boot)
    uint16_t count;       // times seen
    bool     valid;
} device_t;

// ---- Table limits ----
#define MAX_DEVICES 128
#define NAME_LEN    32
#define VENDOR_LEN  16

// ---- OUI entry ----
typedef struct {
    uint8_t    oui[3];
    const char *vendor;
    cat_t      cat;
} oui_t;

// ---- Name / UUID rules (classify) ----
typedef struct {
    const char *kw;
    cat_t       cat;
} name_rule_t;

typedef struct {
    uint16_t uuid;
    cat_t    cat;
} uuid_rule_t;

// ---- Helpers (scanner_common.c) ----
const char *cat_name(cat_t c);
const char *src_name(src_t s);
const char *auth_name(auth_t a);

// Milliseconds since boot (esp_timer on IDF, 0 on host tests).
uint32_t scanner_now_ms(void);

// Extract the "short local name" (AD type 0x08) or "complete local name"
// (0x09) from a BLE advertising data buffer (a sequence of AD structures:
// [len][type][payload...] where len counts type + payload bytes).
// Copies at most out_sz-1 bytes and always NUL-terminates.
// Returns the name length (>=0), or 0 if no name is present.
int ble_adv_get_name(const uint8_t *data, int len, char *out, int out_sz);
