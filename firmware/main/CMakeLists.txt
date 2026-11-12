idf_component_register(
    SRCS "scanner_common.c"
         "scanner_oui.c"
         "scanner_classify.c"
         "scanner_table.c"
         "scanner_wifi.c"
         "scanner_ble.c"
         "scanner_proto.c"
         "scanner_main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_event bt driver freertos
)
