#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "led.h"

#define STOP_BLINKING_BIT BIT0
#define BLINK_CONFIRM_BIT BIT1

#pragma pack(push, 1)
typedef struct blink_arg_t blink_arg_t;

typedef struct blink_arg_t {
    int pin;
    int delay;
    EventGroupHandle_t *event_group;
    blink_arg_t *next;
} blink_arg_t;
#pragma pack(pop)

static uint64_t led_pin_mask = 0;
static uint64_t led_state = 0;
static uint64_t led_blink_state = 0;

static blink_arg_t *blinks = NULL;
static uint8_t num_blinks = 0;

int _check_mask(int pin)
{
    if (led_pin_mask & (1 << pin)) {
        return 0;
    }
    return -1;
}

void _blink_task(void* arg) {
    blink_arg_t* blink_arg = (blink_arg_t*) arg;
    while (true) {
        int state = (led_state >> blink_arg->pin) & 0b1;
        gpio_set_level(blink_arg->pin, !state);
        vTaskDelay(blink_arg->delay / portTICK_PERIOD_MS);
        gpio_set_level(blink_arg->pin, state);
        EventBits_t bits = xEventGroupGetBits(*blink_arg->event_group);
        if (bits & STOP_BLINKING_BIT) {
            xEventGroupClearBits(*blink_arg->event_group, STOP_BLINKING_BIT);
            xEventGroupSetBits(*blink_arg->event_group, BLINK_CONFIRM_BIT);
            vTaskDelete(NULL);
        }
        vTaskDelay(blink_arg->delay / portTICK_PERIOD_MS);
    }
}

int led_init(uint64_t pin_mask)
{
    led_pin_mask = pin_mask;
    gpio_config_t config = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    int ret = gpio_config(&config);
    if (ret != ESP_OK)
    {
        ESP_LOGE("LED", "Failed to initialize GPIOs.");
        return -1;
    }
    return 0;
}

void led_deinit()
{
    if (num_blinks > 0)
    {
        for (int i = 0; i < 64; ++i)
        {
            if ((led_blink_state >> i) & 0b1)
            {
                led_stop_blink(i);
            }
        }
    }

    // Deinit GPIOs
}

int led_on(int pin)
{
    if (_check_mask(pin) < 0) {
        ESP_LOGE("LED", "Error, invalid pin.");
        return -1;
    }

    if (led_is_blinking(pin))
    {
        led_stop_blink(pin);
    }

    led_state |= (1 << pin);
    gpio_set_level(pin, 1);
    return 0;
}

int led_off(int pin)
{
    if (_check_mask(pin) < 0) {
        ESP_LOGE("LED", "Error, invalid pin.");
        return -1;
    }

    if (led_is_blinking(pin))
    {
        led_stop_blink(pin);
    }
    
    led_state &= ~(1 << pin);
    gpio_set_level(pin, 0);
    return 0;
}

int led_toggle(int pin)
{
    if (_check_mask(pin) < 0) {
        ESP_LOGE("LED", "Error, invalid pin.");
        return -1;
    }

    if (led_is_blinking(pin))
    {
        led_stop_blink(pin);
    }

    led_state ^= (1 << pin);
    gpio_set_level(pin, (led_state >> pin) & 0b1);
    return 0;

}

blink_arg_t* _alloc_blink(int pin, int delay)
{
    // Traverse to first null element
    blink_arg_t *curr_blink = blinks;
    while (curr_blink != NULL) {
        curr_blink = curr_blink->next;
    }

    // Allocate memory
    curr_blink = (blink_arg_t*)malloc(sizeof(blink_arg_t));
    if (curr_blink == NULL)
    {
        ESP_LOGE("LED", "Failed to allocate memory for blink args.");
        return NULL;
    }

    curr_blink->event_group = (EventGroupHandle_t*)malloc(sizeof(EventGroupHandle_t));
    if (curr_blink->event_group == NULL)
    {
        ESP_LOGE("LED", "Failed to allocate memory for blink event group.");
        free(curr_blink);
        return NULL;
    }

    // Assign variables
    *curr_blink->event_group = xEventGroupCreate();
    curr_blink->pin = pin;
    curr_blink->delay = delay;
    curr_blink->next = NULL;
    return curr_blink;
}

void _dealloc_blink(int pin)
{
    // Traverse until we find the correct pin
    blink_arg_t *prev_blink = NULL;
    blink_arg_t *curr_blink = blinks;
    while (curr_blink != NULL && curr_blink->pin != pin)
    {
        prev_blink = curr_blink;
        curr_blink = curr_blink->next;
    }
    if (curr_blink == NULL)
    {
        ESP_LOGE("LED", "Error, invalid pin input to dealloc.");
        return;
    }

    prev_blink->next = curr_blink->next;
    vEventGroupDelete(*curr_blink->event_group);
    free(curr_blink->event_group);
    free(curr_blink);
}

int led_blink(int pin, int period)
{
    blink_arg_t *curr_blink = _alloc_blink(pin, period / 2);
    if (curr_blink == NULL)
    {
        ESP_LOGE("LED", "Failed to allocate blink args.");
        return -1;
    }
    
    xTaskCreate(_blink_task, "blink_task", 1024, curr_blink, 5, NULL); // Pass curr_blink instead of &blinks[num_blinks]
    led_blink_state |= (1 << pin);
    num_blinks++;
    return 0;
}

int led_is_blinking(int pin)
{
    return (led_blink_state >> pin) & 0b1;
}

int led_stop_blink(int pin)
{
    blink_arg_t *curr_blink = blinks;
    while (curr_blink != NULL && curr_blink->pin != pin)
    {
        curr_blink = curr_blink->next;
    }
    if (curr_blink == NULL)
    {
        ESP_LOGE("LED", "Error, invalid pin input to stop blink.");
        return -1;
    }

    xEventGroupSetBits(*curr_blink->event_group, STOP_BLINKING_BIT);
    xEventGroupWaitBits(*curr_blink->event_group,
                        BLINK_CONFIRM_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);
    xEventGroupClearBits(*curr_blink->event_group, BLINK_CONFIRM_BIT);
    _dealloc_blink(pin);
    led_blink_state &= ~(1 << pin);
    num_blinks--;
    return 0;
}