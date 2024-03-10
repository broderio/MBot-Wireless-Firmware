#ifndef LED_H
#define LED_H

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

int led_init(uint64_t pin_mask);
void led_deinit();

int led_on(int pin);
int led_off(int pin);
int led_toggle(int pin);

int led_blink(int pin, int period);
int led_is_blinking(int pin);
int led_stop_blink(int pin);

#endif