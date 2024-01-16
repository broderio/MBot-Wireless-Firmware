idf_component_register(SRCS "host_messaging.c"
                    INCLUDE_DIRS "include"
                    REQUIRES usb_device wifi serializer)
