#include "scanner_common.h"
#ifdef CONFIG_IDF_TARGET
#include "esp_timer.h"
#endif

const char *cat_name(cat_t c) {
    switch (c) {
        case CAT_CAMERA:      return "camera";
        case CAT_PHONE:       return "phone";
        case CAT_PRINTER:     return "printer";
        case CAT_HOTSPOT:     return "hotspot";
        case CAT_LED:         return "led";
        case CAT_SIGNAGE:     return "signage";
        case CAT_FACADE:      return "facade";
        case CAT_SCREEN:      return "screen";
        case CAT_TRAFFIC:     return "traffic";
        case CAT_STREETLIGHT: return "streetlight";
        case CAT_PARKING:     return "parking";
        case CAT_ROUTER:      return "router";
        case CAT_IOT:         return "iot";
        case CAT_BLE:         return "ble";
        default:              return "unknown";
    }
}

const char *src_name(src_t s) {
    switch (s) {
        case SRC_WIFI: return "wifi";
        case SRC_BLE:  return "ble";
        default:       return "none";
    }
}

const char *auth_name(auth_t a) {
    switch (a) {
        case AUTH_OPEN:      return "OPEN";
        case AUTH_WEP:       return "WEP";
        case AUTH_WPA:       return "WPA";
        case AUTH_WPA2:      return "WPA2";
        case AUTH_WPA3:      return "WPA3";
        case AUTH_WPA2_WPA3: return "WPA2/3";
        default:             return "?";
    }
}

uint32_t scanner_now_ms(void) {
#ifdef CONFIG_IDF_TARGET
    return (uint32_t)(esp_timer_get_time() / 1000);
#else
    return 0;
#endif
}

int ble_adv_get_name(const uint8_t *data, int len, char *out, int out_sz) {
    if (!out || out_sz <= 0) return 0;
    out[0] = '\0';
    if (!data || len <= 0) return 0;
    int i = 0;
    while (i < len) {
        int ad_len = data[i];               // length byte: type + payload
        if (ad_len == 0) break;             // malformed (would not advance)
        // The structure spans [i .. i+ad_len]; it must fit in the buffer.
        if (i + ad_len >= len) break;       // truncated structure
        int type = data[i + 1];
        if (type == 0x08 || type == 0x09) { // short / complete local name
            int n = ad_len - 1;             // payload bytes
            if (n > out_sz - 1) n = out_sz - 1;
            if (n <= 0) return 0;
            memcpy(out, data + i + 2, (size_t)n);
            out[n] = '\0';
            return n;
        }
        i += ad_len + 1;                    // next structure (after len byte)
    }
    return 0;
}
