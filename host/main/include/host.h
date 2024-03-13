#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

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
#include "esp_timer.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "buttons.h"
#include "joystick.h"
#include "sockets.h"
#include "led.h"
#include "usb_device.h"
#include "pairing.h"
#include "serializer.h"
#include "lcm_types.h"

#include "host.h"

#define BUTTONS_UP_PIN          10                      /**< Controller button 1 (Up) pin on board (GPIO)*/
#define BUTTONS_RIGHT_PIN       9                       /**< Controller Button 2 (Right) pin on board (GPIO)*/
#define BUTTONS_DOWN_PIN        46                      /**< Controller button 3 (Down) pin on board (GPIO)*/
#define BUTTONS_LEFT_PIN        11                      /**< Controller button 4 (Left) pin on board (GPIO)*/
#define BUTTONS_JS_PIN          12                      /**< Controller button 5 (Joystick) pin on board (GPIO)*/

#define JOYSTICK_Y_PIN          4
#define JOYSTICK_X_PIN          5

#define PILOT_PIN               6
#define PAIR_PIN                17

#define LED1_PIN                15                      /**< LED 1 pin on board (GPIO)*/
#define LED2_PIN                16                      /**< LED 2 pin on board (GPIO)*/

#define PILOT_VX_SCALAR         0.5
#define PILOT_WZ_SCALAR         -1.5

#define AP_IS_HIDDEN            1
#define AP_CHANNEL              6
#define AP_MAX_CONN             3
#define AP_PORT                 8000

typedef enum {
    PILOT,
    SERIAL 
} host_state_t;

typedef enum {
    PILOT_STOP = BIT0,
    PILOT_CONFIRM = BIT1,
    SERIAL_STOP = BIT2,
    SERIAL_CONFIRM = BIT3
} control_mode_t;

#pragma pack(push, 1)
typedef struct packet_t {
    uint8_t *data;
    uint16_t len;
} packet_t;
#pragma pack(pop)

void connection_task(void *args);
void socket_task(void *args);
void serial_task(void *args);
void pilot_task(void *args);