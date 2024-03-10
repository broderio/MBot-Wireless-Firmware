#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "driver/gpio.h"

#include "buttons.h"

#define BUTTON_RELEASE_BIT BIT0
#define BUTTON_PRESS_BIT BIT1

#pragma pack(push, 1)

typedef struct button_isr_args_t button_isr_args_t;

typedef struct button_isr_args_t {
    QueueHandle_t *queue;
    EventGroupHandle_t *event_group;
    button_config_t *cfg;
    uint8_t pin;
    button_isr_args_t *next;
} button_isr_args_t;

#pragma pack(pop)

static button_config_t cfgs[4];
static QueueHandle_t queues[4];
static button_isr_args_t *_buttons[4];
static int num_button_configs = 0;

static void buttons_isr(void* arg) {
    static int pair_down = 0;
    button_isr_args_t *button_args = (button_isr_args_t*) arg;
    button_config_t *cfg = button_args->cfg;
    uint8_t pin = button_args->pin;
    QueueHandle_t *queue = button_args->queue;
    xQueueSendFromISR(*queue, &pin, NULL);

    // Set the event bits
    if (pair_down) {
        xEventGroupSetBits(*button_args->event_group, BUTTON_RELEASE_BIT);
        pair_down = 0;
    }
    else {
        xEventGroupSetBits(*button_args->event_group, BUTTON_PRESS_BIT);
        pair_down = 1;
    }

    // Call the callback
    if (cfg->callback) {
        cfg->callback(cfg->args);
    }
}

int _alloc_buttons(button_isr_args_t** args, button_config_t* cfg, QueueHandle_t* queue, uint8_t num_buttons) {
    button_isr_args_t *args_ptr = NULL;
    button_isr_args_t *prev_args_ptr = NULL;
    for (int i = 0; i < num_buttons; i++) {
        args_ptr = (button_isr_args_t*) malloc(sizeof(button_isr_args_t));
        if (args_ptr == NULL) {
            ESP_LOGE("BUTTONS", "Failed to allocate memory for button ISR args");
            return -1;
        }

        args_ptr->event_group = malloc(sizeof(EventGroupHandle_t));
        if (args_ptr->event_group == NULL) {
            ESP_LOGE("BUTTONS", "Failed to allocate memory for button event group");
            free(args_ptr);
            return -1;
        }

        *args_ptr->event_group = xEventGroupCreate();
        args_ptr->cfg = cfg;
        args_ptr->queue = queue;
        args_ptr->pin = -1;
        args_ptr->next = NULL;

        if (prev_args_ptr != NULL) {
            prev_args_ptr->next = args_ptr;
        } else {
            *args = args_ptr;  // Set the head of the list
        }

        prev_args_ptr = args_ptr;
    }
    return 0;
}

int _dealloc_buttons(button_isr_args_t** args) {
    button_isr_args_t *args_ptr = *args;
    while (args_ptr != NULL) {
        button_isr_args_t *next = args_ptr->next;
        vEventGroupDelete(*args_ptr->event_group);
        free(args_ptr->event_group);
        free(args_ptr);
        args_ptr = next;
    }
    return 0;
}

int buttons_init(button_config_t *cfg) {
    // Check if the maximum number of button configs has been reached
    if (num_button_configs >= 4) {
        ESP_LOGE("BUTTONS", "Maximum number of button configs reached");
        return -1;
    }

    // Find the next available slot for the button
    int next_available = -1;
    for (int i = 0; i < 4; i++) {
        if (_buttons[i] == NULL) {
            next_available = i;
            break;
        }
    }
    if (next_available == -1) {
        ESP_LOGE("BUTTONS", "No available slots for button");
        return -1;
    }

    // Check the validity of the button configuration
    assert(cfg->pin_bit_mask != 0);
    assert(cfg->double_press_time_ms > 0);
    assert(cfg->long_press_time_ms > 0);
    assert(cfg->hold_time_ms > 0 && cfg->hold_time_ms > cfg->long_press_time_ms);

    // Count the number of buttons in the configuration
    uint8_t num_buttons = 0;
    for (int i = 0; i < sizeof(cfg->pin_bit_mask) * 8; ++i) 
    {
        if (cfg->pin_bit_mask & (1 << i)) 
        {
            num_buttons++;
        }
    }
    
    // Allocate memory for the button configuration and ISR arguments
    memcpy(&cfgs[next_available], cfg, sizeof(button_config_t));
    queues[next_available] = xQueueCreate(10, sizeof(uint8_t));
    int ret = _alloc_buttons(&_buttons[next_available], 
                             &cfgs[next_available], 
                             &queues[next_available], 
                             num_buttons);
    if (ret != 0) {
        ESP_LOGE("BUTTONS", "Failed to allocate memory for button ISR args");
        return -1;
    }
    
    // Configure the GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = cfg->pin_bit_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);
    
    num_button_configs++;

    // Add the ISR for each button
    button_isr_args_t *args = _buttons[next_available];
    ESP_LOGI("BUTTONS", "pin_bit_mask: %llu", cfg->pin_bit_mask);
    for (int i = 0; i < 64; ++i) 
    {
        if (cfg->pin_bit_mask & ((uint64_t)1 << i)) 
        {
            args->pin = i;
            ESP_LOGI("BUTTONS", "Adding ISR for pin %d", i);
            esp_err_t err = gpio_isr_handler_add(i, buttons_isr, (void*)args);
            if (err == ESP_ERR_INVALID_STATE) 
            {
                gpio_install_isr_service(0);
                err = gpio_isr_handler_add(i, buttons_isr, (void*)args);
            }
            if (err != ESP_OK) 
            {
                ESP_LOGE("BUTTONS", "Failed to add ISR for pair button. error: %s", esp_err_to_name(err));
                buttons_deinit(next_available);
                return -1;
            }
            args = args->next;
            if (args == NULL) 
            {
                break;
            }
        }
    }

    return next_available;
}

void buttons_deinit(int buttons) {
    if (buttons < 0 || buttons >= num_button_configs) {
        ESP_LOGE("BUTTONS", "Invalid button index for deinitialization");
        return;
    }

    if (_buttons[buttons] == NULL) {
        ESP_LOGW("BUTTONS", "Button already deinitialized");
        return;
    }

    _dealloc_buttons(&_buttons[buttons]);
    num_button_configs--;
}

typedef enum {
    FIRST_EVENT,
    SECOND_EVENT,
    PRESS_STATE
} BUTTON_STATE;

void buttons_wait_for_event(int buttons, button_event_t *event, TickType_t ticks_to_wait)
{
    // Clear the queue and event group bits
    xQueueReset(queues[buttons]);
    button_isr_args_t *args = _buttons[buttons];
    while (args != NULL) {
        xEventGroupClearBits(*args->event_group, BUTTON_RELEASE_BIT | BUTTON_PRESS_BIT);
        args = args->next;
    }

    // Wait for the button press
    event->pin = -1;
    int ret = xQueueReceive(queues[buttons], &event->pin, ticks_to_wait);
    if (ret != pdTRUE) {
        event->type = NO_PRESS;
        return;
    }

    // Get the pointer to the button that was pressed
    args = _buttons[buttons];
    while (args != NULL) {
        if (args->pin == event->pin) {
            break;
        }
        args = args->next;
    }
    if (args == NULL) {
        ESP_LOGE("BUTTONS", "Failed to find button ISR args for pin %d", event->pin);
        event->type = NO_PRESS;
        return;
    }
    
    EventGroupHandle_t *event_group = args->event_group;
    button_config_t *cfg = args->cfg;
    TickType_t dt = 0;

    // Event handling state machine
    BUTTON_STATE state = FIRST_EVENT;
    EventBits_t bits;
    int press_count = 0;
    TickType_t ticks_waited = ticks_to_wait;
    while (true) {
        switch (state) {
            case FIRST_EVENT:
                bits = xEventGroupWaitBits(*event_group,
                        BUTTON_PRESS_BIT | BUTTON_RELEASE_BIT,
                        pdFALSE,
                        pdFALSE,
                        ticks_waited);
                // If we receive a press down event, go to the next state
                if (bits & BUTTON_PRESS_BIT) {
                    dt = xTaskGetTickCount();
                    ESP_LOGI("BUTTONS", "Pair button pressed!");
                    xEventGroupClearBits(*event_group, BUTTON_PRESS_BIT);
                    state = SECOND_EVENT;
                    continue;
                }
                // If we receive a release event, return RELEASE
                else if (bits & BUTTON_RELEASE_BIT) {
                    ESP_LOGI("BUTTONS", "Pair button released!");
                    event->type = RELEASE;
                    return;
                }
                // If we receive no event and have no previous press, return NO_PRESS
                else if (press_count == 0) {
                    event->type = NO_PRESS;
                    return;
                }
                // If we receive no event and have a previous press, return SHORT_PRESS 
                else {
                    event->type = SHORT_PRESS;
                    return;
                }
                break;
            case SECOND_EVENT:
                bits = xEventGroupWaitBits(*event_group,
                        BUTTON_RELEASE_BIT,
                        pdFALSE,
                        pdFALSE,
                        pdMS_TO_TICKS(cfg->hold_time_ms));
                dt = xTaskGetTickCount() - dt;
                // If we receive a release event, go to the next state
                if (bits & BUTTON_RELEASE_BIT) {
                    ESP_LOGI("BUTTONS", "Pair button released!");
                    xEventGroupClearBits(*event_group, BUTTON_RELEASE_BIT);
                    state = PRESS_STATE;
                    continue;
                }
                // If we receive no event and have no previous press, return HOLD
                else if (press_count == 0) {
                    event->type = HOLD;
                    return;
                }
                // If we receive no event and have a previous press, return PRESS_AND_HOLD
                else {
                    event->type = PRESS_AND_HOLD;
                    return;
                }
                break;
            case PRESS_STATE:
                // If there was a previous press, return DOUBLE_PRESS
                if (press_count > 0) {
                    event->type = DOUBLE_PRESS;
                    return;
                }
                // If there was no previous press and the press time is less than the long press time, go to first state
                else if (dt < pdMS_TO_TICKS(cfg->long_press_time_ms)) {
                    press_count++;
                    ticks_waited = pdMS_TO_TICKS(cfg->double_press_time_ms);
                    state = FIRST_EVENT;
                    continue;
                }
                // If there was no previous press and the press time is greater than or equal to the long press time, return LONG_PRESS
                else {
                    event->type = LONG_PRESS;
                    return;
                }
                break;
            default:
                ESP_LOGE("BUTTONS", "Invalid button state");
                event->type = NO_PRESS;
                return;
        }
    }
}

void buttons_get_event(int buttons, button_event_t *event)
{
    // Wait for the button press
    event->pin = -1;
    int ret = xQueueReceive(queues[buttons], &event->pin, 0);
    if (ret != pdTRUE) {
        event->type = NO_PRESS;
        return;
    }

    // Get the pointer to the button that was pressed
    button_isr_args_t *args = _buttons[buttons];
    while (args != NULL) {
        if (args->pin == event->pin) {
            break;
        }
        args = args->next;
    }
    if (args == NULL) {
        ESP_LOGE("BUTTONS", "Failed to find button ISR args for pin %d", event->pin);
        event->type = NO_PRESS;
        return;
    }

    EventBits_t bits = xEventGroupGetBits(*(args->event_group));
    if (bits & BUTTON_PRESS_BIT) {
        event->type = PRESS;
    }
    else if (bits & BUTTON_RELEASE_BIT) {
        event->type = RELEASE;
    }
    else {
        event->type = NO_PRESS;
    }
}

void buttons_get_state(int buttons, uint64_t *pin_mask)
{
    *pin_mask = 0;
    button_isr_args_t *args = _buttons[buttons];
    while (args != NULL) {
        if (gpio_get_level(args->pin))
        {
            *pin_mask |= (1 << args->pin);
        }
        args = args->next;
    }
}

void buttons_clear_event(int buttons)
{
    // Clear the queue and event group bits
    xQueueReset(queues[buttons]);
    button_isr_args_t *args = _buttons[buttons];
    while (args != NULL) {
        xEventGroupClearBits(*args->event_group, BUTTON_RELEASE_BIT | BUTTON_PRESS_BIT);
        args = args->next;
    }
}

void buttons_set_callback(int buttons, void (*callback)(void *), void *args)
{
    cfgs[buttons].callback = callback;
    cfgs[buttons].args = args;
}

void buttons_set_long_press_time(int buttons, uint16_t time_ms)
{
    cfgs[buttons].long_press_time_ms = time_ms;
}

void buttons_set_double_press_time(int buttons, uint16_t time_ms)
{
    cfgs[buttons].double_press_time_ms = time_ms;
}

void buttons_set_hold_time(int buttons, uint16_t time_ms)
{
    cfgs[buttons].hold_time_ms = time_ms;
}

void buttons_disable_pin(int buttons, uint8_t pin)
{
    esp_err_t err = gpio_isr_handler_remove(pin);
    if (err != ESP_OK) {
        ESP_LOGE("BUTTONS", "Failed to remove ISR for pin %d. error: %s", pin, esp_err_to_name(err));
    }
}

void buttons_enable_pin(int buttons, uint8_t pin)
{
    esp_err_t err = gpio_isr_handler_add(pin, buttons_isr, (void*)_buttons[buttons]);
    if (err != ESP_OK) {
        ESP_LOGE("BUTTONS", "Failed to add ISR for pin %d. error: %s", pin, esp_err_to_name(err));
    }
}

void buttons_get_config(int buttons, button_config_t *cfg)
{
    memcpy(cfg, _buttons[buttons]->cfg, sizeof(button_config_t));
}
