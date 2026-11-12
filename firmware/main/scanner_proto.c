#include "scanner_proto.h"
#include "scanner_table.h"
#include "scanner_common.h"
#include <stdio.h>
#include <string.h>

#define SNAP_BUF_SIZE 32768  // 32 KB static: worst case is ~128 devices x ~230 B

static void mac_str(const uint8_t *mac, char *out) {
    snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Escape a string for JSON (backslash, quote; skip non-printable chars).
// Always leaves room for the NUL terminator.
static void json_escape(const char *s, char *out, int out_size) {
    int o = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        if (c == '"' || c == '\\') {
            if (o > out_size - 3) break;   // need room for '\' + c + NUL
            out[o++] = '\\';
            out[o++] = c;
        } else if (c >= 0x20 && c < 0x7f) {
            if (o > out_size - 2) break;   // need room for c + NUL
            out[o++] = c;
        }
        // other (non-printable) chars are dropped to keep the JSON clean
    }
    out[o] = '\0';
}

int scanner_proto_snap_to_buf(char *buf, int buf_size) {
    int p = 0;
    int emitted = 0;
    p += snprintf(buf + p, buf_size - p, "{\"t\":\"snap\",\"d\":[");
    // Iterate slots (not count()) so gaps never stop the walk; stop cleanly
    // (partial snapshot, n = emitted) if the buffer runs out.
    for (int i = 0; i < MAX_DEVICES; i++) {
        const device_t *d = scanner_table_get(i);
        if (!d) continue;
        char mac[18];
        mac_str(d->mac, mac);
        char name[NAME_LEN * 2 + 1];
        json_escape(d->name, name, sizeof(name));
        char vendor[VENDOR_LEN * 2 + 1];
        json_escape(d->vendor, vendor, sizeof(vendor));
        const char *comma = (emitted > 0) ? "," : "";
        int row = snprintf(NULL, 0,
            "%s{\"mac\":\"%s\",\"type\":\"%s\",\"name\":\"%s\",\"rssi\":%d,\"ch\":%d,"
            "\"auth\":\"%s\",\"vendor\":\"%s\",\"src\":\"%s\",\"last\":%lu,\"cnt\":%d}",
            comma, mac, cat_name(d->cat), name, (int)d->rssi, (int)d->channel,
            auth_name(d->auth), vendor, src_name(d->src),
            (unsigned long)d->last_ms, (int)d->count);
        if (row < 0) return -1;
        if (p + row + 12 >= buf_size) break;  // + tail "],\"n\":N}": stop cleanly
        p += snprintf(buf + p, buf_size - p,
            "%s{\"mac\":\"%s\",\"type\":\"%s\",\"name\":\"%s\",\"rssi\":%d,\"ch\":%d,"
            "\"auth\":\"%s\",\"vendor\":\"%s\",\"src\":\"%s\",\"last\":%lu,\"cnt\":%d}",
            comma, mac, cat_name(d->cat), name, (int)d->rssi, (int)d->channel,
            auth_name(d->auth), vendor, src_name(d->src),
            (unsigned long)d->last_ms, (int)d->count);
        emitted++;
    }
    p += snprintf(buf + p, buf_size - p, "],\"n\":%d}", emitted);
    return p;
}

void scanner_proto_snap(void) {
    static char buf[SNAP_BUF_SIZE];
    // Serialize under the table lock, print outside it: a ~30 KB printf must
    // not block Wi-Fi/BLE callbacks from updating the table.
    scanner_table_lock();
    int len = scanner_proto_snap_to_buf(buf, sizeof(buf));
    scanner_table_unlock();
    if (len < 0) return;
    printf("%s\n", buf);
}

void scanner_proto_hello(const uint8_t *mac, int heap, int wifi, int ble) {
    char macstr[18];
    mac_str(mac, macstr);
    printf("{\"t\":\"hello\",\"id\":\"%s\",\"fw\":\"v1.1\",\"heap\":%d,\"wifi\":%d,\"ble\":%d}\n",
           macstr, heap, wifi, ble);
}

int scanner_proto_stat_to_buf(char *buf, int buf_size, uint32_t up_ms,
                              int heap, int devs, int wifi, int ble) {
    return snprintf(buf, buf_size,
        "{\"t\":\"stat\",\"up\":%lu,\"heap\":%d,\"devs\":%d,\"wifi\":%d,\"ble\":%d}",
        (unsigned long)up_ms, heap, devs, wifi ? 1 : 0, ble ? 1 : 0);
}

void scanner_proto_stat(uint32_t up_ms, int heap, int devs, int wifi, int ble) {
    char buf[128];
    if (scanner_proto_stat_to_buf(buf, sizeof(buf), up_ms, heap, devs, wifi, ble) > 0) {
        printf("%s\n", buf);
    }
}
