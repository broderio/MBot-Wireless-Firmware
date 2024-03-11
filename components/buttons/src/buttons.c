#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "driver/gpio.h"

#include "buttons.h"

#define TAG "BUTTON_BASE"

/**< Defintions of private structures and functions */

typedef enum {
    BUTTON_READ_TYPE_POLL,
    BUTTON_READ_TYPE_ISR
} button_read_type_t;

/**
 * @brief Structure representing a button.
 */
struct button_t {
    uint32_t _held_threshold; /**< The threshold time in milliseconds for considering the button as held. */
    
    button_callback_t _pressed_callback; /**< Callback function to be called when the button is pressed. */
    void *_pressed_callback_arg; /**< Optional argument to be passed to the pressed callback function. */
    
    button_callback_t _pressed_for_callback; /**< Callback function to be called when the button is pressed for a specified duration. */
    void *_pressed_for_callback_arg; /**< Optional argument to be passed to the pressed for callback function. */
    
    uint8_t _pressed_callback_called; /**< Flag indicating whether the pressed callback function has been called. */
    uint8_t _released_callback_called; /**< Flag indicating whether the released callback function has been called. */
    uint8_t _pressed_for_callback_called; /**< Flag indicating whether the pressed for callback function has been called. */
    uint8_t _released_for_callback_called; /**< Flag indicating whether the released for callback function has been called. */
    
    uint8_t _pin; /**< The pin number to which the button is connected. */
    uint32_t _db_time; /**< The debounce time in milliseconds. */
    uint8_t _pu_enabled; /**< Flag indicating whether the internal pull-up resistor is enabled. */
    uint8_t _intr_enabled; /**< Flag indicating whether the interrupt is enabled. */

    uint8_t _active_low; /**< Flag indicating whether the button is active low. */
    uint8_t _current_state; /**< The current state of the button (pressed or released). */
    uint8_t _last_state; /**< The last state of the button (pressed or released). */
    uint8_t _changed; /**< Flag indicating whether the button state has changed. */
    uint32_t _time; /**< The current time in milliseconds. */
    uint32_t _last_change; /**< The time of the last state change in milliseconds. */
    uint8_t _was_held; /**< Flag indicating whether the button was held. */

    EventGroupHandle_t _event_group; /**< Event group for button events. */    
};

/**
 * @brief Reads the current state of the button pin.
 * @param button The button object.
 * @return The current state of the button pin (0 or 1).
 */
uint8_t _get_state(button_t *button) {
    uint8_t state = gpio_get_level(button->_pin);
    if (button->_active_low) {
        state = !state;
    }
    return state;
}

uint32_t _get_time_ms() {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void _button_isr_handler(void *arg) {
    button_t *button = (button_t *)arg;
    button_read(button);
}

/**< Definitions of public functions */

button_t *button_create(uint8_t pin) {
    button_t *button = (button_t *)malloc(sizeof(button_t));
    if (!button) {
        ESP_LOGE(TAG, "Failed to allocate memory for button object");
        return NULL;
    }
    memset(button, 0, sizeof(button_t));
    
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    
    button->_pin = pin;
    button->_db_time = 35;
    button->_held_threshold = 1000;
    button->_active_low = 1;
    button->_pu_enabled = 1;
    button->_event_group = xEventGroupCreate();
    button->_current_state = _get_state(button);
    ESP_LOGI(TAG, "Button created on pin %d with state %d", pin, button->_current_state);
    button->_time = _get_time_ms();
    button->_last_state = button->_current_state;
    button->_last_change = button->_time;


    return button;
}

void button_free(button_t *button) {
    if (button) {
        vEventGroupDelete(button->_event_group);
        gpio_isr_handler_remove(button->_pin);
        gpio_reset_pin(button->_pin);
        free(button);
    }
}

void button_interrupt_enable(button_t *button) {
    gpio_install_isr_service(0);
    gpio_intr_enable(button->_pin);
    gpio_set_intr_type(button->_pin, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(button->_pin, _button_isr_handler, button);
    button->_intr_enabled = 1;
}

void button_interrupt_disable(button_t *button) {
    gpio_intr_disable(button->_pin);
    gpio_isr_handler_remove(button->_pin);
    button->_intr_enabled = 0;
}

void button_set_active_low(button_t *button) {
    button->_active_low = 1;
}

void button_set_active_high(button_t *button) {
    button->_active_low = 0;
}

uint8_t button_get_active_level(button_t *button) {
    return button->_active_low;
}

void button_set_debounce_time(button_t *button, uint32_t time) {
    button->_db_time = time;
}

uint32_t button_get_debounce_time(button_t *button) {
    return button->_db_time;
}

void button_set_held_threshold(button_t *button, uint32_t time) {
    button->_held_threshold = time;
}

uint32_t button_get_held_threshold(button_t *button) {
    return button->_held_threshold;
}

void button_set_pull_up(button_t *button, uint8_t enabled) {
    button->_pu_enabled = enabled;
    gpio_set_pull_mode(button->_pin, enabled ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
}

uint8_t button_get_pull_up(button_t *button) {
    return button->_pu_enabled;
}

uint8_t button_read(button_t* button) {
    uint32_t read_started_ms = _get_time_ms();
    uint8_t state = _get_state(button);

    uint8_t within_db_time = read_started_ms - button->_last_change < button->_db_time;
    if (within_db_time) {
        // Debounce time has not ellapsed
        button->_changed = 0;
    }
    else {
        // Debounce time has ellapsed
        button->_last_state = button->_current_state;
        button->_current_state = state;
        button->_changed = button->_current_state != button->_last_state;

        if (button->_changed) {
            if (read_started_ms - button->_last_change >= button->_held_threshold) {
                button->_was_held = 1;
            }
            else {
                button->_was_held = 0;
            }
            button->_last_change = read_started_ms;
        }
    }

    if (button_was_released(button)) {
        if (xPortInIsrContext()) {
            xEventGroupSetBitsFromISR(button->_event_group, BIT0, NULL);
        }
        else {
            xEventGroupSetBits(button->_event_group, BIT0);
        }

        if (!button->_was_held && button->_pressed_callback && !button->_pressed_callback_called) {
            button->_pressed_callback_called = 1;
            button->_pressed_callback(button->_pressed_callback_arg);
            button->_pressed_callback_called = 0;
        }
        else if (button->_was_held && button->_pressed_for_callback && !button->_pressed_for_callback_called) {
            button->_pressed_for_callback_called = 1;
            button->_pressed_for_callback(button->_pressed_for_callback_arg);
            button->_pressed_for_callback_called = 0;
        }
        button->_was_held = 0;
    }
    else if (button_was_pressed(button)) {
        if (xPortInIsrContext()) {
            xEventGroupSetBitsFromISR(button->_event_group, BIT1, NULL);
        }
        else {
            xEventGroupSetBits(button->_event_group, BIT1);
        }
    }

    button->_time = read_started_ms;

    return button->_current_state;
}

void button_on_press(button_t *button, button_callback_t callback, void *arg) {
    button->_pressed_callback = callback;
    button->_pressed_callback_arg = arg;
}

void button_on_pressed_for(button_t *button, button_callback_t callback, void *arg) {
    button->_pressed_for_callback = callback;
    button->_pressed_for_callback_arg = arg;
}

uint8_t button_is_pressed(button_t *button) {
    return button->_current_state;
}

uint8_t button_was_pressed(button_t *button) {
    return button->_current_state && button->_changed;
}

uint8_t button_was_pressed_for(button_t *button, uint32_t duration) {
    return button->_current_state && (button->_time - button->_last_change >= duration);
}

uint8_t button_is_released(button_t *button) {
    return !button->_current_state;
}

uint8_t button_was_released(button_t *button) {
    return !button->_current_state && button->_changed;
}

uint8_t button_was_released_for(button_t *button, uint32_t duration) {
    return !button->_current_state && (button->_time - button->_last_change >= duration);
}

void button_wait_for_press(button_t *button) {
    if (button->_intr_enabled) {
        xEventGroupClearBits(button->_event_group, BIT1); 
        xEventGroupWaitBits(button->_event_group, BIT1, pdTRUE, pdTRUE, portMAX_DELAY);
    }
    else {
        while (!button_is_pressed(button)) {
            button_read(button);
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
}

void button_wait_for_release(button_t *button) {
    if (button->_intr_enabled) {
        xEventGroupClearBits(button->_event_group, BIT0); 
        xEventGroupWaitBits(button->_event_group, BIT0, pdTRUE, pdTRUE, portMAX_DELAY);
    }
    else {
        while (!button_is_released(button)) {
            button_read(button);
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
}