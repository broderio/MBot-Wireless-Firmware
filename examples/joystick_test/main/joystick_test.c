#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "joystick.h"

void app_main(void)
{
    joystick_config_t js_config = {
        .x_pin = 5,
        .y_pin = 4,
        .x_in_max = 916,
        .x_in_min = 22,
        .x_out_max = 1,
        .x_out_min = -1,
        .y_in_max = 907,
        .y_in_min = 0,
        .y_out_max = 1,
        .y_out_min = -1,
    };
    joystick_init(&js_config);
    float x, y;
    int x_raw, y_raw;
    while (1) {
        joystick_get_output(&x, &y);
        ESP_LOGI("JOYSTICK", "X: %0.3f, Y: %0.3f", x, y);
        joystick_get_raw(&x_raw, &y_raw);
        ESP_LOGI("JOYSTICK", "X: %d, Y: %d", x_raw, y_raw);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    joystick_deinit();
}