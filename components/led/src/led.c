#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "led.h"

#define TAG "LED"

/**
 * @brief Structure representing an LED.
*/
struct led_t {
    uint8_t _pin; /**< The pin number to which the LED is connected. */
    uint8_t _state; /**< The current state of the LED (on or off). */
    uint32_t _period; /**< The period of the blinking LED. */
    uint8_t _is_blinking; /**< Flag indicating whether the LED is blinking. */
    EventGroupHandle_t _event_group; /**< Event group for the LED. */
};

/**
 * @brief Private function to set state of LED
*/
void _led_set(led_t *led, uint8_t state) {
    gpio_set_level(led->_pin, state);
    led->_state = state;
}

/**
 * @brief Task function for blinking an LED.
 *
 * This task function is responsible for blinking an LED at a specified period.
 * It toggles the LED state and delays for the specified period until it is
 * interrupted by an event. Once the event is received, the task stops blinking
 * the LED and turns it off.
 *
 * @param arg Pointer to the LED object.
 */
void _led_blink_task(void *arg) {
    led_t *led = (led_t *)arg;
    TickType_t xLastWakeTime;
    while (true) {
        xLastWakeTime = xTaskGetTickCount();
        if (xEventGroupGetBits(led->_event_group) & BIT0) {
            xEventGroupClearBits(led->_event_group, BIT0);
            break;
        }
        _led_set(led, !led->_state);
        xTaskDelayUntil(&xLastWakeTime, led->_period / 2 / portTICK_PERIOD_MS);
    }
    xEventGroupSetBits(led->_event_group, BIT1);
    _led_set(led, 0);
    vTaskDelete(NULL);
}

/*< Public function definitions */

led_t *led_create(uint8_t pin) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pin %d", pin);
        return NULL;
    }
    gpio_set_level(pin, 0);

    led_t *led = (led_t *)malloc(sizeof(led_t));
    if (led == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for LED");
        return NULL;
    }
    led->_pin = pin;
    led->_state = 0;
    led->_period = 0;
    led->_is_blinking = 0;
    led->_event_group = xEventGroupCreate();
    return led;
}

void led_free(led_t *led) {
    if (led->_is_blinking) {
        led_stop_blink(led);
    }
    
    free(led);
}

void led_on(led_t *led) {
    led_set(led, 1);
}

uint8_t led_is_on(led_t *led) {
    return led_read(led) == 1;
}

void led_off(led_t *led) {
    led_set(led, 0);
}

uint8_t led_is_off(led_t *led) {
    return led_read(led) == 0;
}

void led_set(led_t *led, uint8_t state) {
    if (state == led->_state) {
        return;
    }
    if (led->_is_blinking) {
        led_stop_blink(led);
    }
    _led_set(led, state);
}

uint8_t led_read(led_t *led) {
    return led->_state;
}

void led_toggle(led_t *led) {
    led_set(led, !led->_state);
}

void led_blink(led_t *led, uint32_t period) {
    if (led->_is_blinking) {
        ESP_LOGW(TAG, "LED is already blinking");
        return;
    }
    led->_period = period;
    xTaskCreate(_led_blink_task, "led_blink_task", 2048, (void *)led, 3, NULL);
    led->_is_blinking = 1;
}

void led_stop_blink(led_t *led) {
    if (!led->_is_blinking) {
        ESP_LOGW(TAG, "LED is not blinking");
        return;
    }
    xEventGroupSetBits(led->_event_group, BIT0);
    xEventGroupWaitBits(led->_event_group, BIT1, pdTRUE, pdTRUE, portMAX_DELAY);
    led->_is_blinking = 0;
}