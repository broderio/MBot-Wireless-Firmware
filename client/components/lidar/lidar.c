#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uart.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

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
    uart_write(0, (const char*)req, 4 + payload_len);
}

void _send_cmd(uint8_t cmd)
{
    uint8_t req[2];
    req[0] = LIDAR_SYNC;
    req[1] = cmd;
    uart_write(0, (const char*)req, 2);
}

int _read_descriptor(lidar_descriptor_t *descriptor)
{
    uint8_t descriptor_bytes[LIDAR_DESCRIPTOR_LEN] = {0};
    int bytes_read = uart_read(0, (char *)descriptor_bytes, LIDAR_DESCRIPTOR_LEN, portMAX_DELAY);
    if (descriptor_bytes[0] != LIDAR_SYNC || descriptor_bytes[1] != LIDAR_SYNC_INV)
    {
        return -1;
    }
    descriptor->dsize = descriptor_bytes[2];
    descriptor->is_single = descriptor_bytes[5] == 0;
    descriptor->dtype = descriptor_bytes[6];
    return bytes_read;
}

int _read_response(uint8_t *data, int dsize)
{
    return uart_read(0, (char*)data, dsize, 5000);
}

/* Public functions */

int lidar_init(lidar_t *lidar)
{
    lidar->motor_running = false;
    lidar->density = STANDARD;

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 500,  // Set output frequency at 500 Hz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LIDAR_PWM_PIN,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    lidar_connect(lidar);
    lidar_start_motor(lidar, LIDAR_DEFAULT_MOTOR_PWM);
    lidar->scanning = false;

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

int lidar_deinit(lidar_t *lidar)
{
    if (lidar->scanning)
    {
        lidar_stop_scan(lidar);
    }
    lidar_reset(lidar);
    lidar_stop_motor(lidar);
    lidar_disconnect(lidar);
    return 0;
}

void lidar_connect()
{
    uart_init(0, 115200, UART_DATA_8_BITS, UART_PARITY_DISABLE, UART_STOP_BITS_1);
}

void lidar_disconnect()
{
    uart_deinit(0);
}

void lidar_set_point_density(lidar_t *lidar, POINT_DENSITY density)
{
    lidar->density = density;
}

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
    uart_flush(0);
    return 0;
}

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

int lidar_stop_scan(lidar_t *lidar)
{
    _send_cmd(LIDAR_STOP);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    lidar->scanning = false;
    int ret = lidar_clear_input(lidar);
    return ret;
}

int lidar_reset(lidar_t *lidar)
{
    _send_cmd(LIDAR_RESET);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    int ret = lidar_clear_input(lidar);
    return ret;
}

int lidar_print_scan(scan_t *scan)
{
    printf("Angle: %u, Distance: %lu, Quality: %u \n", scan->angle_q6 >> 6, scan->distance_q2 >> 2, scan->quality);
    return 0;
}

int lidar_print_exp_scan(express_scan_t *scan)
{
    for (int i = 0; i < 32; i++)
    {
        printf("%u : %u \n", scan->angles[i] >> 6, scan->distances[i]);
    }
    return 0;
}

int lidar_get_scan_360(lidar_t *lidar, uint32_t distances[360])
{
    if (!lidar->scanning | !lidar->motor_running)
    {
        return -1;
    }

    int dsize = lidar->descriptor_size;
    uint8_t* raw = malloc(dsize);
    scan_t scan;
    uint8_t new_scan = 0;

    // Wait for new scan
    while (!new_scan)
    {
        if (_read_response(raw, dsize) != dsize) continue;
        _process_scan(&scan, raw, &new_scan);
    }

    // Set first distance
    uint16_t angle = scan.angle_q6 >> 6; // Only keep integer part
    distances[angle] = scan.distance_q2 >> 2; // Only keep integer part
    new_scan = 0;
    
    // Read rest of scan
    while (true)
    {
        _read_response(raw, dsize);
        _process_scan(&scan, raw, &new_scan);
        if (new_scan) break;

        if (scan.quality > 10)
        {
            angle = scan.angle_q6 >> 6;
            distances[angle] = scan.distance_q2 >> 2;
        }
    }
    free(raw);
    return 0;
}

int lidar_get_exp_scan_360(lidar_t *lidar, uint16_t* distances)
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
    uint8_t* raw = malloc(dsize);
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
            if (angle > num_points - 1) {
                continue;
            }
            if (angle_prev == angle || distance == 0) {
                continue;
            }
            distances[angle] = distance;
            // printf("%u : %u \n", angle, distance);
            angle_prev = angle;
        }
    }
    
    free(raw);
    return 0;
}