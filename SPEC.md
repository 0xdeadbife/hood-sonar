# Hood Sonar

Firmware for the ESP32-C3 (no PSRAM, 400 KB SRAM, 4 MB flash) that continuously
scans Wi-Fi + BLE, classifies nearby devices with a focus on urban surfaces /
building facades, and emits compact JSON over USB-Serial/JTAG (WebSerial). A
single-file web dashboard (dark/sleek) renders the device list. Framework:
**ESP-IDF v5.3**.

## Scope
In:
- Wi-Fi active scan (async) + BLE (NimBLE) continuous scan, coexistence.
- Classification: OUI (vendor) + keywords in SSID/name + BLE service UUIDs.
- Categories: camera, phone, printer, hotspot, led, signage, facade, screen,
  traffic, streetlight, parking, router, iot, ble, unknown.
- Device table (max 128, dedup by MAC+source), JSON snapshot every ~2 s,
  board-health stat every ~10 s.
- Dashboard: dark/sleek, inline SVG icons (no CDN), signal bars, filters, sort,
  search, stats, localStorage.
Out:
- No HTTP server on the board (dashboard = local file + WebSerial over USB).
- No dual OTA (single app, default partition table).
- No Classic BT / 802.15.4 (C3 limitation).

## Files
- firmware/CMakeLists.txt, firmware/sdkconfig.defaults
- firmware/main/{scanner_common.h, scanner_oui.{h,c}, scanner_classify.{h,c},
  scanner_table.{h,c}, scanner_wifi.{h,c}, scanner_ble.{h,c},
  scanner_proto.{h,c}, scanner_main.c, CMakeLists.txt}
- web/dashboard.html, web/index.html (Pages redirect)
- README.md (build/flash for /dev/ttyACM0)

## Acceptance (each one verifiable with a command)
A1. `cd firmware && idf.py set-target esp32c3 && idf.py build` → "Project build complete".
A2. Pure-C logic: gcc test of classify + table passes.
A3. OUI table: ≥150 entries, 5 known OUIs map to the expected category.
A4. `check_html.py web/dashboard.html` → RESULT: OK.
A5. `check_html.py web/dashboard.html --run "..."` → WebSerial connects + renders mock snapshot.
A6. README has exact build/flash commands for /dev/ttyACM0.
