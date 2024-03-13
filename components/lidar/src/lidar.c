#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"

#include "uart.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "lidar.h"

/**
 * @brief Structure representing a single lidar scan data.
 */
typedef struct scan_t
{
    uint16_t angle_q6;              /**< Angle of the scan point in quarter degrees. */
    uint32_t distance_q2;           /**< Distance of the scan point in millimeters. */
    uint8_t quality;                /**< Quality of the scan point. */
}  scan_t;

/**
 * @brief Structure representing an express scan data.
 */
typedef struct express_scan_t
{
    uint16_t angles[32];            /**< Array of angles for each scan point in quarter degrees. */
    uint16_t distances[32];         /**< Array of distances for each scan point in millimeters. */
} express_scan_t;

/**
 * @brief Structure representing a cabin in an express scan.
 */
typedef struct cabin_t
{
    uint16_t distance_1;            /**< Distance of the first scan point in millimeters. */
    uint16_t distance_2;            /**< Distance of the second scan point in millimeters. */
    uint8_t d_angle_1;              /**< Difference in angle between the first and second scan points. */
    uint8_t d_angle_2;              /**< Difference in angle between the second and third scan points. */
} cabin_t;

/**
 * @brief Structure representing raw express scan data.
 */
typedef struct express_scan_raw_t
{
    uint16_t start_angle_q6;        /**< Starting angle of the express scan in quarter degrees. */
    cabin_t cabins[16];             /**< Array of cabins in the express scan. */
} express_scan_raw_t;

/**
 * @brief Structure representing a lidar device.
 */
struct lidar_t 
{
    uint8_t _motor_running;          /**< Flag indicating if the motor is running. */
    uint8_t _scanning;               /**< Flag indicating if the lidar is currently scanning. */
    uint8_t _descriptor_size;        /**< Size of the lidar descriptor. */
    uint8_t _rx_pin;                 /**< Pin number for receiving data. */
    uint8_t _tx_pin;                 /**< Pin number for transmitting data. */
    uint8_t _motor_pin;              /**< Pin number for controlling the motor. */
    uint8_t _port_num;               /**< Port number of the lidar device. */
    uint8_t _first_scan;             /**< Flag indicating if the first scan has been read. */
    lidar_scan_density_t _density;        /**< Density of lidar scan points. */
    lidar_scan_type_t _scan_type;        /**< Type of lidar scan. */
    scan_t _last_standard_scan;     /**< Last standard scan data. */
    express_scan_raw_t _last_exp_raw_scan; /**< Last raw express scan data. */
};

/**
 * @brief Calculates the difference between two angles.
 *
 * This function calculates the difference between two angles in degrees.
 *
 * @param w1 First angle in degrees.
 * @param w2 Second angle in degrees.
 * @return Returns the difference between the two angles.
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
 * @brief Processes a scan and extracts relevant information.
 *
 * This function processes a scan represented by the `raw` data and extracts the new scan flag, quality, angle, and distance values.
 *
 * @param scan Pointer to the `scan_t` structure to store the processed scan information.
 * @param raw Pointer to the raw scan data.
 * @param new_scan Pointer to a variable to store the new scan flag.
 * @return Returns 0 if the scan is processed successfully, nonzero otherwise.
 */
uint8_t _process_scan(scan_t *scan, uint8_t *raw, uint8_t *new_scan)
{
    // Process new scan flag
    *new_scan = raw[0] & 0b1;                        // bit 0
    uint8_t inversed_new_scan = (raw[0] >> 1) & 0b1; // bit 1

    // Check if S == S`
    if (*new_scan == inversed_new_scan)
        return 1;

    // Check if C == 1
    uint8_t check_bit = raw[1] & 0b1; // bit 8
    if (check_bit != 1)
        return 1;
    
    // Extract quality
    scan->quality = raw[0] >> 2;

    // Extract angle and distance
    scan->angle_q6 = (raw[1] >> 1) | ((uint16_t)raw[2] << 7);
    scan->distance_q2 = raw[3] | ((uint32_t)raw[4] << 8);
    return 0;
}

/**
 * @brief Processes an express scan and extracts relevant information.
 *
 * This function processes an express scan represented by the `raw` data and extracts the start angle, distance, and angle difference values for each cabin.
 *
 * @param scan Pointer to the `express_scan_raw_t` structure to store the processed express scan information.
 * @param raw Pointer to the raw express scan data.
 * @return Returns 0 if the express scan is processed successfully, nonzero otherwise.
 */
uint8_t _get_raw_scan_express(express_scan_raw_t *scan, uint8_t *raw)
{
    // Parse metadata
    uint8_t sync_1 = raw[0] >> 4;
    uint8_t sync_2 = raw[1] >> 4;
    if (sync_1 != LIDAR_EXP_SYNC1 || sync_2 != LIDAR_EXP_SYNC2)
    {
        return 1;
    }

    // Calculate checksum
    uint8_t checksum = ((raw[1] & 0x0F) << 4) | (raw[0] & 0x0F);

    // Extract start angle
    scan->start_angle_q6 = (((uint16_t)(raw[3] & 0x7F)) << 8) | raw[2];

    // Parse cabins
    for (int i = 0; i < 16; ++i)
    {
        int index = 4 + 5 * i;

        // Extract distance values
        scan->cabins[i].distance_1 = (((uint16_t)raw[index + 1]) << 6) | (raw[index] >> 2);
        scan->cabins[i].distance_2 = (((uint16_t)raw[index + 3]) << 6) | (raw[index + 2] >> 2);

        // Extract d_angle values
        scan->cabins[i].d_angle_1 = ((raw[index] & 0x03) << 4) | (raw[index + 4] & 0x0F);
        scan->cabins[i].d_angle_2 = ((raw[index + 2] & 0x03) << 4) | (raw[index + 4] >> 4);
    }
    return 0;
}

/**
 * @brief Processes an express scan and extracts relevant information.
 *
 * This function processes an express scan represented by the `last_raw_scan` and `curr_raw_scan` data and extracts the angles and distances for each cabin.
 *
 * @param out_scan Pointer to the `express_scan_t` structure to store the processed express scan information.
 * @param last_raw_scan Pointer to the first raw express scan data.
 * @param curr_raw_scan Pointer to the second raw express scan data.
 * @return Returns 0 if the express scan is processed successfully, nonzero otherwise.
 */
uint8_t _process_scan_express(express_scan_t *out_scan, const express_scan_raw_t *last_raw_scan, const express_scan_raw_t *curr_raw_scan)
{
    if (out_scan == NULL || last_raw_scan == NULL || curr_raw_scan == NULL)
    {
        return 1;
    }

    for (int k = 0; k < 32; k += 2)
    {
        cabin_t cabin = last_raw_scan->cabins[k / 2];
        uint16_t w1 = last_raw_scan->start_angle_q6;
        uint16_t w2 = curr_raw_scan->start_angle_q6;
        uint16_t d_angle = _angle_diff(w1, w2) / 32;

        out_scan->angles[k] = w1 + d_angle * k - cabin.d_angle_1;
        out_scan->angles[k + 1] = w1 + d_angle * (k + 1) - cabin.d_angle_2;
        out_scan->distances[k] = cabin.distance_1;
        out_scan->distances[k + 1] = cabin.distance_2;
    }
    return 0;
}

/**
 * @brief Sends a command over UART.
 *
 * This function sends a command over UART. It creates a request buffer, sets the sync byte and command byte, and sends the request buffer over UART.
 *
 * @param cmd The command byte.
 */
void _send_cmd(lidar_t *lidar, uint8_t cmd)
{
    uint8_t req[2];
    req[0] = LIDAR_SYNC;
    req[1] = cmd;
    uart_write(lidar->_port_num, (const char *)req, 2);
}

/**
 * @brief Sends a command with payload over UART.
 *
 * This function sends a command with payload over UART. It creates a request buffer, sets the sync byte, command byte, payload length, copies the payload data, calculates the checksum, and sends the request buffer over UART.
 *
 * @param cmd The command byte.
 * @param payload Pointer to the payload data.
 * @param payload_len Length of the payload data.
 */
void _send_payload_cmd(lidar_t *lidar, uint8_t cmd, uint8_t *payload, int payload_len)
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
    uart_write(lidar->_port_num, (const char *)req, 4 + payload_len);
}

/**
 * @brief Reads the lidar descriptor.
 *
 * This function reads the lidar descriptor from the UART and stores it in the `lidar_descriptor_t` structure.
 *
 * @param descriptor Pointer to the `lidar_descriptor_t` structure to store the lidar descriptor.
 * @return Returns the number of bytes read if successful, -1 otherwise.
 */
int _read_descriptor(lidar_t *lidar, lidar_descriptor_t *descriptor)
{
    uint8_t descriptor_bytes[LIDAR_DESCRIPTOR_LEN] = {0};
    int bytes_read = uart_read_bytes(lidar->_port_num, descriptor_bytes, LIDAR_DESCRIPTOR_LEN, portMAX_DELAY);
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
 * @brief Reads the lidar response.
 *
 * This function reads the lidar response from the UART and stores it in the `data` buffer.
 *
 * @param data Pointer to the buffer to store the lidar response.
 * @param dsize Size of the lidar response.
 * @return Returns the number of bytes read if successful, -1 otherwise.
 */
int _read_response(lidar_t *lidar, uint8_t *data, int dsize)
{
    return uart_read(lidar->_port_num, (char *)data, dsize, 5000);
}


/**
 * Starts a standard scan on the lidar device.
 *
 * @param lidar A pointer to the lidar_t structure representing the lidar device.
 * @return 0 if the scan was started successfully, 1 otherwise.
 */
uint8_t _start_standard_scan(lidar_t *lidar) {
    _send_cmd(lidar, LIDAR_SCAN);

    lidar_descriptor_t descriptor = {0};
    int lidar_err = _read_descriptor(lidar, &descriptor);
    if (lidar_err < 0)
    {
        ESP_LOGE("LIDAR_START_SCAN", "Failed to read scan descriptor");
        return 1;
    }

    if (descriptor.dsize != LIDAR_SCAN_RESP_LEN)
    {
        ESP_LOGE("LIDAR_START_SCAN", "Invalid scan descriptor, size: %d\n", descriptor.dsize);
        return 1;
    }
    if (descriptor.is_single)
    {
        ESP_LOGE("LIDAR_START_SCAN", "Invalid scan descriptor, not single");
        return 1;
    }
    if (descriptor.dtype != LIDAR_SCAN_DTYPE)
    {
        ESP_LOGE("LIDAR_START_SCAN", "Invalid scan descriptor, type: %d\n", descriptor.dtype);
        return 1;
    }

    lidar->_scanning = true;
    lidar->_descriptor_size = descriptor.dsize;
    return 0;
}

/**
 * @brief Starts an express scan on the lidar.
 *
 * This function sends a payload command to start an express scan on the lidar.
 * It then reads the descriptor response to verify the scan settings.
 * If the descriptor is valid, the lidar scanning flag is set to true and the descriptor size is stored.
 *
 * @param lidar Pointer to the lidar object.
 * @return Returns 0 on success, 1 on failure.
 */
uint8_t _start_express_scan(lidar_t *lidar)
{
    uint8_t payload[5] = {0};
    _send_payload_cmd(lidar, LIDAR_EXP_SCAN, payload, 5);

    lidar_descriptor_t descriptor = {0};
    int lidar_err = _read_descriptor(lidar, &descriptor);
    if (lidar_err < 0)
    {
        ESP_LOGE("LIDAR_START_EXP", "Failed to read express scan descriptor");
        return 1;
    }

    if (descriptor.dsize != LIDAR_EXP_SCAN_RESP_LEN)
    {
        ESP_LOGE("LIDAR_START_EXP", "Invalid express scan descriptor, size: %d\n", descriptor.dsize);
        return 1;
    }
    if (descriptor.is_single)
    {
        ESP_LOGE("LIDAR_START_EXP", "Invalid express scan descriptor, not single");
        return 1;
    }
    if (descriptor.dtype != LIDAR_EXP_SCAN_DTYPE)
    {
        ESP_LOGE("LIDAR_START_EXP", "Invalid express scan descriptor, type: %d\n", descriptor.dtype);
        return 1;
    }
    
    lidar->_scanning = true;
    lidar->_descriptor_size = descriptor.dsize;
    return 0;
}

uint8_t _get_standard_scan_360(lidar_t *lidar, uint16_t distances[360])
{
    if (lidar_get_scan_type(lidar) != SCAN_TYPE_STANDARD)
    {
        ESP_LOGE("LIDAR", "Invalid scan type");
        return 1;
    }

    size_t dsize = lidar->_descriptor_size;
    if (dsize != LIDAR_SCAN_RESP_LEN)
    {
        ESP_LOGE("LIDAR", "Invalid descriptor size: %d", dsize);
        return 1;
    }

    uint8_t *raw = (uint8_t*)malloc(dsize);
    if (raw == NULL)
    {
        ESP_LOGE("LIDAR", "Failed to allocate memory for raw scan data");
        return 1;
    }

    uint32_t num_points = 360;
    uint8_t shift_factor = 6;
    switch (lidar_get_scan_density(lidar))
    {
    case DENSITY_STANDARD:
        break;
    case DENSITY_QUARTER:
        num_points /= 4;
        shift_factor = 8;
        break;
    case DENSITY_HALF:
        num_points /= 2;
        shift_factor = 7;
        break;
    case DENSITY_DOUBLE:
        num_points *= 2;
        shift_factor = 5;
        break;
    default:
        ESP_LOGE("LIDAR", "Invalid scan density");
        return 1;
    }
    
    scan_t scan;
    uint8_t new_scan = 0;

    // Wait for new scan
    if (lidar->_first_scan) {
        while (!new_scan)
        {
            int bytes_read = _read_response(lidar, raw, dsize);
            if (bytes_read < 0) {
                ESP_LOGE("LIDAR", "Error reading response during standard scan");
                return 1;
            }
            else if (bytes_read != dsize) {
                ESP_LOGE("LIDAR", "Error reading response, read %d bytes\n", bytes_read);
                return 1;
            }
            _process_scan(&scan, raw, &new_scan);
        }
        lidar->_first_scan = 0;
        new_scan = 0;
    }
    else {
        scan = lidar->_last_standard_scan;
    }

    // Set first distance
    uint16_t angle = scan.angle_q6 >> 6;      // Only keep integer part
    distances[angle] = scan.distance_q2 >> 2; // Only keep integer part

    // Read rest of scan
    while (true)
    {
       int bytes_read = _read_response(lidar, raw, dsize);
       if (bytes_read < 0) {
            ESP_LOGE("LIDAR", "Error reading response during standard scan");
            return 1;
        }
        else if (bytes_read != dsize) {
            ESP_LOGE("LIDAR", "Error reading response, read %d bytes\n", bytes_read);
            return 1;
        }

        _process_scan(&scan, raw, &new_scan);
        if (new_scan) {
            lidar->_last_standard_scan = scan;
            break;
        }

        uint16_t angle_prev = num_points; // Set to value outside of range
        if (scan.quality > 10)
        {
            angle = scan.angle_q6 >> shift_factor;
            if (angle == angle_prev || angle > num_points - 1)
            {
                continue;
            }
            distances[angle] = scan.distance_q2 >> 2;
            angle_prev = angle;
        }
    }
    free(raw);
    return 0;
}

int _get_express_scan_360(lidar_t *lidar, uint16_t *distances)
{
    if (lidar_get_scan_type(lidar) != SCAN_TYPE_EXPRESS)
    {
        ESP_LOGE("LIDAR", "Invalid scan type");
        return 1;
    }

    // int bytes_waiting = uart_in_waiting(UART_NUM_0);
    // if (bytes_waiting > LIDAR_BUFFER_SIZE) {
    //     ESP_LOGW("LIDAR", "Buffer overflow, clearing input");
    //     lidar_stop_scan(lidar);
    //     lidar_start_exp_scan(lidar);
    // }

    size_t dsize = lidar->_descriptor_size;
    if (dsize != LIDAR_EXP_SCAN_RESP_LEN)
    {
        ESP_LOGE("LIDAR", "Invalid descriptor size: %d", dsize);
        return 1;
    }

    uint8_t *raw = (uint8_t*)malloc(dsize);
    if (raw == NULL)
    {
        ESP_LOGE("LIDAR", "Failed to allocate memory for raw scan data");
        return 1;
    }

    uint32_t num_points = 360;
    uint8_t shift_factor = 6;
    switch (lidar_get_scan_density(lidar))
    {
    case DENSITY_STANDARD:
        break;
    case DENSITY_QUARTER:
        num_points /= 4;
        shift_factor = 8;
        break;
    case DENSITY_HALF:
        num_points /= 2;
        shift_factor = 7;
        break;
    case DENSITY_DOUBLE:
        num_points *= 2;
        shift_factor = 5;
        break;
    default:
        ESP_LOGE("LIDAR", "Invalid scan density");
        return 1;
    }

    express_scan_raw_t curr_exp_raw_scan;
    express_scan_t scan;

    // Get first scan
    // printf("Reading first scan\n");

    if (lidar->_first_scan) {
        int bytes_read = _read_response(lidar, raw, dsize);
        if (bytes_read < 0) {
            ESP_LOGE("LIDAR", "Error reading response during express scan");
            return 1;
        }
        else if (bytes_read != dsize) {
            ESP_LOGE("LIDAR", "Error reading response, read %d bytes\n", bytes_read);
            return 1;
        }
        _get_raw_scan_express(&lidar->_last_exp_raw_scan, raw);
        lidar->_first_scan = 0;
    }

    int scan_over = 0;
    uint16_t raw_angle_prev = lidar->_last_exp_raw_scan.start_angle_q6;
    while (!scan_over)
    {
        int bytes_read = _read_response(lidar, raw, dsize);
        if (bytes_read < 0) {
            ESP_LOGE("LIDAR", "Error reading response during express scan");
            return 1;
        }
        else if (bytes_read != dsize) {
            ESP_LOGE("LIDAR", "Error reading response, read %d bytes\n", bytes_read);
            return 1;
        }

        uint8_t lidar_err = _get_raw_scan_express(&curr_exp_raw_scan, raw);
        if (lidar_err) {
            ESP_LOGW("LIDAR", "Received invalid express scan data");
            continue;
        }
        
        _process_scan_express(&scan, &lidar->_last_exp_raw_scan, &curr_exp_raw_scan);
        lidar->_last_exp_raw_scan = curr_exp_raw_scan;

        if (lidar->_last_exp_raw_scan.start_angle_q6 < raw_angle_prev)
        {
            scan_over = 1;
        }
        raw_angle_prev = lidar->_last_exp_raw_scan.start_angle_q6;

        uint16_t angle_prev = num_points; // Set to value outside of range
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

/**< Public function definitions */

lidar_t *lidar_create(uint8_t port, uint8_t rx_pin, uint8_t tx_pin, uint8_t motor_pin)
{
    lidar_t *lidar = (lidar_t*)malloc(sizeof(lidar_t));
    if (lidar == NULL)
    {
        ESP_LOGE("LIDAR", "Failed to allocate memory for lidar_t");
        return NULL;
    }

    lidar->_motor_running = false;
    lidar->_density = DENSITY_STANDARD;
    lidar->_scanning = false;
    lidar->_port_num = port;
    lidar->_rx_pin = rx_pin;
    lidar->_tx_pin = tx_pin;
    lidar->_motor_pin = motor_pin;

    uart_init(lidar->_port_num, lidar->_rx_pin, lidar->_tx_pin, LIDAR_BAUDRATE, LIDAR_BUFFER_SIZE);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 500, // Set output frequency at 500 Hz
        .clk_cfg = LEDC_AUTO_CLK};
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK)
    {
        ESP_LOGE("LIDAR", "Failed to configure LEDC timer");
        free(lidar);
        return NULL;
    }

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = lidar->_motor_pin,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK)
    {
        ESP_LOGE("LIDAR", "Failed to configure LEDC channel");
        free(lidar);
        return NULL;
    }

    uint8_t lidar_err = lidar_start_motor(lidar, LIDAR_DEFAULT_MOTOR_PWM);
    if (lidar_err)
    {
        ESP_LOGE("LIDAR", "Failed to start motor");
        free(lidar);
        return NULL;
    }

    lidar_health_t health;
    lidar_err = lidar_get_health(lidar, &health);
    if (lidar_err || health.status != LIDAR_HEALTH_GOOD)
    {
        lidar_reset(lidar);
        lidar_err = lidar_get_health(lidar, &health);
        if (lidar_err || health.status != LIDAR_HEALTH_GOOD)
        {
            ESP_LOGE("LIDAR", "Failed to initialize lidar, health status: %d", health.status);
            free(lidar);
            return NULL;
        }
    }

    lidar_info_t info;
    lidar_err = lidar_get_info(lidar, &info);
    if (lidar_err) {
        ESP_LOGE("LIDAR", "Failed to retrieve lidar information");
        free(lidar);
        return NULL;
    }

    return lidar;
}

void lidar_free(lidar_t *lidar)
{
    if (lidar_is_scanning(lidar))
    {
        lidar_stop_scan(lidar);
    }
    lidar_reset(lidar);
    uart_deinit(lidar->_port_num);

    if (lidar_is_motor_running(lidar))
    {
        lidar_stop_motor(lidar);
    }
    free(lidar);
}

uint8_t lidar_get_port(lidar_t *lidar)
{
    return lidar->_port_num;
}

uint8_t lidar_get_rx_pin(lidar_t *lidar)
{
    return lidar->_rx_pin;
}

uint8_t lidar_get_tx_pin(lidar_t *lidar)
{
    return lidar->_tx_pin;
}

uint8_t lidar_get_motor_pin(lidar_t *lidar)
{
    return lidar->_motor_pin;
}

void lidar_set_scan_density(lidar_t *lidar, lidar_scan_density_t density)
{
    lidar->_density = density;
}

lidar_scan_density_t lidar_get_scan_density(lidar_t *lidar)
{
    return lidar->_density;
}

uint8_t lidar_is_scanning(lidar_t *lidar)
{
    return lidar->_scanning;
}

uint8_t lidar_is_motor_running(lidar_t *lidar)
{
    return lidar->_motor_running;
}

lidar_scan_type_t lidar_get_scan_type(lidar_t *lidar)
{
    return lidar->_scan_type;
}

uint8_t lidar_start_motor(lidar_t *lidar, float duty_cycle)
{
    if (lidar->_motor_running)
    {
        return 0;
    }

    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (uint32_t)(duty_cycle * LIDAR_MAX_MOTOR_PWM));
    if (err != ESP_OK)
    {
        ESP_LOGE("LIDAR", "Failed to set LEDC duty");
        return 1;
    }

    err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (err != ESP_OK)
    {
        ESP_LOGE("LIDAR", "Failed to update LEDC duty");
        return 1;
    }

    lidar->_motor_running = true;
    return 0;
}

void lidar_stop_motor(lidar_t *lidar)
{
    if (!lidar->_motor_running)
    {
        return;
    }
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE("LIDAR", "Failed to set LEDC duty");
        return;
    }

    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (err != ESP_OK)
    {
        ESP_LOGE("LIDAR", "Failed to update LEDC duty");
        return;
    }
    
    lidar->_motor_running = false;
}

uint8_t lidar_get_info(lidar_t *lidar, lidar_info_t *info)
{
    // Send command to retrieve lidar information
    _send_cmd(lidar, LIDAR_GET_INFO);

    // Read the descriptor
    lidar_descriptor_t descriptor = {0};
    _read_descriptor(lidar, &descriptor);

    // Verify descriptor validity
    if (descriptor.dsize != LIDAR_INFO_LEN)
    {
        ESP_LOGE("LIDAR_GET_INFO", "Invalid info descriptor, size: %d\n", descriptor.dsize);
        return 1;
    }
    if (!descriptor.is_single)
    {
        ESP_LOGE("LIDAR_GET_INFO", "Invalid info descriptor, not single");
        return 1;
    }
    if (descriptor.dtype != LIDAR_INFO_TYPE)
    {
        ESP_LOGE("LIDAR_GET_INFO", "Invalid info descriptor, type: %d\n", descriptor.dtype);
        return 1;
    }

    // Read the response data
    uint8_t data[LIDAR_INFO_LEN] = {0};
    int lidar_err = _read_response(lidar, data, descriptor.dsize);
    if (lidar_err < 0)
    {
        ESP_LOGE("LIDAR_GET_INFO", "Failed to read info response descriptor");
        return 1;
    }

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

uint8_t lidar_get_health(lidar_t *lidar, lidar_health_t *health)
{
    _send_cmd(lidar, LIDAR_GET_HEALTH);

    lidar_descriptor_t descriptor = {0};
    int lidar_err = _read_descriptor(lidar, &descriptor);
    if (lidar_err < 0)
    {
        ESP_LOGE("LIDAR_GET_HEALTH", "Failed to read health descriptor, %d", lidar_err);
        return 1;
    }

    if (descriptor.dsize != LIDAR_HEALTH_LEN)
    {
        ESP_LOGE("LIDAR_GET_HEALTH", "Invalid health descriptor, size: %d\n", descriptor.dsize);
        return 1;
    }
    if (!descriptor.is_single)
    {
        ESP_LOGE("LIDAR_GET_HEALTH", "Invalid health descriptor, not single");
        return 1;
    }
    if (descriptor.dtype != LIDAR_HEALTH_TYPE)
    {
        ESP_LOGE("LIDAR_GET_HEALTH", "Invalid health descriptor, type: %d\n", descriptor.dtype);
        return 1;
    }

    uint8_t data[LIDAR_HEALTH_LEN] = {0};
    lidar_err = _read_response(lidar, data, descriptor.dsize);
    if (lidar_err < 0)
    {
        ESP_LOGE("LIDAR_GET_HEALTH", "Failed to read health response descriptor");
        return 1;
    }

    health->status = data[0];
    health->error_code = data[1] | ((uint16_t)data[2] << 8);
    return 0;
}

void lidar_reset(lidar_t *lidar)
{
    _send_cmd(lidar, LIDAR_RESET);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    lidar_clear_input(lidar);
}

void lidar_clear_input(lidar_t *lidar)
{
    uart_flush_buffer(lidar->_port_num);
}

uint8_t lidar_start_scan(lidar_t *lidar, lidar_scan_type_t type)
{
    if (lidar->_scanning)
    {
        ESP_LOGW("LIDAR", "Lidar is already scanning");
        return 0;
    }

    if (!lidar->_motor_running)
    {
        ESP_LOGE("LIDAR", "Lidar motor is not running");
        return 1;
    }

    lidar_health_t health;
    uint8_t lidar_err = lidar_get_health(lidar, &health);
    if (lidar_err || health.status != LIDAR_HEALTH_GOOD)
    {
        lidar_reset(lidar);
        lidar_err = lidar_get_health(lidar, &health);
        if (lidar_err || health.status != LIDAR_HEALTH_GOOD)
        {
            ESP_LOGE("LIDAR", "Failed to initialize lidar, health status: %d", health.status);
            return 1;
        }
    }

    lidar->_scan_type = type;
    switch (type)
    {
        case SCAN_TYPE_STANDARD:
            return _start_standard_scan(lidar);
        case SCAN_TYPE_EXPRESS:
            return _start_express_scan(lidar);
        default:
            ESP_LOGE("LIDAR_START_SCAN", "Invalid scan type: %d", type);
            return 1;
    }
}

void lidar_stop_scan(lidar_t *lidar)
{
    _send_cmd(lidar, LIDAR_STOP);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    lidar_clear_input(lidar);
    lidar->_first_scan = 1;
    lidar->_scanning = false;
}

uint8_t lidar_get_scan_360(lidar_t *lidar, uint16_t distances[360]) {
    if (!lidar_is_scanning(lidar))
    {
        ESP_LOGE("LIDAR", "Lidar is not scanning");
        return 1;
    }
    if (!lidar_is_motor_running(lidar))
    {
        ESP_LOGE("LIDAR", "Lidar motor is not running");
        return 1;
    }
    switch (lidar_get_scan_type(lidar))
    {
        case SCAN_TYPE_STANDARD:
            return _get_standard_scan_360(lidar, distances);
        case SCAN_TYPE_EXPRESS:
            return _get_express_scan_360(lidar, distances);
        default:
            ESP_LOGE("LIDAR_GET_SCAN", "Invalid scan type: %d", lidar_get_scan_type(lidar));
            return 1;
    }
}