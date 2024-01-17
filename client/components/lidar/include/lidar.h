#include <stdio.h>
#include "uart.h"

#ifndef LIDAR_H
#define LIDAR_H

#define LIDAR_TX_PIN                43                  /**< GPIO Pin for UART transmit line */
#define LIDAR_RX_PIN                44                  /**< GPIO Pin for UART receive line */
#define LIDAR_PWM_PIN               1                   /**< GPIO Pin for LIDAR PWM signal */

#define LIDAR_SYNC                  0xA5
#define LIDAR_SYNC_INV              0x5A

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

#define LIDAR_SCAN_RESPONSE_LEN     5
#define LIDAR_SCAN_DTYPE            0x81

#define LIDAR_DESCRIPTOR_LEN        7
#define LIDAR_INFO_LEN              20
#define LIDAR_HEALTH_LEN            3

#define LIDAR_INFO_TYPE             0x04
#define LIDAR_HEALTH_TYPE           0x06

#define LIDAR_LIDAR_BAUDRATE        115200
#define LIDAR_MAX_MOTOR_PWM         1023
#define LIDAR_DEFAULT_MOTOR_PWM     660
#define LIDAR_MAX_BUF_MEAS          500

typedef enum {
    SCAN_TYPE_NORMAL,
    SCAN_TYPE_FORCE,
    SCAN_TYPE_EXPRESS
} SCAN_TYPE;

typedef enum {
    GOOD,
    WARNING,
    ERROR
} HEALTH_STATUS;

typedef struct
{
    uint16_t angle_q6;
    uint32_t distance_q2;
    uint8_t quality;
}  scan_t;

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


void lidar_init(lidar_t *lidar);
void lidar_connect();
void lidar_disconnect();
void lidar_start_motor(lidar_t *lidar);
void lidar_stop_motor(lidar_t *lidar);
int lidar_get_info(lidar_info_t *info);
int lidar_get_health(lidar_health_t *health);
int lidar_clear_input(lidar_t *lidar);
int lidar_start(lidar_t *lidar);
int lidar_stop(lidar_t *lidar);
int lidar_reset(lidar_t *lidar);
void lidar_get_scan_360(lidar_t *lidar, uint32_t distances[360]);

#endif