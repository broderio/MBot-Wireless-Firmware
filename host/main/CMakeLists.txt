idf_component_register(SRCS "src/host.c" "src/utils.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_timer nvs_flash sockets buttons led joystick usb_device serializer)
