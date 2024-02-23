/**
 * @file lidar.c
 * @brief This file contains the implementation of functions for interacting with the lidar sensor.
 *
 * The lidar module provides functions for initializing the lidar, starting and stopping scans,
 * retrieving lidar information and health status, and controlling the lidar motor.
 * It also includes functions for processing lidar scan data and printing the scan results.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uart.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "lidar.h"

typedef struct
{
    uint16_t angle_q6;
    uint32_t distance_q2;
    uint8_t quality;
}  scan_t;

typedef struct 
{
    uint16_t angles[32];
    uint16_t distances[32];
} express_scan_t;

typedef struct
{
    uint16_t distance_1;
    uint16_t distance_2;
    uint8_t d_angle_1;
    uint8_t d_angle_2;
} cabin_t;

typedef struct
{
    uint16_t start_angle_q6;
    cabin_t cabins[16];
} express_scan_raw_t;

/**
 * Process a lidar scan.
 *
 * This function takes a lidar scan, represented by the `scan` structure, and processes it.
 * It receives the raw data of the scan in the `raw` parameter and stores the processed scan
 * in the `new_scan` parameter.
 *
 * @param scan The lidar scan structure.
 * @param raw The raw data of the scan.
 * @param new_scan The processed scan.
 * @return The status of the scan processing.
 */
int _process_scan(scan_t *scan, uint8_t *raw, uint8_t *new_scan)
{
    *new_scan = raw[0] & 0b1;                        // bit 0
    uint8_t inversed_new_scan = (raw[0] >> 1) & 0b1; // bit 1

    // Check if S == S`
    if (*new_scan == inversed_new_scan)
        return -1;

    scan->quality = raw[0] >> 2;

    // Check if C == 1
    uint8_t check_bit = raw[1] & 0b1; // bit 8
    if (check_bit != 1)
        return -1;

    scan->angle_q6 = (raw[1] >> 1) | ((uint16_t)raw[2] << 7);
    scan->distance_q2 = raw[3] | ((uint32_t)raw[4] << 8);
    return 0;
}

/**
 * @brief Retrieves the raw scan data in express mode.
 *
 * This function populates the provided `scan` structure with the raw scan data
 * obtained from the Lidar sensor. The raw data is stored in the `raw` buffer.
 *
 * @param scan Pointer to the `express_scan_raw_t` structure to store the raw scan data.
 * @param raw Pointer to the buffer to store the raw scan data.
 * @return int Returns 0 on success, or an error code on failure.
 */
int _get_raw_scan_express(express_scan_raw_t *scan, uint8_t *raw)
{
    // Parse metadata
    uint8_t sync_1 = raw[0] >> 4;
    uint8_t sync_2 = raw[1] >> 4;
    if (sync_1 != LIDAR_EXP_SYNC1 || sync_2 != LIDAR_EXP_SYNC2)
    {
        return -1;
    }

    uint8_t checksum = ((raw[1] & 0x0F) << 4) | (raw[0] & 0x0F);
    // uint8_t checksum_calc = 0;
    // for (int i = 0; i < LIDAR_EXP_SCAN_RESP_LEN; i++)
    // {
    //     checksum_calc ^= raw[i];
    // }
    // if (checksum != checksum_calc)
    // {
    //     return -1;
    // }

    scan->start_angle_q6 = (((uint16_t)(raw[3] & 0x7F)) << 8) | raw[2];

    // Parse cabins
    for (int i = 0; i < 16; ++i)
    {
        int index = 4 + 5 * i;

        // Get the distance values
        scan->cabins[i].distance_1 = (((uint16_t)raw[index + 1]) << 6) | (raw[index] >> 2);
        scan->cabins[i].distance_2 = (((uint16_t)raw[index + 3]) << 6) | (raw[index + 2] >> 2);

        // Get d_angle values
        scan->cabins[i].d_angle_1 = ((raw[index] & 0x03) << 4) | (raw[index + 4] & 0x0F);
        scan->cabins[i].d_angle_2 = ((raw[index + 2] & 0x03) << 4) | (raw[index + 4] >> 4);
    }
    return 0;
}

/**
 * Calculates the difference between two angles.
 *
 * @param w1 The first angle in degrees.
 * @param w2 The second angle in degrees.
 * @return The difference between the two angles.
 */
uint16_t _angle_diff(uint16_t w1, uint16_t w2)
{
    if (w1 <= w2)
    {
        return w2 - w1;
    }
    else
    {
        return 360 + w2 - w1;
    }
}

/**
 * @brief Process the express scan data and populate the output scan structure.
 *
 * This function takes two raw scan data structures and calculates the angles and distances
 * for each cabin in the express scan. The calculated values are stored in the output scan
 * structure.
 *
 * @param[out] out_scan Pointer to the output scan structure.
 * @param[in] raw_scan_1 Pointer to the first raw scan data structure.
 * @param[in] raw_scan_2 Pointer to the second raw scan data structure.
 * @return 0 if the process is successful, otherwise an error code.
 */
int _process_scan_express(express_scan_t *out_scan, const express_scan_raw_t *raw_scan_1, const express_scan_raw_t *raw_scan_2)
{
    for (int k = 0; k < 32; k += 2)
    {
        cabin_t cabin = raw_scan_1->cabins[k / 2];
        uint16_t w1 = raw_scan_1->start_angle_q6;
        uint16_t w2 = raw_scan_2->start_angle_q6;
        uint16_t d_angle = _angle_diff(w1, w2) / 32;

        out_scan->angles[k] = w1 + d_angle * k - cabin.d_angle_1;
        out_scan->angles[k + 1] = w1 + d_angle * (k + 1) - cabin.d_angle_2;
        out_scan->distances[k] = cabin.distance_1;
        out_scan->distances[k + 1] = cabin.distance_2;
    }
    return 0;
}

/**
 * Sends a payload command to the LIDAR device.
 *
 * @param cmd The command to be sent.
 * @param payload The payload data to be sent.
 * @param payload_len The length of the payload data.
 */
void _send_payload_cmd(uint8_t cmd, uint8_t *payload, int payload_len)
{
    // Create the request buffer
    uint8_t req[2 + 1 + payload_len + 1];
    
    // Set the sync byte, command byte, and payload length
    req[0] = LIDAR_SYNC;
    req[1] = cmd;
    req[2] = payload_len;
    
    // Copy the payload data into the request buffer
    for (int i = 0; i < payload_len; i++)
    {
        req[3 + i] = payload[i];
    }
    
    // Calculate the checksum
    uint8_t checksum = 0;
    for (int i = 0; i < 3 + payload_len; i++)
    {
        checksum ^= req[i];
    }
    
    // Add the checksum to the request buffer
    req[3 + payload_len] = checksum;
    
    // Send the request buffer over UART
    uart_write(0, (const char *)req, 4 + payload_len);
}

/**
 * @brief Sends a command to the LIDAR device.
 *
 * This function sends a command to the LIDAR device over UART.
 *
 * @param cmd The command to be sent.
 */
void _send_cmd(uint8_t cmd)
{
    uint8_t req[2];
    req[0] = LIDAR_SYNC;
    req[1] = cmd;
    uart_write(0, (const char *)req, 2);
}

/**
 * @brief Reads the descriptor from the LIDAR device.
 *
 * This function reads the descriptor bytes from the LIDAR device using UART communication.
 * It replaces the deprecated function `uart_read` with `uart_read_bytes`.
 *
 * @param descriptor Pointer to the `lidar_descriptor_t` structure to store the descriptor information.
 * @return The number of bytes read on success, or -1 if the descriptor bytes are invalid.
 */
int _read_descriptor(lidar_descriptor_t *descriptor)
{
    uint8_t descriptor_bytes[LIDAR_DESCRIPTOR_LEN] = {0};
    int bytes_read = uart_read_bytes(0, descriptor_bytes, LIDAR_DESCRIPTOR_LEN, portMAX_DELAY); // Replace uart_read with uart_read_bytes
    if (descriptor_bytes[0] != LIDAR_SYNC || descriptor_bytes[1] != LIDAR_SYNC_INV)
    {
        return -1;
    }
    descriptor->dsize = descriptor_bytes[2];
    descriptor->is_single = descriptor_bytes[5] == 0;
    descriptor->dtype = descriptor_bytes[6];
    return bytes_read;
}

/**
 * Reads the response from the LIDAR sensor.
 *
 * This function reads the response from the LIDAR sensor through UART communication.
 *
 * @param data The buffer to store the received data.
 * @param dsize The size of the buffer.
 * @return The number of bytes read, or -1 if an error occurred.
 */
int _read_response(uint8_t *data, int dsize)
{
    return uart_read(0, (char *)data, dsize, 5000);
}

/**
 * @brief Initializes the lidar sensor.
 *
 * This function initializes the lidar sensor by setting up the necessary configurations and parameters.
 *
 * @param lidar Pointer to the lidar_t structure.
 * @param rx_pin The RX pin number.
 * @param tx_pin The TX pin number.
 * @param motor_pin The motor control pin number.
 * @return 0 if initialization is successful, -1 otherwise.
 */
int lidar_init(lidar_t *lidar, int rx_pin, int tx_pin, int motor_pin)
{
    lidar->motor_running = false;
    lidar->density = STANDARD;
    lidar->scanning = false;
    lidar->port_num = 0;
    lidar->rx_pin = rx_pin;
    lidar->tx_pin = tx_pin;
    lidar->motor_pin = motor_pin;

    uart_init(lidar->port_num, lidar->rx_pin, lidar->tx_pin, 115200, LIDAR_BUFFER_SIZE);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 500, // Set output frequency at 500 Hz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = lidar->motor_pin,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    lidar_start_motor(lidar, LIDAR_DEFAULT_MOTOR_PWM);

    int error = 0;
    lidar_health_t health;
    error = lidar_get_health(&health);
    if (error != 0)
    {
        lidar_reset(lidar);
        error = lidar_get_health(&health);
        if (error != 0)
        {
            return -1;
        }
    }

    lidar_info_t info;
    error = lidar_get_info(&info);
    if (error != 0)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief Deinitializes the lidar module.
 *
 * This function stops the lidar scanning if it is currently scanning,
 * deinitializes the UART port used by the lidar, resets the lidar module,
 * and stops the lidar motor.
 *
 * @param lidar Pointer to the lidar_t structure representing the lidar module.
 * @return 0 on success, -1 on failure.
 */
int lidar_deinit(lidar_t *lidar)
{
    if (lidar->scanning)
    {
        lidar_stop_scan(lidar);
    }
    uart_deinit(lidar->port_num);
    lidar_reset(lidar);
    lidar_stop_motor(lidar);
    return 0;
}

/**
 * @brief Sets the point density for the lidar.
 *
 * This function sets the point density for the lidar sensor.
 *
 * @param lidar Pointer to the lidar_t structure.
 * @param density The desired point density.
 */
void lidar_set_point_density(lidar_t *lidar, POINT_DENSITY density)
{
    lidar->density = density;
}

/**
 * @brief Starts the motor of the lidar.
 *
 * This function starts the motor of the lidar sensor with the specified duty cycle.
 *
 * @param lidar Pointer to the lidar_t structure.
 * @param duty_cycle The duty cycle for the motor.
 */
void lidar_start_motor(lidar_t *lidar, int duty_cycle)
{
    if (lidar->motor_running)
    {
        return;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    lidar->motor_running = true;
}

/**
 * @brief Stops the motor of the lidar.
 *
 * This function stops the motor of the lidar sensor.
 *
 * @param lidar Pointer to the lidar_t structure.
 */
void lidar_stop_motor(lidar_t *lidar)
{
    if (!lidar->motor_running)
    {
        return;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    lidar->motor_running = false;
}

/**
 * @brief Retrieves information about the lidar device.
 *
 * This function sends a command to the lidar device to retrieve its information.
 * It reads the descriptor and verifies its validity. If the descriptor is valid,
 * it reads the response data and populates the lidar_info_t structure with the
 * retrieved information.
 *
 * @param info Pointer to the lidar_info_t structure to store the retrieved information.
 * @return 0 if successful, -1 if an error occurred.
 */
int lidar_get_info(lidar_info_t *info)
{
    // Send command to retrieve lidar information
    _send_cmd(LIDAR_GET_INFO);

    // Read the descriptor
    lidar_descriptor_t descriptor = {0};
    _read_descriptor(&descriptor);

    // Verify descriptor validity
    if (descriptor.dsize != LIDAR_INFO_LEN)
    {
        return -1;
    }
    if (!descriptor.is_single)
    {
        return -1;
    }
    if (descriptor.dtype != LIDAR_INFO_TYPE)
    {
        return -1;
    }

    // Read the response data
    uint8_t data[LIDAR_INFO_LEN] = {0};
    _read_response(data, descriptor.dsize);

    // Populate lidar_info_t structure with retrieved information
    info->model = data[0];
    info->firmware_minor = data[1];
    info->firmware_major = data[2];
    info->hardware = data[3];
    for (int i = 0; i < 16; i++)
    {
        info->serialnumber[i] = data[4 + i];
    }

    return 0;
}

/**
 * @brief Get the health status of the lidar.
 *
 * This function sends a command to the lidar device to retrieve its health status.
 * It reads the descriptor and checks if it is a single descriptor of the correct type and size.
 * If the descriptor is valid, it reads the response data and updates the health structure with the status and error code.
 *
 * @param health Pointer to the lidar_health_t structure to store the health status.
 * @return 0 if successful, -1 if there is an error.
 */
int lidar_get_health(lidar_health_t *health)
{
    _send_cmd(LIDAR_GET_HEALTH);
    lidar_descriptor_t descriptor = {0};
    _read_descriptor(&descriptor);
    if (descriptor.dsize != LIDAR_HEALTH_LEN)
    {
        return -1;
    }
    if (!descriptor.is_single)
    {
        return -1;
    }
    if (descriptor.dtype != LIDAR_HEALTH_TYPE)
    {
        return -1;
    }
    uint8_t data[LIDAR_HEALTH_LEN] = {0};
    _read_response(data, descriptor.dsize);
    health->status = data[0];
    health->error_code = data[1] | ((uint16_t)data[2] << 8);
    return 0;
}

/**
 * @brief Clears the input buffer of the UART.
 * 
 * @param lidar The lidar object.
 * @return 0 on success, -1 on failure.
 */
int lidar_clear_input(lidar_t *lidar)
{
    uart_flush(0);
    return 0;
}

/**
 * @brief Starts the lidar scan.
 * 
 * @param lidar The lidar object.
 * @return 0 on success, -1 on failure.
 */
int lidar_start_scan(lidar_t *lidar)
{
    if (lidar->scanning || !lidar->motor_running)
    {
        return -1;
    }

    lidar_health_t health = {0};
    lidar_get_health(&health);
    if (health.status == ERROR)
    {
        lidar_reset(lidar);
        lidar_get_health(&health);
        if (health.status == ERROR)
        {
            return -1;
        }
    }

    _send_cmd(LIDAR_SCAN);

    lidar_descriptor_t descriptor = {0};
    _read_descriptor(&descriptor);
    if (descriptor.dsize != LIDAR_SCAN_RESP_LEN)
    {
        return -1;
    }
    if (descriptor.is_single)
    {
        return -1;
    }
    if (descriptor.dtype != LIDAR_SCAN_DTYPE)
    {
        return -1;
    }
    lidar->scanning = true;
    lidar->descriptor_size = descriptor.dsize;
    return 0;
}

/**
 * @brief Starts the lidar express scan.
 * 
 * @param lidar The lidar object.
 * @return 0 on success, -1 on failure.
 */
int lidar_start_exp_scan(lidar_t *lidar)
{

    uint8_t payload[5] = {0};
    _send_payload_cmd(LIDAR_EXP_SCAN, payload, 5);

    lidar_descriptor_t descriptor = {0};
    _read_descriptor(&descriptor);
    if (descriptor.dsize != LIDAR_EXP_SCAN_RESP_LEN)
    {
        printf("Descriptor size: %d\n", descriptor.dsize);
        return -1;
    }
    if (descriptor.is_single)
    {
        printf("Not single\n");
        return -1;
    }
    if (descriptor.dtype != LIDAR_EXP_SCAN_DTYPE)
    {
        printf("Descriptor type: %d\n", descriptor.dtype);
        return -1;
    }
    lidar->scanning = true;
    lidar->descriptor_size = descriptor.dsize;
    return 0;
}

/**
 * @brief Stops the lidar scan.
 * 
 * @param lidar The lidar object.
 * @return 0 on success, -1 on failure.
 */
int lidar_stop_scan(lidar_t *lidar)
{
    _send_cmd(LIDAR_STOP);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    lidar->scanning = false;
    int ret = lidar_clear_input(lidar);
    return ret;
}

/**
 * @brief Resets the lidar sensor.
 * 
 * @param lidar The lidar object.
 * @return 0 on success, -1 on failure.
 */
int lidar_reset(lidar_t *lidar)
{
    _send_cmd(LIDAR_RESET);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    int ret = lidar_clear_input(lidar);
    return ret;
}
/**
 * @brief Prints the lidar scan data.
 * 
 * @param scan The lidar scan data.
 * @return 0 on success.
 */
int lidar_print_scan(scan_t *scan)
{
    printf("Angle: %u, Distance: %lu, Quality: %u \n", scan->angle_q6 >> 6, scan->distance_q2 >> 2, scan->quality);
    return 0;
}

/**
 * @brief Prints the lidar express scan data.
 * 
 * @param scan The lidar express scan data.
 * @return 0 on success.
 */
int lidar_print_exp_scan(express_scan_t *scan)
{
    for (int i = 0; i < 32; i++)
    {
        printf("%u : %u \n", scan->angles[i] >> 6, scan->distances[i]);
    }
    return 0;
}

/**
 * @brief Gets the lidar scan data for a 360-degree scan.
 * 
 * @param lidar The lidar object.
 * @param distances Array to store the distances.
 * @return 0 on success, -1 on failure.
 */
int lidar_get_scan_360(lidar_t *lidar, uint32_t distances[360])
{
    if (!lidar->scanning | !lidar->motor_running)
    {
        return -1;
    }

    int dsize = lidar->descriptor_size;
    uint8_t *raw = malloc(dsize);
    scan_t scan;
    uint8_t new_scan = 0;

    // Wait for new scan
    while (!new_scan)
    {
        if (_read_response(raw, dsize) != dsize)
            continue;
        _process_scan(&scan, raw, &new_scan);
    }

    // Set first distance
    uint16_t angle = scan.angle_q6 >> 6;      // Only keep integer part
    distances[angle] = scan.distance_q2 >> 2; // Only keep integer part
    new_scan = 0;

    // Read rest of scan
    while (true)
    {
        _read_response(raw, dsize);
        _process_scan(&scan, raw, &new_scan);
        if (new_scan)
            break;

        if (scan.quality > 10)
        {
            angle = scan.angle_q6 >> 6;
            distances[angle] = scan.distance_q2 >> 2;
        }
    }
    free(raw);
    return 0;
}

/**
 * @brief Get the 360-degree express scan distances from the lidar.
 *
 * This function retrieves the distances of the 360-degree express scan from the lidar.
 * The lidar must be in scanning mode and the motor must be running for the function to succeed.
 *
 * @param lidar The lidar object.
 * @param distances An array to store the distances of the express scan.
 * @return 0 if successful, -1 if there was an error.
 */
int lidar_get_exp_scan_360(lidar_t *lidar, uint16_t *distances)
{
    if (!lidar->scanning || !lidar->motor_running)
    {
        return -1;
    }

    size_t num_points = 360;
    int shift_factor = 6;
    switch (lidar->density)
    {
    case STANDARD:
        break;
    case QUARTER:
        num_points = 360 / 4;
        shift_factor = 8;
        break;
    case HALF:
        num_points = 360 / 2;
        shift_factor = 7;
        break;
    case DOUBLE:
        num_points = 360 * 2;
        shift_factor = 5;
        break;
    }

    int dsize = lidar->descriptor_size;
    uint8_t *raw = malloc(dsize);
    express_scan_raw_t scan_raw_1, scan_raw_2;
    express_scan_t scan;

    // Get first scan
    // printf("Reading first scan\n");
    int ret = _read_response(raw, dsize);
    if (ret < dsize)
    {
        printf("Error reading response, read %d bytes\n", ret);
        return -1;
    }
    _get_raw_scan_express(&scan_raw_1, raw);

    int scan_over = 0;
    uint16_t raw_angle_prev = scan_raw_1.start_angle_q6;
    while (!scan_over)
    {
        // printf("Reading scan, buffer: %d\n", uart_in_waiting(0));
        ret = _read_response(raw, dsize);
        // printf("Read: %d, expected: %d\n", uart_in_waiting(0));
        if (ret < dsize)
        {
            printf("Error reading response, read %d bytes\n", ret);
            return -1;
        }
        _get_raw_scan_express(&scan_raw_2, raw);
        _process_scan_express(&scan, &scan_raw_1, &scan_raw_2);
        scan_raw_1 = scan_raw_2;

        if (scan_raw_1.start_angle_q6 < raw_angle_prev)
        {
            scan_over = 1;
        }
        raw_angle_prev = scan_raw_1.start_angle_q6;

        uint16_t angle_prev = num_points;
        for (int j = 0; j < 32; j++)
        {
            uint16_t angle = scan.angles[j] >> shift_factor;
            uint16_t distance = scan.distances[j];
            if (angle > num_points - 1)
            {
                continue;
            }
            if (angle_prev == angle || distance == 0)
            {
                continue;
            }
            distances[angle] = distance;
            angle_prev = angle;
        }
    }

    free(raw);
    return 0;
}