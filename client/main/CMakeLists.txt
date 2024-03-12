idf_component_register(SRCS "src/client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_timer nvs_flash uart lidar camera sockets buttons serializer pairing usb_device)
