idf_component_register(SRCS "src/pairing.c" "src/wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi usb_device nvs_flash)