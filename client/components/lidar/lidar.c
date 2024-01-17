#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"

#include "lidar.h"

int _process_scan(scan_t *scan, uint8_t *raw, uint8_t *new_scan)
{
    *new_scan = raw[0] & 0b1; // bit 0
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

void _send_payload_cmd(uint8_t cmd, uint8_t *payload, int payload_len)
{
    uint8_t req[2 + 1 + payload_len + 1];
    req[0] = LIDAR_SYNC;
    req[1] = cmd;
    req[2] = payload_len;
    for (int i = 0; i < payload_len; i++)
    {
        req[3 + i] = payload[i];
    }
    uint8_t checksum = 0;
    for (int i = 0; i < 3 + payload_len; i++)
    {
        checksum ^= req[i];
    }
    req[3 + payload_len] = checksum;
    // TODO: Replace with ESP32 UART API
    // uart_write_blocking(uart0, req, 3 + payload_len + 1);
}

void _send_cmd(uint8_t cmd)
{
    uint8_t req[2];
    req[0] = LIDAR_SYNC;
    req[1] = cmd;
    // TODO: Replace with ESP32 UART API
    // uart_write_blocking(uart0, req, 2);
}

int _read_descriptor(lidar_descriptor_t *descriptor)
{
    uint8_t descriptor_bytes[LIDAR_DESCRIPTOR_LEN] = {0};
    // TODO: Replace with ESP32 UART API
    // uart_read_blocking(uart0, descriptor_bytes, DESCRIPTOR_LEN);
    if (descriptor_bytes[0] != LIDAR_SYNC || descriptor_bytes[1] != LIDAR_SYNC_INV)
    {
        return -1;
    }
    descriptor->dsize = descriptor_bytes[2];
    descriptor->is_single = descriptor_bytes[5] == 0;
    descriptor->dtype = descriptor_bytes[6];
    return 0;
}

void _read_response(uint8_t *data, int dsize)
{
    // TODO: Replace with ESP32 UART API
    // uart_read_blocking(uart0, data, dsize);
}

/* Public functions */

void lidar_init(lidar_t *lidar)
{
    lidar->motor_running = false;

    lidar_connect(lidar);
    lidar_start_motor(lidar);
}

void lidar_connect()
{
    // TODO: Replace with ESP32 UART API
    // uart_init(uart0, LIDAR_BAUDRATE);
    // gpio_set_function(LIDAR_TX_PIN, GPIO_FUNC_UART);
    // gpio_set_function(LIDAR_RX_PIN, GPIO_FUNC_UART);
}

void lidar_disconnect()
{
    // TODO: Replace with ESP32 UART API
    // uart_deinit(uart0);
}

void lidar_start_motor(lidar_t *lidar)
{
    if (lidar->motor_running)
    {
        return;
    }
    // TODO: Replace with ESP32 API
    // mbot_motor_init(2);
    // mbot_motor_set_duty(2, 0.5); // Need to make sure this is correct PWM
    lidar->motor_running = true;
}

void lidar_stop_motor(lidar_t *lidar)
{
    if (!lidar->motor_running)
    {
        return;
    }
    // TODO: Replace with ESP32 API
    // mbot_motor_set_duty(2, 0.0);
    lidar->motor_running = false;
}

int lidar_get_info(lidar_info_t *info)
{
    _send_cmd(LIDAR_GET_INFO);
    lidar_descriptor_t descriptor = {0};
    _read_descriptor(&descriptor);
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
    uint8_t data[LIDAR_INFO_LEN] = {0};
    _read_response(data, descriptor.dsize);
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

int lidar_clear_input(lidar_t *lidar)
{
    if (lidar->scanning)
    {
        return -1;
    }
    // TODO: Replace with ESP32 UART API
    // Flush uart input buffer
    // while (uart_is_readable(uart0))
    // {
    //     uart_getc(uart0);
    // }
    return 0;
}

int lidar_start(lidar_t *lidar)
{
    if (lidar->scanning)
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
    if (descriptor.dsize != LIDAR_SCAN_RESPONSE_LEN)
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

int lidar_stop(lidar_t *lidar)
{
    _send_cmd(LIDAR_STOP);
    // TODO: Replace with ESP32 API
    // sleep_ms(1);
    lidar->scanning = false;
    int ret = lidar_clear_input(lidar);
    return ret;
}

int lidar_reset(lidar_t *lidar)
{
    _send_cmd(LIDAR_RESET);
    // TODO: Replace with ESP32 API
    // sleep_ms(2);
    int ret = lidar_clear_input(lidar);
    return ret;
}

// Updates distances (in millimeters) with a 0-360 degree scan. If a distance = 0, then it is invalid.
void lidar_get_scan_360(lidar_t *lidar, uint32_t distances[360])
{
    lidar_start_motor(lidar);
    if (!lidar->scanning)
    {
        lidar_start(lidar);
    }

    int dsize = lidar->descriptor_size;
    // TODO: Replace with ESP32 UART API
    // int data_in_buf = uart_get_hw(uart0)->dr;
    // if (data_in_buf > LIDAR_MAX_BUF_MEAS * dsize)
    // {
    //     uart_read_blocking(uart0, NULL, data_in_buf / (dsize * dsize));
    // }

    uint8_t raw[dsize];
    scan_t scan;
    uint8_t new_scan = 0;

    // Wait for new scan
    while (!new_scan)
    {
        _read_response(raw, dsize);
        _process_scan(&scan, raw, &new_scan);
    }

    // Set first distance
    uint16_t angle = scan.angle_q6 >> 6; // Only keep integer part
    distances[angle] = scan.distance_q2 >> 2; // Only keep integer part

    // Read rest of scan
    while (!new_scan)
    {
        _read_response(raw, dsize);
        _process_scan(&scan, raw, &new_scan);
        angle = scan.angle_q6 >> 6;
        distances[angle] = scan.distance_q2 >> 2;
    }
}