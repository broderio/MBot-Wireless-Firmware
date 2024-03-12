/**
 * @file pairing.h
 * @brief This file contains functions for initializing and managing WiFi connections in station and access point modes.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

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
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "usb_device.h"

#define MBOT_IP_ADDR        "192.168.4.2"
#define MBOT_NETMASK_ADDR   "255.0.0.0"
#define MBOT_GW_ADDR        "192.168.4.1"

#define DEFAULT_PAIR_CFG            \
        {                           \
            .ssid = "mBot",         \
            .password = "I<3robots" \
        }

typedef struct pair_config_t {
    char ssid[9]; /*< Maximum unique name is 8 characters + null terminator */
    char password[16];
} pair_config_t;

pair_config_t get_pair_config();

void set_pair_config(pair_config_t *pair_cfg);

/**
 * @brief Initializes ESP WiFi into station mode.
 * @return A pointer to the wifi_config_t structure containing the station configuration.
 */
wifi_config_t* station_init();

/**
 * @brief Deinitializes ESP WiFi.
 * @param wifi_sta_cfg A pointer to the wifi_config_t structure containing the station configuration.
 */
void station_deinit(wifi_config_t* wifi_sta_cfg);

/**
 * @brief Sets the maximum number of connection attempts. 
 * @param attempts The maximum number of connection attempts. (-1 for infinite attempts)
 */
void station_set_max_conn_attempts(int16_t attempts);

/**
 * @brief Returns the maximum number of connection attempts.
 * @return The maximum number of connection attempts.
 */
int16_t station_get_max_conn_attempts();

/**
 * @brief Scans for WiFi access points.
 * @param wifi_sta_cfg A pointer to the wifi_config_t structure containing the station configuration.
 * @param num_ap_records A pointer to the variable that will store the number of access points found.
 * @param show_hidden A flag indicating whether to show hidden access points.
 * @return An array of wifi_ap_record_t structures containing the information of the found access points.
 */
wifi_ap_record_t *station_scan(wifi_config_t* wifi_sta_cfg, uint16_t* num_ap_records, uint8_t show_hidden);

/**
 * @brief Frees the memory allocated for the scan.
 * @param ap_records An array of wifi_ap_record_t structures.
 */
void station_free_scan(wifi_ap_record_t *ap_records);

/**
 * @brief Connects to a WiFi access point.
 * @param wifi_sta_cfg A pointer to the wifi_config_t structure containing the station configuration.
 * @param ssid The SSID of the access point to connect to.
 * @param password The password of the access point.
 * @param channel The channel of the access point.
 */
void station_connect(wifi_config_t *wifi_sta_cfg, const char *ssid, const char *password);

/**
 * @brief Disconnects from a WiFi access point.
 */
void station_disconnect();

/**
 * @brief Checks if the station is currently connected.
 *
 * @return 1 if the station is connected, 0 otherwise.
 */
uint8_t station_is_connected();

/**
 * @brief Checks if the station is currently disconnected.
 *
 * @return 1 if the station is disconnected, 0 otherwise.
 */
uint8_t station_is_disconnected();

/**
 * @brief Checks if the station is currently connecting.
 *
 * @return 1 if the station is connecting, 0 otherwise.
 */
uint8_t station_is_connecting();

/**
 * @brief Checks if the station connection has failed.
 *
 * @return 1 if the station connection has failed, 0 otherwise.
 */
uint8_t station_connection_failed();

/**
 * @brief Initializes ESP WiFi into access point mode.
 * @param ssid The SSID of the access point.
 * @param password The password of the access point.
 * @param hidden A flag indicating whether to hide the SSID of the access point.
 * @return A pointer to the wifi_config_t structure containing the access point configuration.
 */
wifi_config_t* access_point_init(const char *ssid, const char *password, uint8_t channel, uint8_t hidden);

/**
 * @brief Hides the SSID of the access point.
 * @param wifi_ap_cfg A pointer to the wifi_config_t structure containing the access point configuration.
 */
void access_point_hide_ssid(wifi_config_t* wifi_ap_cfg);

/**
 * @brief Shows the SSID of the access point.
 * @param wifi_ap_cfg A pointer to the wifi_config_t structure containing the access point configuration.
 */
void access_point_show_ssid(wifi_config_t* wifi_ap_cfg);

/**
 * @brief Returns the hidden status of the access point.
 * @param wifi_ap_cfg A pointer to the wifi_config_t structure containing the access point configuration.
 * @return The hidden status of the access point (1 if hidden, 0 if not hidden).
 */
uint8_t access_point_get_hidden(wifi_config_t* wifi_ap_cfg);

/**
 * @brief Returns the SSID of the access point.
 * @param wifi_ap_cfg A pointer to the wifi_config_t structure containing the access point configuration.
 * @return The SSID of the access point.
 */
const char* access_point_get_ssid(wifi_config_t* wifi_ap_cfg);

/**
 * @brief Returns the password of the access point.
 * @param wifi_ap_cfg A pointer to the wifi_config_t structure containing the access point configuration.
 * @return The password of the access point.
 */
const char* access_point_get_password(wifi_config_t* wifi_ap_cfg);

/**
 * @brief Deinitializes ESP WiFi.
 * @param wifi_ap_cfg A pointer to the wifi_config_t structure containing the access point configuration.
 */
void access_point_deinit(wifi_config_t* wifi_ap_cfg);