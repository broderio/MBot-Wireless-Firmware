#ifndef BUTTONS_H
#define BUTTONS_H

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

typedef enum {
    NO_PRESS,
    PRESS,
    SHORT_PRESS,
    LONG_PRESS,
    HOLD,
    DOUBLE_PRESS,
    PRESS_AND_HOLD,
    RELEASE
} button_press_t;

#pragma pack(push, 1)
typedef struct button_event_t {
    uint8_t pin;
    button_press_t type;
} button_event_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct button_config_t {
    uint64_t pin_bit_mask;
    uint16_t long_press_time_ms;
    uint16_t hold_time_ms;
    uint16_t double_press_time_ms;
    void (*callback)(void *);
    void *args;
} button_config_t;
#pragma pack(pop)

int buttons_init(button_config_t *cfg);
void buttons_deinit(int buttons);

void buttons_wait_for_event(int buttons, button_event_t *event, TickType_t ticks_to_wait);
void buttons_get_event(int buttons, button_event_t *event);
void buttons_get_state(int buttons, uint64_t *pin_mask);
void buttons_clear_event(int buttons);

void buttons_set_callback(int buttons, void (*callback)(void *), void *args);
void buttons_set_long_press_time(int buttons, uint16_t time_ms);
void buttons_set_double_press_time(int buttons, uint16_t time_ms);
void buttons_set_hold_time(int buttons, uint16_t time_ms);

void buttons_disable_pin(int buttons, uint8_t pin);
void buttons_enable_pin(int buttons, uint8_t pin);

void buttons_get_config(int buttons, button_config_t *cfg);

#endif