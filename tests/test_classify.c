#include <stdio.h>
#include <string.h>
#include "scanner_classify.h"

static int fails = 0;

static void check(const uint8_t *mac, const char *name,
                  const uint16_t *uuids, int n, cat_t want) {
    const char *vendor = NULL;
    cat_t cat = CAT_UNKNOWN;
    scanner_classify(mac, name, uuids, n, &vendor, &cat);
    int ok = cat == want;
    printf("%s  name=\"%s\" mac=%02x:%02x:%02x uuid=%d -> %s (want %s) vendor=%s\n",
           ok ? "PASS" : "FAIL", name ? name : "",
           mac ? mac[0] : 0, mac ? mac[1] : 0, mac ? mac[2] : 0,
           n, cat_name(cat), cat_name(want), vendor ? vendor : "?");
    if (!ok) fails++;
}

int main(void) {
    const uint8_t apple[3]  = {0x3C,0x5A,0xB5};   // base CAT_PHONE
    const uint8_t hue[3]    = {0x00,0x12,0x4B};   // base CAT_LED
    const uint8_t unk[3]    = {0xDE,0xAD,0xBE};   // base CAT_UNKNOWN

    // OUI only (no name, no uuid)
    check(apple, NULL, NULL, 0, CAT_PHONE);
    check(hue,   NULL, NULL, 0, CAT_LED);

    // name keyword wins
    check(unk, "Hikvision-01",   NULL, 0, CAT_CAMERA);
    check(unk, "semaforo-123",   NULL, 0, CAT_TRAFFIC);
    check(unk, "Media Facade",   NULL, 0, CAT_FACADE);
    check(unk, "HP LaserJet",    NULL, 0, CAT_PRINTER);
    check(unk, "Hue Bulb",       NULL, 0, CAT_LED);
    check(unk, "LED Wall",       NULL, 0, CAT_LED);
    check(unk, "Smart TV",       NULL, 0, CAT_SCREEN);
    check(unk, "parking sensor", NULL, 0, CAT_PARKING);

    // name > base (Apple OUI but name says LED)
    check(apple, "Hue Bulb", NULL, 0, CAT_LED);

    // uuid wins when no name
    const uint16_t hr[1] = {0x180D};       // Heart Rate (wearable)
    check(unk, "", hr, 1, CAT_PHONE);
    const uint16_t env[1] = {0x181A};      // Environmental Sensing (sensor)
    check(unk, "", env, 1, CAT_IOT);
    // Battery Service (0x180F) is intentionally unmapped: generic.
    const uint16_t batt[1] = {0x180F};
    check(unk, "", batt, 1, CAT_UNKNOWN);

    // name > uuid (name is more specific)
    const uint16_t devinfo[1] = {0x180A}; // not in table -> no uuid_cat
    check(unk, "Hue Bulb", devinfo, 1, CAT_LED);

    printf(fails ? "TEST_CLASSIFY FAIL (%d)\n" : "TEST_CLASSIFY OK\n", fails);
    return fails ? 1 : 0;
}
