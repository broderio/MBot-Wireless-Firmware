idf_component_register(SRCS "joystick_test.c"
                    INCLUDE_DIRS "."
                    REQUIRES joystick buttons pairing led nvs_flash)