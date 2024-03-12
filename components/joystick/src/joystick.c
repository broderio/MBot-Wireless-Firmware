#include "string.h"
#include "math.h"

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#include "nvs_flash.h"
#include "esp_log.h"

#include "joystick.h"

typedef struct joystick_t {
    uint8_t x_pin;
    uint8_t y_pin;
    int32_t x_raw;
    int32_t y_raw;
    float _deadband;
    adc_oneshot_unit_handle_t x_adc_handle;
    adc_oneshot_unit_handle_t y_adc_handle;
    adc_channel_t x_channel;
    adc_channel_t y_channel;
    adc_unit_t x_unit;
    adc_unit_t y_unit;
    joystick_config_t cfg;
    uint8_t _is_calibrated;
} joystick_t;

joystick_t *joystick_create(uint8_t x_pin, uint8_t y_pin) {
    joystick_t *js = (joystick_t*)malloc(sizeof(joystick_t));
    if (js == NULL) {
        ESP_LOGE("JOYSTICK", "Failed to allocate memory for joystick configuration");
        return NULL;
    }
    memset(js, 0, sizeof(joystick_t));
    js->x_pin = x_pin;
    js->y_pin = y_pin;
    js->_deadband = 0.05;

    adc_oneshot_io_to_channel(js->x_pin, &js->x_unit, &js->x_channel);
    adc_oneshot_unit_init_cfg_t unit_cfg ={
        .unit_id = js->x_unit,
    };
    adc_oneshot_new_unit(&unit_cfg, &js->x_adc_handle);

    adc_oneshot_io_to_channel(js->y_pin, &js->y_unit, &js->y_channel);
    if (js->y_unit != js->x_unit) {
        unit_cfg.unit_id = js->y_unit;
        adc_oneshot_new_unit(&unit_cfg, &js->y_adc_handle);
    }
    else {
        js->y_adc_handle = js->x_adc_handle;
    }

    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(js->x_adc_handle, js->x_channel, &adc_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(js->y_adc_handle, js->y_channel, &adc_config));

    joystick_config_t default_cfg = DEFAULT_JOYSTICK_CONFIG;
    js->_is_calibrated = 0;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGW("JOYSTICK", "Error (%s) opening NVS handle!", esp_err_to_name(err));
        memcpy(&js->cfg, &default_cfg, sizeof(joystick_config_t));
        return js;
    }

    size_t len = sizeof(joystick_config_t);
    err = nvs_get_blob(nvs, "js_cfg", &js->cfg, &len);
    switch (err) { 
        case ESP_OK:
            ESP_LOGI("JOYSTICK", "Found joystick configuration");
            js->_is_calibrated = 1;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW("JOYSTICK", "No configuration found, using default");
            memcpy(&js->cfg, &default_cfg, sizeof(joystick_config_t));
            break;
        default:
            ESP_LOGE("JOYSTICK", "Error (%s) reading!", esp_err_to_name(err));
            memcpy(&js->cfg, &default_cfg, sizeof(joystick_config_t));
            break;
    }
    nvs_close(nvs);
    return js;
}

void joystick_free(joystick_t *js) {
    adc_oneshot_del_unit(js->x_adc_handle);
    if (js->y_adc_handle != js->x_adc_handle) {
        adc_oneshot_del_unit(js->y_adc_handle);
    }
    free(js);
    js = NULL;
}

void joystick_read(joystick_t *js) {
    adc_oneshot_read(js->x_adc_handle, js->x_channel, (int*)&js->x_raw);
    adc_oneshot_read(js->y_adc_handle, js->y_channel, (int*)&js->y_raw);
}

void joystick_set_deadband(joystick_t *js, float deadband) {
    js->_deadband = deadband;
}

int32_t joystick_get_x_raw(joystick_t *js) {
    return js->x_raw;
}

int32_t joystick_get_y_raw(joystick_t *js) {
    return js->y_raw;
}

float joystick_get_x(joystick_t *js) {
    float out;
    if (js->x_raw > js->cfg.x_rest) {
        out = (float)(js->x_raw - js->cfg.x_rest) / (float)(js->cfg.x_max - js->cfg.x_rest);
    }
    else {
        out = (float)(js->x_raw - js->cfg.x_rest) / (float)(js->cfg.x_rest - js->cfg.x_min);
    }

    if (out > 1.0) {
        out = 1.0;
    }
    else if (out < -1.0) {
        out = -1.0;
    }

    if (fabs(out) < js->_deadband) {
        out = 0.0;
    }

    return out;
}

float joystick_get_y(joystick_t *js) {
    float out;
    if (js->y_raw > js->cfg.y_rest) {
        out = (float)(js->y_raw - js->cfg.y_rest) / (float)(js->cfg.y_max - js->cfg.y_rest);
    }
    else {
        out = (float)(js->y_raw - js->cfg.y_rest) / (float)(js->cfg.y_rest - js->cfg.y_min);
    }
    
    if (out > 1.0) {
        out = 1.0;
    }
    else if (out < -1.0) {
        out = -1.0;
    }

    if (fabs(out) < js->_deadband) {
        out = 0.0;
    }

    return out;
}

uint8_t joystick_is_calibrated(joystick_t *js) {
    return js->_is_calibrated;
}

joystick_config_t joystick_get_config(joystick_t *js) {
    return js->cfg;
}

void joystick_write_cfg(joystick_config_t *cfg) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE("JOYSTICK", "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(nvs, "js_cfg", cfg, sizeof(joystick_config_t));
    if (err != ESP_OK) {
        ESP_LOGE("JOYSTICK", "Error (%s) writing!", esp_err_to_name(err));
    }

    err = nvs_commit(nvs);
    if (err != ESP_OK) {
        ESP_LOGE("JOYSTICK", "Error (%s) committing!", esp_err_to_name(err));
    }

    nvs_close(nvs);
}