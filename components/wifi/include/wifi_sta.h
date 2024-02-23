#ifndef WIFI_STA_H
#define WIFI_STA_H

#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

void wifi_init_sta(const char *ssid, const char *password);
void wifi_init_sta_static_ip(const char *ssid, const char *password, const char *static_ip, const char *gateway, const char *netmask);

#endif