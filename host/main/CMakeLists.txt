idf_component_register(SRCS "src/host.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_timer nvs_flash network buttons led joystick usb_device serializer wifi)
