idf_component_register(SRCS "network_test.c"
                    INCLUDE_DIRS "."
                    REQUIRES wifi sockets nvs_flash)
