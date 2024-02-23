#ifndef WIFI_AP_H
#define WIFI_AP_H

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/**
 * @brief Initializes the soft access point (AP) mode for the Wi-Fi module.
 *
 * This function sets up the Wi-Fi module to operate as a soft access point (AP) with the specified SSID, password,
 * channel, and maximum number of connections.
 *
 * @param ssid The SSID (network name) of the soft AP.
 * @param pass The password for the soft AP.
 * @param channel The Wi-Fi channel to use for the soft AP.
 * @param max_conn The maximum number of connections allowed for the soft AP.
 */
void wifi_init_softap(const char* ssid, const char* pass, int channel, int max_conn);

void set_static_ip(const char* ip, const char* gateway, const char* netmask);
#endif
