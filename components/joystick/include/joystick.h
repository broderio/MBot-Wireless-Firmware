#pragma once

#include "string.h"
#include "math.h"

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#include "nvs_flash.h"
#include "esp_log.h"

#define DEFAULT_JOYSTICK_CONFIG         \
        {                               \
            .x_max = 3903,              \
            .x_min = 0,                 \
            .x_rest = 1745,             \
            .y_max = 3813,              \
            .y_min = 0,                 \
            .y_rest = 1651              \
        }

/**
 * @brief Structure representing the configuration of a joystick.
 */
typedef struct joystick_config_t {
    int32_t x_max;   /**< Maximum value for the X-axis. */
    int32_t x_min;   /**< Minimum value for the X-axis. */
    int32_t x_rest;  /**< Resting value for the X-axis. */
    int32_t y_max;   /**< Maximum value for the Y-axis. */
    int32_t y_min;   /**< Minimum value for the Y-axis. */
    int32_t y_rest;  /**< Resting value for the Y-axis. */
} joystick_config_t;

/**
 * @brief Structure representing a joystick.
 */
typedef struct joystick_t joystick_t;

/**
 * @brief Creates a joystick object.
 *
 * @param x_pin The pin number for the X-axis of the joystick.
 * @param y_pin The pin number for the Y-axis of the joystick.
 * @return A pointer to the created joystick object.
 */
joystick_t *joystick_create(uint8_t x_pin, uint8_t y_pin);

/**
 * @brief Frees the memory allocated for a joystick object.
 *
 * @param js A pointer to the joystick object to be freed.
 */
void joystick_free(joystick_t *js);

/**
 * @brief Reads the current position of the joystick.
 *
 * @param js A pointer to the joystick object.
 */
void joystick_read(joystick_t *js);

/**
 * @brief Sets the deadband for the joystick.
 *
 * @param js A pointer to the joystick object.
 * @param deadband The deadband value to be set.
 */
void joystick_set_deadband(joystick_t *js, float deadband);

/**
 * @brief Gets the raw X-axis value of the joystick.
 *
 * @param js A pointer to the joystick object.
 * @return The raw X-axis value.
 */
int32_t joystick_get_x_raw(joystick_t *js);

/**
 * @brief Gets the raw Y-axis value of the joystick.
 *
 * @param js A pointer to the joystick object.
 * @return The raw Y-axis value.
 */
int32_t joystick_get_y_raw(joystick_t *js);

/**
 * @brief Gets the normalized X-axis value of the joystick.
 *
 * @param js A pointer to the joystick object.
 * @return The normalized X-axis value.
 */
float joystick_get_x(joystick_t *js);

/**
 * @brief Gets the normalized Y-axis value of the joystick.
 *
 * @param js A pointer to the joystick object.
 * @return The normalized Y-axis value.
 */
float joystick_get_y(joystick_t *js);

/**
 * @brief Checks if the joystick is calibrated.
 *
 * @param js A pointer to the joystick object.
 * @return 1 if calibrated, 0 otherwise.
 */
uint8_t joystick_is_calibrated(joystick_t *js);

/**
 * @brief Gets the configuration of the joystick.
 *
 * @param js A pointer to the joystick object.
 * @return The configuration of the joystick.
 */
joystick_config_t joystick_get_config(joystick_t *js);

/**
 * @brief Writes the configuration of the joystick.
 *
 * @param cfg A pointer to the joystick configuration object.
 */
void joystick_write_cfg(joystick_config_t *cfg);