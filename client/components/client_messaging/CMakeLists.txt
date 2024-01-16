idf_component_register(SRCS "client_messaging.c"
                    INCLUDE_DIRS "include"
                    REQUIRES spi_follower wifi serializer)
