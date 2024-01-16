idf_component_register(SRCS "client.c"
                    INCLUDE_DIRS "."
                    REQUIRES camera client_messaging lidar)
