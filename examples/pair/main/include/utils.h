#ifndef UTILS_H
#define UTILS_H

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

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "driver/gpio.h"

#define PAIR_PIN                17
#define LED1_PIN                15                      /**< LED 1 pin on board (GPIO)*/
#define LED2_PIN                16                      /**< LED 2 pin on board (GPIO)*/

#define PAIR_PRESSED_BIT            BIT0 
#define STOP_BLINKING_BIT           BIT0 
#define BLINK_CONFIRM_BIT           BIT1

#define SSID_PREFIX         "mbot-"
#define AP_PASSWORD         "i<3robots!"

#define MBOT_IP_ADDR        "192.168.4.2"
#define MBOT_NETMASK_ADDR   "255.0.0.0"
#define MBOT_GW_ADDR        "192.168.4.1"
#define MAX_CONN            3

extern int num_connections_socket;
extern int num_connections_ap;

extern EventGroupHandle_t s_pair_event_group;
extern EventGroupHandle_t s_blink_event_group;

void utils_init();

void init_sta(wifi_config_t *wifi_sta_config);
void init_ap(wifi_config_t *wifi_ap_config);

void wait_for_pair();
void wait_for_no_hosts();

void start_blink(int pin, int delay_ms);
void stop_blink();

#endif
