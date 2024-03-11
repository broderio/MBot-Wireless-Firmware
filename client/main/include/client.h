#ifndef HOST_H
#define HOST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "nvs_flash.h"

#include "buttons.h"
#include "sockets.h"

#define CAM_MCLK_PIN                18                  /**< GPIO Pin for I2S master clock */
#define CAM_PCLK_PIN                8                   /**< GPIO Pin for I2S peripheral clock */
#define CAM_VSYNC_PIN               46                  /**< GPIO Pin for I2S vertical sync */
#define CAM_HSYNC_PIN               9                   /**< GPIO Pin for I2S horizontal sync */
#define CAM_D0_PIN                  14                  /**< GPIO Pin for I2S 0th bit data line */
#define CAM_D1_PIN                  47                  /**< GPIO Pin for I2S 1st bit data line */
#define CAM_D2_PIN                  48                  /**< GPIO Pin for I2S 2nd bit data line */
#define CAM_D3_PIN                  21                  /**< GPIO Pin for I2S 3rd bit data line */
#define CAM_D4_PIN                  13                  /**< GPIO Pin for I2S 4th bit data line */
#define CAM_D5_PIN                  12                  /**< GPIO Pin for I2S 5th bit data line */
#define CAM_D6_PIN                  11                  /**< GPIO Pin for I2S 6th bit data line */
#define CAM_D7_PIN                  10                  /**< GPIO Pin for I2S 7th bit data line */
#define CAM_SDA_PIN                 3                   /**< GPIO Pin for I2C data line */
#define CAM_SCL_PIN                 2                   /**< GPIO Pin for I2C clock line */

#define RC_RX_PIN                   4 
#define RC_TX_PIN                   5 

#define LIDAR_TX_PIN                44                  /**< GPIO Pin for UART transmit line */
#define LIDAR_RX_PIN                43                  /**< GPIO Pin for UART receive line */
#define LIDAR_PWM_PIN               1                   /**< GPIO Pin for LIDAR PWM signal */

#define PAIR_PIN                17

typedef enum {
    CONNECT = BIT0,
    DISCONNECT = BIT1
} connect_bits_t;

typedef enum {
    LIDAR_INIT = BIT0,
    LIDAR_START_SCAN = BIT1,
    LIDAR_GET_SCAN = BIT2,
    LIDAR_STOP_SCAN = BIT3,
    LIDAR_MALLOC = BIT4,
} lidar_bits_t;

typedef enum {
    CAMERA_INIT = BIT0,
    CAMERA_START = BIT1,
    CAMERA_GET = BIT2,
    CAMERA_STOP = BIT3,
    CAMERA_MALLOC = BIT4,
} camera_bits_t;

typedef enum {
    HOST,
    MBOT
} destination_t;

typedef struct packet_t {
    destination_t dest;
    uint16_t len;
    uint8_t *data;
} packet_t;

void sender_task(void *args);
void mbot_task(void *args);
void socket_task(void *args);
void lidar_task(void *args);
void camera_task(void *args);

#endif