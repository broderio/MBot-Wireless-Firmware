#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#include "joystick.h"

int x_center = 0;
int y_center = 0;
joystick_config_t config = {0};

void _joystick_calibrate() {
    int adc_val = 0, x_tmp = 0, y_tmp = 0, n = 1000;
    for (size_t i = 0; i < n; ++i) {
        // adc_oneshot_get_calibrated_result(adc1_handle, JS_X_cali, ADC_CHANNEL_4, &adc_val);
        x_tmp += adc_val;
        // adc_oneshot_get_calibrated_result(adc1_handle, JS_Y_cali, ADC_CHANNEL_3, &adc_val);
        y_tmp += adc_val;
    }
    x_center = x_tmp / n;
    y_center = y_tmp / n;
}

void joystick_init(joystick_config_t* config) {
    // TODO: Implement
}

void joystick_deinit() {
    // TODO: Implement
}

void joystick_get_output(int *x, int *y) {
    // TODO: Implement
}

void joystick_get_raw_position(int *x, int *y) {
    // TODO: Implement
}