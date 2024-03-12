idf_component_register(SRCS "src/joystick.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_adc nvs_flash)
