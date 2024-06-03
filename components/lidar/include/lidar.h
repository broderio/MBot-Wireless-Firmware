#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uart.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#ifndef LIDAR_H
#define LIDAR_H

#define LIDAR_SYNC                  0xA5
#define LIDAR_SYNC_INV              0x5A
#define LIDAR_EXP_SYNC1             0xA
#define LIDAR_EXP_SYNC2             0x5

#define LIDAR_GET_INFO              0x50
#define LIDAR_GET_HEALTH            0x52

#define LIDAR_STOP                  0x25
#define LIDAR_RESET                 0x40
#define LIDAR_SCAN                  0x20
#define LIDAR_EXP_SCAN              0x82
#define LIDAR_FORCE_SCAN            0x21
#define LIDAR_GET_INFO              0x50
#define LIDAR_GET_HEALTH            0x52
#define LIDAR_GET_SAMPLE_RATE       0x59
#define LIDAR_GET_LIDAR_CONF        0x84

#define LIDAR_SCAN_RESP_LEN         5
#define LIDAR_SCAN_DTYPE            0x81

#define LIDAR_EXP_SCAN_RESP_LEN     84
#define LIDAR_EXP_SCAN_DTYPE        0x82

#define LIDAR_DESCRIPTOR_LEN        7
#define LIDAR_INFO_LEN              20
#define LIDAR_HEALTH_LEN            3

#define LIDAR_INFO_TYPE             0x04
#define LIDAR_HEALTH_TYPE           0x06

#define LIDAR_BAUDRATE              115200
#define LIDAR_MAX_MOTOR_PWM         (1 << 10)
#define LIDAR_DEFAULT_MOTOR_PWM     0.5
#define LIDAR_BUFFER_SIZE           2048

/**
 * @brief Structure representing a lidar device.
 */
typedef struct lidar_t lidar_t;

/**
 * @brief Enumeration for different scan types.
 */
typedef enum {
    SCAN_TYPE_STANDARD,   /**< Normal scan type. */
    SCAN_TYPE_EXPRESS,   /**< Express scan type. */
    SCAN_TYPE_NONE
} lidar_scan_type_t;

/**
 * @brief Enumeration for different point densities.
 */
typedef enum {
    DENSITY_STANDARD,   /**< Standard point density. */
    DENSITY_QUARTER,    /**< Quarter point density. */
    DENSITY_HALF,       /**< Half point density. */
    DENSITY_DOUBLE      /**< Double point density. */
} lidar_scan_density_t;

/**
 * @brief Structure representing lidar information.
 */
typedef struct {
    uint8_t model;                      /**< Model of the lidar device. */
    uint8_t firmware_minor;             /**< Minor version of the firmware. */
    uint8_t firmware_major;             /**< Major version of the firmware. */
    uint8_t hardware;                   /**< Hardware version of the lidar device. */
    uint8_t serialnumber[16];           /**< Serial number of the lidar device. */
} lidar_info_t;

/**
 * @brief Structure representing lidar health information.
 */
typedef struct {
    uint8_t status;                     /**< Status of the lidar device. */
    uint16_t error_code;                /**< Error code of the lidar device. */
} lidar_health_t;

typedef enum {
    LIDAR_HEALTH_GOOD,              /**< Good health status. */
    LIDAR_HEALTH_WARNING,           /**< Warning health status. */
    LIDAR_HEALTH_ERROR              /**< Error health status. */
} lidar_health_status_t;

/**
 * @brief Structure representing lidar descriptor.
 */
typedef struct {
    uint8_t dsize;                      /**< Size of the descriptor. */
    uint8_t is_single;                  /**< Flag indicating if the descriptor is single. */
    uint8_t dtype;                      /**< Type of the descriptor. */
} lidar_descriptor_t;

/**
 * @brief Creates a new lidar device.
 *
 * @param rx_pin The RX pin number.
 * @param tx_pin The TX pin number.
 * @param motor_pin The motor pin number.
 * @return A pointer to the created lidar device.
 */
lidar_t *lidar_create(uint8_t port, uint8_t rx_pin, uint8_t tx_pin, uint8_t motor_pin);

/**
 * @brief Frees the memory allocated for a lidar device.
 *
 * @param lidar A pointer to the lidar device.
 */
void lidar_free(lidar_t *lidar);

/**
 * @brief Gets the UART port currently set for the LIDAR module.
 *
 * @param lidar Pointer to the LIDAR object.
 * @return The UART port currently set.
 */
uint8_t lidar_get_port(lidar_t *lidar);

/**
 * @brief Gets the RX pin currently set for the LIDAR module.
 *
 * @param lidar Pointer to the LIDAR object.
 * @return The RX pin currently set.
 */
uint8_t lidar_get_rx_pin(lidar_t *lidar);

/**
 * @brief Gets the TX pin currently set for the LIDAR module.
 *
 * @param lidar Pointer to the LIDAR object.
 * @return The TX pin currently set.
 */
uint8_t lidar_get_tx_pin(lidar_t *lidar);

/**
 * @brief Gets the motor pin currently set for the LIDAR module.
 *
 * @param lidar Pointer to the LIDAR object.
 * @return The motor pin currently set.
 */
uint8_t lidar_get_motor_pin(lidar_t *lidar);

/**
 * @brief Sets the scan density for the LIDAR module.
 *
 * @param lidar Pointer to the LIDAR object.
 * @param density The scan density to set.
 */
void lidar_set_scan_density(lidar_t *lidar, lidar_scan_density_t density);

/**
 * @brief Gets the scan density currently set for the LIDAR module.
 *
 * @param lidar Pointer to the LIDAR object.
 * @return The scan density currently set.
 */
lidar_scan_density_t lidar_get_scan_density(lidar_t *lidar);

/**
 * @brief Gets the scan type currently set for the LIDAR module.
 * 
 * @param lidar Pointer to the LIDAR object.
 * @return The scan type currently set.
*/
lidar_scan_type_t lidar_get_scan_type(lidar_t *lidar);

/**
 * @brief Checks if the LIDAR module is currently scanning.
 *
 * @param lidar Pointer to the LIDAR object.
 * @return 1 if scanning, 0 otherwise.
 */
uint8_t lidar_is_scanning(lidar_t *lidar);

/**
 * @brief Checks if the motor of the LIDAR is running.
 *
 * This function checks the status of the motor of the LIDAR device
 * specified by the `lidar` parameter.
 *
 * @param lidar A pointer to the LIDAR device structure.
 * @return Returns 1 if the motor is running, 0 otherwise.
 */
uint8_t lidar_is_motor_running(lidar_t *lidar);

/**
 * @brief Starts the lidar motor with the specified duty cycle.
 *
 * @param lidar A pointer to the lidar device.
 * @param duty_cycle The duty cycle to start the motor with.
 * @return 0 if successful, non-zero otherwise.
 */
uint8_t lidar_start_motor(lidar_t *lidar, float duty_cycle);

/**
 * @brief Stops the lidar motor.
 *
 * @param lidar A pointer to the lidar device.
 */
void lidar_stop_motor(lidar_t *lidar);

/**
 * @brief Retrieves information about the lidar device.
 *
 * This function retrieves information about the lidar device specified by the `lidar` parameter
 * and stores the information in the `info` parameter.
 *
 * @param lidar Pointer to the lidar device structure.
 * @param info Pointer to the structure where the lidar information will be stored.
 * @return 0 if successful, non-zero otherwise.
 */
uint8_t lidar_get_info(lidar_t *lidar, lidar_info_t *info);

/**
 * @brief Retrieves the health status of the lidar device.
 *
 * This function retrieves the health status of the lidar device specified by the `lidar` parameter
 * and stores the status in the `health` parameter.
 *
 * @param lidar Pointer to the lidar device structure.
 * @param health Pointer to the structure where the lidar health status will be stored.
 * @return 0 if successful, non-zero otherwise.
 */
uint8_t lidar_get_health(lidar_t *lidar, lidar_health_t *health);

/**
 * @brief Resets the lidar device.
 *
 * @param lidar A pointer to the lidar device.
 */
void lidar_reset(lidar_t *lidar);

/**
 * @brief Clears the input of the lidar device.
 *
 * @param lidar A pointer to the lidar device.
 * @return 0 if successful, non-zero otherwise.
 */
void lidar_clear_input(lidar_t *lidar);

/**
 * @brief Starts a lidar scan with the specified scan type.
 *
 * @param lidar A pointer to the lidar device.
 * @param type The scan type to start.
 * @return 0 if successful, non-zero otherwise.
 */
uint8_t lidar_start_scan(lidar_t *lidar, lidar_scan_type_t type);

/**
 * @brief Stops the lidar scan.
 *
 * @param lidar A pointer to the lidar device.
 */
void lidar_stop_scan(lidar_t *lidar);

/**
 * @brief Retrieves a 360-degree scan from the lidar device.
 *
 * @param lidar A pointer to the lidar device.
 * @param distances An array to store the distance values.
 * @return 0 if successful, non-zero otherwise.
 */
uint8_t lidar_get_scan_360(lidar_t *lidar, uint16_t distances[360]);

#endif