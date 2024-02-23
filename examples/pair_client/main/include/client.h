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
#include "fram.h"
#include "joystick.h"
#include "sockets.h"

#define BUTTONS_UP_PIN          10                      /**< Controller button 1 (Up) pin on board (GPIO)*/
#define BUTTONS_RIGHT_PIN       9                       /**< Controller Button 2 (Right) pin on board (GPIO)*/
#define BUTTONS_DOWN_PIN        46                      /**< Controller button 3 (Down) pin on board (GPIO)*/
#define BUTTONS_LEFT_PIN        11                      /**< Controller button 4 (Left) pin on board (GPIO)*/
#define BUTTONS_JS_PIN          12                      /**< Controller button 5 (Joystick) pin on board (GPIO)*/

#define FRAM_SDA_PIN            3                       /**< I2C SDA pin on board (GPIO)*/
#define FRAM_SCL_PIN            2                       /**< I2C SCL pin on board (GPIO)*/

#define PILOT_PIN               6
#define PAIR_PIN                17
#define LED1_PIN                15                      /**< LED 1 pin on board (GPIO)*/
#define LED2_PIN                16                      /**< LED 2 pin on board (GPIO)*/

#endif