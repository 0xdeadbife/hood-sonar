#include <stdio.h>
#include <string.h>
#include "scanner_common.h"

static int fails = 0;
#define CHECK(cond, msg) do { \
    int ok = (cond); \
    printf("%s  %s\n", ok ? "PASS" : "FAIL", msg); \
    if (!ok) fails++; \
} while (0)

int main(void) {
    char name[32];

    // flags (0x01) then complete name (0x09): the classic layout
    const uint8_t adv1[] = {0x02,0x01,0x06, 0x04,0x09,'H','u','e'};
    CHECK(ble_adv_get_name(adv1, sizeof(adv1), name, sizeof(name)) == 3 &&
          strcmp(name, "Hue") == 0, "flags + complete name -> \"Hue\"");

    // name is the first structure (short name 0x08)
    const uint8_t adv2[] = {0x05,0x08,'A','B','C','D', 0x03,0x19,0x00,0x00};
    CHECK(ble_adv_get_name(adv2, sizeof(adv2), name, sizeof(name)) == 4 &&
          strcmp(name, "ABCD") == 0, "short name as first AD struct");

    // no name at all
    const uint8_t adv3[] = {0x02,0x01,0x06, 0x03,0x19,0x00,0x01};
    CHECK(ble_adv_get_name(adv3, sizeof(adv3), name, sizeof(name)) == 0 &&
          name[0] == '\0', "no name -> empty");

    // truncated: length byte claims more bytes than the buffer has
    const uint8_t adv4[] = {0x02,0x01,0x06, 0x0B,0x09,'t','r','u'};
    CHECK(ble_adv_get_name(adv4, sizeof(adv4), name, sizeof(name)) == 0,
          "truncated structure -> no name (no overrun)");

    // zero length byte: malformed, must not loop forever
    const uint8_t adv5[] = {0x00,0x01,0x02, 0x04,0x09,'Z','e','n'};
    CHECK(ble_adv_get_name(adv5, sizeof(adv5), name, sizeof(name)) == 0,
          "zero-length AD struct -> stop, no name");

    // out_sz smaller than the name: truncate, still NUL-terminated
    const uint8_t adv6[] = {0x0B,0x09,'0','1','2','3','4','5','6','7','8','9'};
    char small[6];
    CHECK(ble_adv_get_name(adv6, sizeof(adv6), small, sizeof(small)) == 5 &&
          strlen(small) == 5 && small[5] == '\0', "name truncated to out_sz-1");

    // degenerate inputs
    CHECK(ble_adv_get_name(NULL, 10, name, sizeof(name)) == 0, "NULL data -> 0");
    CHECK(ble_adv_get_name(adv1, 0, name, sizeof(name)) == 0, "len 0 -> 0");
    CHECK(ble_adv_get_name(adv1, sizeof(adv1), NULL, 0) == 0, "NULL out -> 0");

    printf(fails ? "TEST_ADV FAIL (%d)\n" : "TEST_ADV OK\n", fails);
    return fails ? 1 : 0;
}
