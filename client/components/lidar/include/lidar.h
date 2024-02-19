#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uart.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#ifndef LIDAR_H
#define LIDAR_H

#define LIDAR_TX_PIN                43                  /**< GPIO Pin for UART transmit line */
#define LIDAR_RX_PIN                44                  /**< GPIO Pin for UART receive line */
#define LIDAR_PWM_PIN               1                   /**< GPIO Pin for LIDAR PWM signal */

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
#define LIDAR_DEFAULT_MOTOR_PWM     600
#define LIDAR_MAX_BUF_MEAS          500

typedef enum {
    SCAN_TYPE_NORMAL,
    SCAN_TYPE_EXPRESS
} SCAN_TYPE;

typedef enum {
    GOOD,
    WARNING,
    ERROR
} HEALTH_STATUS;

typedef struct {
    bool motor_running;
    bool scanning;
    int descriptor_size;
} lidar_t;

typedef struct {
    uint8_t model;
    uint8_t firmware_minor;
    uint8_t firmware_major;
    uint8_t hardware;
    uint8_t serialnumber[16];
} lidar_info_t;

typedef struct {
    uint8_t status;
    uint16_t error_code;
} lidar_health_t;

typedef struct {
    uint8_t dsize;
    uint8_t is_single;
    uint8_t dtype;
} lidar_descriptor_t;

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

void lidar_init(lidar_t *lidar);
void lidar_connect();
void lidar_disconnect();
void lidar_start_motor(lidar_t *lidar);
void lidar_stop_motor(lidar_t *lidar);
int lidar_get_info(lidar_info_t *info);
int lidar_get_health(lidar_health_t *health);
int lidar_clear_input(lidar_t *lidar);
int lidar_start_scan(lidar_t *lidar);
int lidar_start_exp_scan(lidar_t *lidar);
int lidar_stop_scan(lidar_t *lidar);
int lidar_reset(lidar_t *lidar);
int lidar_get_scan(lidar_t *lidar, scan_t *scan);
int lidar_get_exp_scan(lidar_t *lidar, express_scan_t *scan, int num_scans);
int lidar_print_scan(scan_t *scan);
int lidar_print_exp_scan(express_scan_t *scan);
int lidar_get_scan_360(lidar_t *lidar, uint32_t distances[360]);
int lidar_get_exp_scan_360(lidar_t *lidar, uint16_t distances[360]);

#endif