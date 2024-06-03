#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "led.h"
#include "buttons.h"
#include "joystick.h"

#define JS_X_PIN 5
#define JS_Y_PIN 4
#define BTN_PIN 17
#define LED_PIN 15

#define TAG "CONFIGURATION"

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    usb_device_init();

    button_t *btn = button_create(BTN_PIN);
    if (btn == NULL) {
        ESP_LOGE("BUTTON", "Failed to create button");
        return;
    }
    button_interrupt_enable(btn);

    led_t *led = led_create(LED_PIN);
    if (led == NULL) {
        ESP_LOGE("LED", "Failed to create led");
        return;
    }
    led_on(led);

    joystick_t *js = joystick_create(JS_X_PIN, JS_Y_PIN);
    if (js == NULL) {
        ESP_LOGE("JOYSTICK", "Failed to create joystick");
        return;
    }
    
    joystick_config_t cfg = joystick_get_config(js, DEFAULT_JS_CFG_NAME);
    if (joystick_is_calibrated(js)) {
        ESP_LOGI(TAG, "Joystick is already calibrated.");
        ESP_LOGI(TAG, "x_max: %ld, x_min: %ld, x_rest: %ld, y_max: %ld, y_min: %ld, y_rest: %ld", cfg.x_max, cfg.x_min, cfg.x_rest, cfg.y_max, cfg.y_min, cfg.y_rest);
        ESP_LOGI(TAG, "Press the button to recalibrate the joystick");
    }
    else {
        ESP_LOGI(TAG, "Joystick is not calibrated. Press the button to calibrate the joystick");
    }

    // Initialize max and min values with values from the set
    joystick_config_t js_cfg;
    joystick_read(js);
    js_cfg.x_rest = js_cfg.x_max = js_cfg.x_min = joystick_get_x_raw(js);
    js_cfg.y_rest = js_cfg.y_max = js_cfg.y_min = joystick_get_y_raw(js);

    // Wait for button press to start finding center
    button_wait_for_press(btn);
    button_wait_for_release(btn);
    int32_t x_rest = 0, y_rest = 0, count = 0;
    while (!button_is_pressed(btn)) {
        joystick_read(js);
        x_rest += joystick_get_x_raw(js);
        y_rest += joystick_get_y_raw(js);
        count++;
        ESP_LOGI(TAG, "x_rest: %ld, y_rest: %ld", x_rest / count, y_rest / count);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    js_cfg.x_rest = x_rest / count;
    js_cfg.y_rest = y_rest / count;

    // Wait for button press to start finding min/max
    button_wait_for_press(btn);
    button_wait_for_release(btn);
    led_blink(led, 100);
    while (!button_is_pressed(btn)) {
        joystick_read(js);
        int32_t x_raw = joystick_get_x_raw(js);
        int32_t y_raw = joystick_get_y_raw(js);
        if (x_raw > js_cfg.x_max) {
            js_cfg.x_max = x_raw;
        }
        if (x_raw < js_cfg.x_min) {
            js_cfg.x_min = x_raw;
        }
        if (y_raw > js_cfg.y_max) {
            js_cfg.y_max = y_raw;
        }
        if (y_raw < js_cfg.y_min) {
            js_cfg.y_min = y_raw;
        }
        ESP_LOGI(TAG, "x_max: %ld, x_min: %ld, y_max: %ld, y_min: %ld", js_cfg.x_max, js_cfg.x_min, js_cfg.y_max, js_cfg.y_min);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    joystick_write_cfg(&js_cfg, DEFAULT_JS_CFG_NAME);
    ESP_LOGI(TAG, "Calibration complete! Press the button to see the joystick values");

    // Wait for button press to start reading joystick values
    button_wait_for_press(btn);
    button_wait_for_release(btn);
    while (!button_is_pressed(btn)) {
        joystick_read(js);
        ESP_LOGI(TAG, "x: %f, y: %f", joystick_get_x(js), joystick_get_y(js));
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    led_stop_blink(led);
    joystick_free(js);
}