idf_component_register(SRCS "src/tcp_socket.c" "src/direct.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver common esp_wifi wifi)
