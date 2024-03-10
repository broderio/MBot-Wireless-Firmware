#include "string.h"

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#include "esp_log.h"

#include "joystick.h"

int x_center = 0;
int y_center = 0;
joystick_config_t *js_config = NULL;

adc_oneshot_unit_handle_t x_adc_handle;
adc_oneshot_unit_handle_t y_adc_handle;

adc_channel_t x_channel;
adc_channel_t y_channel;

adc_cali_handle_t js_y_cali_handle;
adc_cali_handle_t js_x_cali_handle;

void _joystick_calibrate(int *x, int *y) {
    int adc_val = 0, x_tmp = 0, y_tmp = 0, n = 1000;
    for (size_t i = 0; i < n; ++i) {
        adc_oneshot_get_calibrated_result(x_adc_handle, js_x_cali_handle, x_channel, &adc_val);
        x_tmp += adc_val;
        adc_oneshot_get_calibrated_result(y_adc_handle, js_y_cali_handle, y_channel, &adc_val);
        y_tmp += adc_val;
    }
    *x = x_tmp / n;
    *y = y_tmp / n;
}

void joystick_init(joystick_config_t* cfg) {
    js_config = (joystick_config_t*)malloc(sizeof(joystick_config_t));
    if (js_config == NULL) {
        ESP_LOGE("JOYSTICK", "Failed to allocate memory for joystick configuration");
        return;
    }
    memcpy(js_config, cfg, sizeof(joystick_config_t));

    adc_unit_t unit;
    adc_oneshot_io_to_channel(js_config->x_pin, &unit, &x_channel);
    adc_oneshot_unit_init_cfg_t unit_cfg ={
        .unit_id = unit
    };
    adc_oneshot_new_unit(&unit_cfg, &x_adc_handle);

    adc_oneshot_io_to_channel(js_config->y_pin, &unit, &y_channel);
    if (unit != unit_cfg.unit_id) {
        unit_cfg.unit_id = unit;
        adc_oneshot_new_unit(&unit_cfg, &y_adc_handle);
    }
    else {
        y_adc_handle = x_adc_handle;
    }

    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(x_adc_handle, x_channel, &adc_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(y_adc_handle, y_channel, &adc_config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = ADC_CHANNEL_3,
        .atten = ADC_ATTEN_DB_0,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config, &js_y_cali_handle);

    cali_config.chan = ADC_CHANNEL_4;
    adc_cali_create_scheme_curve_fitting(&cali_config, &js_x_cali_handle);

    _joystick_calibrate(&x_center, &y_center);
}

void joystick_deinit() {
    free(js_config);
    js_config = NULL;
    adc_oneshot_del_unit(x_adc_handle);
    adc_oneshot_del_unit(y_adc_handle);
}

void joystick_get_output(float *x, float *y) {
    int x_raw = 0, y_raw = 0;
    adc_oneshot_get_calibrated_result(x_adc_handle, js_x_cali_handle, x_channel, &x_raw);
    adc_oneshot_get_calibrated_result(y_adc_handle, js_y_cali_handle, y_channel, &y_raw);
    *x = ((float)(x_raw - x_center) * (js_config->x_out_max - js_config->x_out_min)) / (js_config->x_in_max - js_config->x_in_min);
    *y = ((float)(y_raw - y_center) * (js_config->y_out_max - js_config->y_out_min)) / (js_config->y_in_max - js_config->y_in_min);
    if (*x > js_config->x_out_max) {
        *x = js_config->x_out_max;
    }
    else if (*x < js_config->x_out_min) {
        *x = js_config->x_out_min;
    }

    if (*y > js_config->y_out_max) {
        *y = js_config->y_out_max;
    }
    else if (*y < js_config->y_out_min) {
        *y = js_config->y_out_min;
    }
}

void joystick_get_raw(int *x, int *y) {
    adc_oneshot_get_calibrated_result(x_adc_handle, js_x_cali_handle, x_channel, x);
    adc_oneshot_get_calibrated_result(y_adc_handle, js_y_cali_handle, y_channel, y);
}