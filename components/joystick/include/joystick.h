#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#ifndef JOYSTICK_H
#define JOYSTICK_H

typedef struct joystick_config_t {
    int x_in_max; // Voltage
    int x_in_min;
    int x_out_max; // Output Unit
    int x_out_min;
    int y_in_max;
    int y_in_min;
    int y_out_max;
    int y_out_min;
} joystick_config_t;

void joystick_init(joystick_config_t* config);
void joystick_deinit();
void joystick_get_output(int *x, int *y);
void joystick_get_raw_position(int *x, int *y);

#endif