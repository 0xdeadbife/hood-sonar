#include <stdio.h>
#include <string.h>
#include "scanner_oui.h"

static int fails = 0;

static void check(const uint8_t *mac, const char *want_vendor, cat_t want_cat) {
    const char *vendor = NULL;
    cat_t cat = CAT_UNKNOWN;
    bool found = scanner_oui_lookup(mac, &vendor, &cat);
    int ok = found && vendor && strcmp(vendor, want_vendor) == 0 && cat == want_cat;
    printf("%s  %02x:%02x:%02x -> %s / %s (want %s / %s)\n",
           ok ? "PASS" : "FAIL", mac[0], mac[1], mac[2],
           vendor ? vendor : "?", cat_name(cat), want_vendor, cat_name(want_cat));
    if (!ok) fails++;
}

int main(void) {
    unsigned int n = scanner_oui_count();
    printf("OUI table count = %u (want >= 150)\n", n);
    if (n < 150) { printf("FAIL count < 150\n"); fails++; }
    else { printf("PASS count >= 150\n"); }

    check((const uint8_t[]){0x3C,0x5A,0xB5}, "Apple",     CAT_PHONE);
    check((const uint8_t[]){0x00,0x16,0x6C}, "Samsung",   CAT_PHONE);
    check((const uint8_t[]){0x00,0x1B,0x2A}, "HP",        CAT_PRINTER);
    check((const uint8_t[]){0x00,0x0D,0x42}, "Hikvision", CAT_CAMERA);
    check((const uint8_t[]){0x50,0x63,0x46}, "TP-Link",   CAT_ROUTER);

    // Unknown OUI -> miss
    const uint8_t unk[3] = {0xDE,0xAD,0xBE};
    const char *v = NULL; cat_t c = CAT_UNKNOWN;
    bool f = scanner_oui_lookup(unk, &v, &c);
    int ok = !f && c == CAT_UNKNOWN;
    printf("%s  unknown %02x:%02x:%02x -> %s\n", ok ? "PASS" : "FAIL",
           unk[0], unk[1], unk[2], f ? "found" : "miss");
    if (!ok) fails++;

    printf(fails ? "TEST_OUI FAIL (%d)\n" : "TEST_OUI OK\n", fails);
    return fails ? 1 : 0;
}
