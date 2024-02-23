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

#include "client.h"

#define SSID_PREFIX             "mbot-"
#define SSID_LEN                23
#define AP_PASSWORD             "i<3robots!"
#define MBOT_HOST_IP_ADDR       "192.168.4.2"

#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

#define PAIR_PRESSED_BIT        BIT0 

extern EventGroupHandle_t s_wifi_event_group;
extern EventGroupHandle_t s_pair_event_group;

void utils_init();
int find_paired_ssid(char *ssid);
void set_paired_ssid(char *ssid);
void get_sta_config(wifi_config_t *wifi_config);
void set_sta_config(wifi_config_t *wifi_config, char* ssid);
void start_wifi_event_handler();
void wait_for_pair();
int wait_for_connect(int attempts);

#endif