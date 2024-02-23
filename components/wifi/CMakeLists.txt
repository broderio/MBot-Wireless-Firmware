idf_component_register(SRCS "src/wifi_ap.c" "src/espnow.c" "src/wifi_sta.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi)
