/**
 * @file buttonBase.h
 * @brief Button driver for ESP32.
 * This driver provides a simple interface for reading the state of a button and
 * registering callback functions to be called when the button is pressed or released.
 * 
 * This driver is based on the EasyButton library developed by evert-arias and can be found at:
 * https://github.com/evert-arias/EasyButton
*/

#pragma once

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

/**
 * @brief The button object structure.
 * 
 * This structure represents a button object and should be treated as opaque.
 */
typedef struct button_t button_t;

/**
 * @brief The button callback function type.
 * 
 * This type represents a callback function to be called when a button is pressed or released.
 * The callback function will be called with an optional argument.
 */
typedef void (*button_callback_t)(void *arg);

/**
 * @brief Creates a new button object.
 * 
 * By default, pull-up is enabled, the button is considered active low, the debounce time 
 * is 35ms, the held threshold is 1s, and interrupt is disabled.
 * 
 * @param pin The pin number to which the button is connected.
 * @return A pointer to the newly created button object.
 */
button_t *button_create(uint8_t pin);

/**
 * @brief Frees the memory allocated for a button object.
 * 
 * This function frees the memory allocated for the specified button object.
 * 
 * @param button A pointer to the button object.
 */
void button_free(button_t *button);

/**
 * @brief Enables the interrupt for a button.
 *
 * This function enables the interrupt for the specified button. When the button is pressed,
 * the interrupt will be triggered. This will cause the callback functions to be called from the ISR.
 * Ensure that if you enable the interrupt, your callback functions are ISR safe
 *
 * @param button A pointer to the button object.
 */
void button_interrupt_enable(button_t *button);

/**
 * @brief Disables the interrupt for a button.
 *
 * This function disables the interrupt for the specified button.
 *
 * @param button A pointer to the button object.
 */
void button_interrupt_disable(button_t *button);

/**
 * @brief Sets the button to be active low.
 *
 * This function sets the button to be active low, meaning that the button is considered
 * pressed when the corresponding pin is pulled low. By default, buttons are active low.
 *
 * @param button A pointer to the button object.
 */
void button_set_active_low(button_t *button);

/**
 * @brief Sets the button to be active high.
 *
 * This function sets the button to be active high, meaning that the button is considered
 * pressed when the corresponding pin is pulled high. By default, buttons are active low.
 *
 * @param button A pointer to the button object.
 */
void button_set_active_high(button_t *button);

/**
 * @brief Retrieves the active level of a button.
 *
 * This function returns the active level of the specified button. The active level
 * indicates the logical state of the button when it is considered "pressed" or "active".
 * For example, if the active level is HIGH, the button is considered pressed when its
 * corresponding pin reads a HIGH voltage level. By default, buttons are active low.
 *
 * @param button A pointer to the button object.
 * @return The active level of the button.
 */
uint8_t button_get_active_level(button_t *button);

/**
 * @brief Sets the debounce time for a button.
 *
 * This function sets the debounce time for a button, which is the amount of time
 * that the button's state must remain stable before it is considered a valid button press.
 *
 * @param button Pointer to the button object.
 * @param time The debounce time in milliseconds.
 */

void button_set_debounce_time(button_t *button, uint32_t time);

/**
 * @brief Retrieves the debounce time of a button.
 *
 * This function retrieves the debounce time of the specified button.
 *
 * @param button A pointer to the button object.
 * @return The debounce time of the button in milliseconds.
 */
uint32_t button_get_debounce_time(button_t *button);

/**
 * @brief Sets the hold threshold for a button.
 *
 * This function sets the hold threshold for a button, which is the minimum time
 * that the button needs to be held down for it to be considered a "hold" event.
 *
 * @param button The button for which to set the hold threshold.
 * @param time The hold threshold time in milliseconds.
 */
void button_set_held_threshold(button_t *button, uint32_t time);

/**
 * @brief Retrieves the hold threshold for a button.
 *
 * This function retrieves the hold threshold for the specified button.
 *
 * @param button A pointer to the button object.
 * @return The hold threshold for the button in milliseconds.
 */
uint32_t button_get_held_threshold(button_t *button);

/**
 * @brief Sets the pull-up resistor for the button.
 *
 * This function enables or disables the pull-up resistor for the specified button.
 *
 * @param button Pointer to the button structure.
 * @param enabled Flag indicating whether the pull-up resistor should be enabled (1) or disabled (0).
 */

void button_set_pull_up(button_t *button, uint8_t enabled);

/**
 * @brief Retrieves the pull-up resistor setting for a button.
 *
 * This function retrieves the pull-up resistor setting for the specified button.
 *
 * @param button A pointer to the button object.
 * @return A uint8_t value indicating whether the pull-up resistor is enabled (non-zero) or disabled (zero).
 */
uint8_t button_get_pull_up(button_t *button);

/**
 * @brief Reads the state of a button.
 *
 * This function reads the state of the specified button and returns the current state as a uint8_t value.
 * This function will call the specified on_pressed and on_pressed_for callback functions.
 * If interrupts are enabled, this function will be called from the ISR.
 *
 * @param button A pointer to the button_t structure representing the button.
 * @return The current state of the button as a uint8_t value.
 */
uint8_t button_read(button_t *button);

/**
 * @brief Sets the callback function to be called when the button is pressed.
 *
 * The callback function will be called on release if the button has been pressed for less than the hold threshold.
 *
 * @param button A pointer to the button object.
 * @param callback The callback function to be called when the button is pressed.
 * @param arg An optional argument to be passed to the callback function.
 */
void button_on_press(button_t *button, button_callback_t callback, void *arg);

/**
 * @brief Sets a callback function to be called when a button is pressed for a specified duration.
 *
 * The callback function will be called on release if the button has been pressed for the hold threshold.
 *
 * @param button The button object.
 * @param callback The callback function to be called when the button is pressed for the specified duration.
 * @param arg An optional argument to be passed to the callback function.
 */
void button_on_pressed_for(button_t *button, button_callback_t callback, void *arg);

/**
 * @brief Checks if the button is currently pressed.
 *
 * This function checks if the button is currently in the pressed state.
 *
 * @param button A pointer to the button object.
 * @return A uint8_t value indicating whether the button is pressed (non-zero) or not (zero).
 */
uint8_t button_is_pressed(button_t *button);

/**
 * @brief Checks if the button is currently released.
 *
 * This function checks if the button is currently in the released state.
 *
 * @param button A pointer to the button object.
 * @return A uint8_t value indicating whether the button is released (non-zero) or not (zero).
 */
uint8_t button_is_released(button_t *button);

/**
 * @brief Checks if the button was pressed.
 *
 * This function checks if the button was pressed since the last time this function was called.
 *
 * @param button A pointer to the button object.
 * @return A uint8_t value indicating whether the button was pressed (non-zero) or not (zero).
 */
uint8_t button_was_pressed(button_t *button);

/**
 * @brief Checks if the button was pressed for a specified duration.
 *
 * This function checks if the button was pressed for a specified number of ticks since the last time this function was called.
 *
 * @param button A pointer to the button object.
 * @param duration The duration of the press to check for.
 * @return A uint8_t value indicating whether the button was pressed for the specified duration (non-zero) or not (zero).
 */
uint8_t button_was_pressed_for(button_t *button, uint32_t duration);

/**
 * @brief Checks if the button was released.
 *
 * This function checks if the button was released since the last time this function was called.
 *
 * @param button A pointer to the button object.
 * @return A uint8_t value indicating whether the button was released (non-zero) or not (zero).
 */
uint8_t button_was_released(button_t *button);

/**
 * @brief Checks if the button was released for a specified duration.
 *
 * This function checks if the button was released for a specified number of ticks since the last time this function was called.
 *
 * @param button A pointer to the button object.
 * @param duration The duration of the release to check for.
 * @return A uint8_t value indicating whether the button was released for the specified duration (non-zero) or not (zero).
 */
uint8_t button_was_released_for(button_t *button, uint32_t duration);

/**
 * @brief Waits for the specified button to be pressed.
 * 
 * This function will block the current task until the specified button is pressed.
 *
 * @param button A pointer to the button object.
 */
void button_wait_for_press(button_t *button);

/**
 * @brief Waits for the specified button to be released.
 * 
 * This function will block the current task until the specified button is released.
 *
 * @param button A pointer to the button object.
 */
void button_wait_for_release(button_t *button);