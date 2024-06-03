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

#define DEFAULT_PAIR_CFG            \
        {                           \
            .ssid = "mBot",         \
            .password = "I<3robots" \
        }

/**
 * @brief Structure representing the configuration for pairing.
 */
typedef struct pair_config_t {
    char ssid[9]; /**< Maximum unique name is 8 characters + null terminator */
    char password[16]; /**< Password for the Wi-Fi network */
} pair_config_t;

/**
 * @brief Retrieves the pair configuration.
 *
 * @return The pair configuration.
 */
pair_config_t get_pair_config();

/**
 * @brief Sets the pair configuration.
 *
 * @param pair_cfg The pair configuration to set.
 */
void set_pair_config(pair_config_t *pair_cfg);