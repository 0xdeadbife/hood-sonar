#include "scanner_classify.h"
#include "scanner_oui.h"
#include <ctype.h>
#include <strings.h>

// ---- Name keyword rules (ordered by specificity; first match wins) ----
static const name_rule_t NAME_RULES[] = {
    // Traffic / semaforos
    { "semaforo",     CAT_TRAFFIC },
    { "semaphore",    CAT_TRAFFIC },
    { "traffic",      CAT_TRAFFIC },
    { "crosswalk",    CAT_TRAFFIC },
    { "pedestrian",   CAT_TRAFFIC },
    { "signal",       CAT_TRAFFIC },
    { "v2x",          CAT_TRAFFIC },
    { "tsl",          CAT_TRAFFIC },
    // Facade / media facade / video wall
    { "media facade", CAT_FACADE },
    { "video wall",   CAT_FACADE },
    { "billboard",    CAT_FACADE },
    { "facade",       CAT_FACADE },
    { "outdoor",      CAT_FACADE },
    { "building",     CAT_FACADE },
    // Signage / LED banners
    { "led sign",     CAT_SIGNAGE },
    { "led banner",   CAT_SIGNAGE },
    { "digital sign", CAT_SIGNAGE },
    { "signage",      CAT_SIGNAGE },
    { "banner",       CAT_SIGNAGE },
    // Streetlight / alumbrado
    { "streetlight",  CAT_STREETLIGHT },
    { "street light", CAT_STREETLIGHT },
    { "solar light",  CAT_STREETLIGHT },
    { "luminaire",    CAT_STREETLIGHT },
    // Parking
    { "parking",      CAT_PARKING },
    { "park sensor",  CAT_PARKING },
    { "curb",         CAT_PARKING },
    // Camera / CCTV
    { "hikvision",    CAT_CAMERA },
    { "dahua",        CAT_CAMERA },
    { "reolink",      CAT_CAMERA },
    { "axis",         CAT_CAMERA },
    { "cctv",         CAT_CAMERA },
    { "camera",       CAT_CAMERA },
    { "ipc",          CAT_CAMERA },
    { "dvr",          CAT_CAMERA },
    { "nvr",          CAT_CAMERA },
    { "cam",          CAT_CAMERA },
    // Printer
    { "laserjet",     CAT_PRINTER },
    { "officejet",    CAT_PRINTER },
    { "imageclass",   CAT_PRINTER },
    { "ecotank",      CAT_PRINTER },
    { "workforce",    CAT_PRINTER },
    { "canon",        CAT_PRINTER },
    { "epson",        CAT_PRINTER },
    { "brother",      CAT_PRINTER },
    { "printer",      CAT_PRINTER },
    // Hotspot
    { "hotspot",      CAT_HOTSPOT },
    { "mifi",         CAT_HOTSPOT },
    { "mi router",    CAT_HOTSPOT },
    { "xpon",         CAT_HOTSPOT },
    { "optix",        CAT_HOTSPOT },
    // LED / smart lights
    { "nanoleaf",     CAT_LED },
    { "govee",        CAT_LED },
    { "hue",          CAT_LED },
    { "led",          CAT_LED },
    { "light",        CAT_LED },
    { "lamp",         CAT_LED },
    // Screen / display
    { "smart tv",     CAT_SCREEN },
    { "screen",       CAT_SCREEN },
    { "display",      CAT_SCREEN },
    { "monitor",      CAT_SCREEN },
    { "tv",           CAT_SCREEN },
    // Router / AP
    { "router",       CAT_ROUTER },
    { "mesh",         CAT_ROUTER },
    { "repeater",     CAT_ROUTER },
    { "extender",     CAT_ROUTER },
    { "wifi",         CAT_ROUTER },
    { "ap",           CAT_ROUTER },
    // Phone
    { "iphone",       CAT_PHONE },
    { "galaxy",       CAT_PHONE },
    { "pixel",        CAT_PHONE },
    { "xiaomi",       CAT_PHONE },
    { "redmi",        CAT_PHONE },
    { "oneplus",      CAT_PHONE },
    { "mate",         CAT_PHONE },
    { "nova",         CAT_PHONE },
    // IoT
    { "thermostat",   CAT_IOT },
    { "sensor",       CAT_IOT },
    { "smart",        CAT_IOT },
    { "plug",         CAT_IOT },
    { "switch",       CAT_IOT },
    { "iot",          CAT_IOT },
};
#define N_NAME_RULES (sizeof(NAME_RULES)/sizeof(NAME_RULES[0]))

// ---- BLE service UUID rules (16-bit assigned GATT services) ----
// Only services that say something about the device type. Battery Service
// (0x180F) is deliberately NOT mapped: virtually every BLE device advertises
// it, so it carries no type information.
static const uuid_rule_t UUID_RULES[] = {
    { 0x180D, CAT_PHONE },   // Heart Rate (wearable)
    { 0x1810, CAT_PHONE },   // Blood Pressure (wearable)
    { 0x181B, CAT_PHONE },   // Body Composition (wearable)
    { 0x1812, CAT_PHONE },   // HID: keyboards/mice/headsets (accessory)
    { 0x1809, CAT_IOT },     // Health Thermometer (sensor)
    { 0x1814, CAT_IOT },     // Running Speed and Cadence (sensor)
    { 0x1816, CAT_IOT },     // Cycling Speed and Cadence (sensor)
    { 0x1817, CAT_IOT },     // Cycling Power (sensor)
    { 0x1818, CAT_IOT },     // Location and Navigation (tracker/puck)
    { 0x181A, CAT_IOT },     // Environmental Sensing (sensor)
    { 0x181D, CAT_IOT },     // Weight Scale (sensor)
};
#define N_UUID_RULES (sizeof(UUID_RULES)/sizeof(UUID_RULES[0]))

// Case-insensitive substring match. For short keywords (<=3 chars) require
// word boundaries so "ap" doesn't match "app"/"cap".
static bool match_kw(const char *name, const char *kw) {
    if (!name || !name[0] || !kw || !kw[0]) return false;
    size_t klen = strlen(kw);
    const char *p = name;
    while (*p) {
        if (strncasecmp(p, kw, klen) == 0) {
            if (klen <= 3) {
                char prev = (p == name) ? 0 : p[-1];
                char next = p[klen];
                bool bprev = (prev == 0) || (!isalpha((unsigned char)prev) && prev != '_');
                bool bnext = (next == 0) || (!isalpha((unsigned char)next) && next != '_');
                if (bprev && bnext) return true;
            } else {
                return true;
            }
        }
        p++;
    }
    return false;
}

void scanner_classify(const uint8_t *mac, const char *name,
                      const uint16_t *uuids, int n_uuids,
                      const char **vendor, cat_t *cat) {
    // 1. OUI -> vendor + base category
    cat_t base = CAT_UNKNOWN;
    const char *v = NULL;
    if (mac) scanner_oui_lookup(mac, &v, &base);
    if (vendor) *vendor = v ? v : "";

    // 2. name keyword
    cat_t name_cat = CAT_UNKNOWN;
    if (name && name[0]) {
        for (size_t i = 0; i < N_NAME_RULES; i++) {
            if (match_kw(name, NAME_RULES[i].kw)) { name_cat = NAME_RULES[i].cat; break; }
        }
    }

    // 3. BLE service UUID
    cat_t uuid_cat = CAT_UNKNOWN;
    if (uuids && n_uuids > 0) {
        for (int i = 0; i < n_uuids && uuid_cat == CAT_UNKNOWN; i++) {
            for (size_t j = 0; j < N_UUID_RULES; j++) {
                if (uuids[i] == UUID_RULES[j].uuid) { uuid_cat = UUID_RULES[j].cat; break; }
            }
        }
    }

    // priority: name > uuid > base
    if (name_cat != CAT_UNKNOWN)      *cat = name_cat;
    else if (uuid_cat != CAT_UNKNOWN) *cat = uuid_cat;
    else                              *cat = base;
}
