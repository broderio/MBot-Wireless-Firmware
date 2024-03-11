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

#include "led.h"

#include "utils.h"

int num_connections_ap = 0;
int num_connections_socket = 0;

static esp_err_t set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE)) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    led_t *led = (led_t *)arg;
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        num_connections_ap++;
        ESP_LOGI("HOST", "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
        led_toggle(led);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        led_toggle(led);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        num_connections_ap--;
        ESP_LOGI("HOST", "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

void set_static_ip(esp_netif_t *netif, const char *static_ip, const char* static_netmask, const char* static_gw)
{
    if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE("HOST", "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0 , sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(static_ip);
    ip.netmask.addr = ipaddr_addr(static_netmask);
    ip.gw.addr = ipaddr_addr(static_gw);
    esp_netif_dhcps_stop(netif);
    esp_err_t err = esp_netif_set_ip_info(netif, &ip);
    if (err != ESP_OK) {
        ESP_LOGE("HOST", "Failed to set ip info. error: %s", esp_err_to_name(err));
        return;
    }
    esp_netif_dhcps_start(netif);
    ESP_LOGD("HOST", "Success to set static ip: %s, netmask: %s, gw: %s", static_ip, static_netmask, static_gw);
    ESP_ERROR_CHECK(set_dns_server(netif, ipaddr_addr(static_gw), ESP_NETIF_DNS_MAIN));
    ESP_ERROR_CHECK(set_dns_server(netif, ipaddr_addr("0.0.0.0"), ESP_NETIF_DNS_BACKUP));
}

void init_sta(wifi_config_t *wifi_sta_config) {
    ESP_LOGI("HOST", "Setting Wi-Fi mode to STA");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_LOGI("HOST", "Creating default Wi-Fi STA netif");
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_config_t wifi_config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .threshold.authmode = WIFI_AUTH_OPEN,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    *wifi_sta_config = wifi_config;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, wifi_sta_config));
}

void init_ap(wifi_config_t *wifi_ap_config) {
    ESP_LOGI("HOST", "Setting Wi-Fi mode to AP");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_LOGI("HOST", "Creating default Wi-Fi AP netif");
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);
    uint8_t mac_address[6];
    esp_wifi_get_mac(WIFI_IF_AP, mac_address);

    char mac_str[18]; // 6*2 digits + 5 colons + null terminator
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);

    wifi_config_t wifi_config = {
        .ap = {
            .channel = 6,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .password = AP_PASSWORD,
            .max_connection = MAX_CONN,
            .beacon_interval = 100,
        },
    };
    strcpy((char*)wifi_config.ap.ssid, SSID_PREFIX);
    strcat((char*)wifi_config.ap.ssid, mac_str);
    
    *wifi_ap_config = wifi_config;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, wifi_ap_config));

    led_t *led = led_create(LED1_PIN);
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        led,
                                                        NULL));
    set_static_ip(ap_netif, MBOT_IP_ADDR, MBOT_NETMASK_ADDR, MBOT_GW_ADDR);
    ESP_LOGI("HOST", "Restarting Wi-Fi");
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wait_for_no_hosts()
{
    ESP_ERROR_CHECK(esp_wifi_start());

    // Scan to check for broadcasted MBotWireless APs until there are no MBotWireless APs found
    uint16_t number = 5;
    wifi_ap_record_t ap_info[5];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_LOGI("HOST", "Scanning for MBotWireless APs");
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_PASSIVE,
        .scan_time.passive = 100,
    };
    uint16_t mbot_ap_count;
    do {
        esp_wifi_scan_start(&scan_config, true);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        esp_wifi_scan_stop();
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

        mbot_ap_count = 0;
        for (int i = 0; i < ap_count; i++) {
            if (strstr((char*)ap_info[i].ssid, SSID_PREFIX)) {
                mbot_ap_count++;
            }
        }
        ESP_LOGI("HOST", "MBotWireless APs found: %d", mbot_ap_count);
    } while (mbot_ap_count > 0);
    vTaskDelay(250 / portTICK_PERIOD_MS);
    ESP_LOGI("HOST", "Stopping scan.");
    ESP_ERROR_CHECK(esp_wifi_stop());
}