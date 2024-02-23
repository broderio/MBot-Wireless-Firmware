idf_component_register(SRCS "src/client.c" "src/utils.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi nvs_flash sockets buttons fram joystick)
