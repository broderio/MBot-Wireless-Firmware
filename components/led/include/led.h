#pragma once

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

typedef struct led_t led_t;

/**
 * @brief Creates a new LED object.
 *
 * This function creates a new LED object with the specified pin number.
 *
 * @param pin The pin number to which the LED is connected.
 * @return A pointer to the created LED object.
 */
led_t *led_create(uint8_t pin);

/**
 * @brief Frees the memory allocated for an LED object.
 *
 * This function frees the memory allocated for the specified LED object.
 *
 * @param led A pointer to the LED object to be freed.
 */
void led_free(led_t *led);

/**
 * @brief Turns on the LED.
 *
 * This function turns on the specified LED.
 *
 * @param led A pointer to the LED object to be turned on.
 */
void led_on(led_t *led);

/**
 * @brief Checks if the LED is currently on.
 *
 * This function checks if the specified LED is currently on.
 *
 * @param led A pointer to the LED object to be checked.
 * @return 1 if the LED is on, 0 otherwise.
 */
uint8_t led_is_on(led_t *led);

/**
 * @brief Turns off the LED.
 *
 * This function turns off the specified LED.
 *
 * @param led A pointer to the LED object to be turned off.
 */
void led_off(led_t *led);

/**
 * @brief Checks if the LED is currently off.
 *
 * This function checks if the specified LED is currently off.
 *
 * @param led A pointer to the LED object to be checked.
 * @return 1 if the LED is off, 0 otherwise.
 */
uint8_t led_is_off(led_t *led);

/**
 * @brief Sets the state of the LED.
 *
 * This function sets the state of the specified LED to the specified state.
 *
 * @param led A pointer to the LED object to be set.
 * @param state The state to set the LED to (1 for on, 0 for off).
 */
void led_set(led_t *led, uint8_t state);

/**
 * @brief Reads the current state of the LED.
 *
 * This function reads the current state of the specified LED.
 *
 * @param led A pointer to the LED object to be read.
 * @return The current state of the LED (1 if on, 0 if off).
 */
uint8_t led_read(led_t *led);

/**
 * @brief Toggles the state of the LED.
 *
 * This function toggles the state of the specified LED.
 *
 * @param led A pointer to the LED object to be toggled.
 */
void led_toggle(led_t *led);

/**
 * @brief Blinks an LED with the specified period.
 *
 * This function starts a task that blinks the LED with the specified period.
 * If the LED is already blinking, a warning message is logged and the function returns.
 *
 * @param led Pointer to the LED object.
 * @param period The period of the LED blinking in milliseconds.
 */
void led_blink(led_t *led, uint32_t period);

/**
 * @brief Stops the blinking of an LED.
 *
 * This function stops the blinking of the specified LED. If the LED is not currently blinking,
 * a warning message is logged and the function returns without taking any action.
 *
 * @param led Pointer to the LED object.
 */
void led_stop_blink(led_t *led);