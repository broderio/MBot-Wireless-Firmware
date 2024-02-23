idf_component_register(SRCS "wifi_sta_test.c"
                    INCLUDE_DIRS "."
                    REQUIRES wifi sockets nvs_flash)
