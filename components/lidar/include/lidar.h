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
#define LIDAR_DEFAULT_MOTOR_PWM     600
#define LIDAR_BUFFER_SIZE           2048

typedef enum {
    SCAN_TYPE_NORMAL,
    SCAN_TYPE_EXPRESS
} SCAN_TYPE;

typedef enum {
    GOOD,
    WARNING,
    ERROR
} HEALTH_STATUS;

typedef enum {
    STANDARD,
    QUARTER,
    HALF,
    DOUBLE
} POINT_DENSITY;

typedef struct {
    bool motor_running;
    bool scanning;
    int descriptor_size;
    POINT_DENSITY density;
    int rx_pin;
    int tx_pin;
    int motor_pin;
    int port_num;
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

int lidar_init(lidar_t *lidar, int rx_pin, int tx_pin, int motor_pin);
int lidar_deinit(lidar_t *lidar);

void lidar_connect();
void lidar_disconnect();

void lidar_start_motor(lidar_t *lidar, int duty_cycle);
void lidar_stop_motor(lidar_t *lidar);

int lidar_get_info(lidar_info_t *info);
int lidar_get_health(lidar_health_t *health);
int lidar_reset(lidar_t *lidar);

int lidar_clear_input(lidar_t *lidar);

void lidar_set_point_density(lidar_t *lidar, POINT_DENSITY density);

int lidar_start_scan(lidar_t *lidar);
int lidar_start_exp_scan(lidar_t *lidar);
int lidar_stop_scan(lidar_t *lidar);

int lidar_get_scan_360(lidar_t *lidar, uint32_t distances[360]);
int lidar_get_exp_scan_360(lidar_t *lidar, uint16_t* distances);

#endif