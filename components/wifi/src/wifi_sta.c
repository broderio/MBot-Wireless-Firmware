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

#include "wifi_sta.h"

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

const char *WIFI_STA_TAG = "wifi station";

/**
 * @brief Structure representing the arguments for configuring the WiFi station mode.
 */
typedef struct _wifi_sta_args_t
{
    esp_netif_t *netif;         /**< Pointer to the network interface. */
    char *static_ip;            /**< Static IP address. */
    char *static_netmask;       /**< Static netmask. */
    char *static_gw;            /**< Static gateway. */
} _wifi_sta_args_t;

/**
 * @brief Sets the DNS server for the specified network interface.
 *
 * This function sets the DNS server for the specified network interface to the provided IP address.
 * The IP address must be a valid IPv4 address. If the address is zero or IPADDR_NONE, the DNS server will not be set.
 *
 * @param netif The network interface to set the DNS server for.
 * @param addr The IP address of the DNS server.
 * @param type The type of DNS server (primary or secondary).
 * @return esp_err_t Returns ESP_OK if the DNS server is set successfully, otherwise returns an error code.
 */
static esp_err_t _set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE)) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}

/**
 * @brief Sets the static IP address, netmask, and gateway for a given network interface.
 *
 * This function stops the DHCP client for the specified network interface and sets the provided static IP address,
 * netmask, and gateway. It also sets the DNS servers to the provided gateway and a backup DNS server to "0.0.0.0".
 *
 * @param netif The network interface to set the static IP for.
 * @param static_ip The static IP address to set.
 * @param static_netmask The static netmask to set.
 * @param static_gw The static gateway to set.
 */
static void _set_static_ip(esp_netif_t *netif, const char *static_ip, const char* static_netmask, const char* static_gw)
{
    if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE(WIFI_STA_TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0 , sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(static_ip);
    ip.netmask.addr = ipaddr_addr(static_netmask);
    ip.gw.addr = ipaddr_addr(static_gw);
    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK) {
        ESP_LOGE(WIFI_STA_TAG, "Failed to set ip info");
        return;
    }
    ESP_LOGD(WIFI_STA_TAG, "Success to set static ip: %s, netmask: %s, gw: %s", static_ip, static_netmask, static_gw);
    ESP_ERROR_CHECK(_set_dns_server(netif, ipaddr_addr(static_gw), ESP_NETIF_DNS_MAIN));
    ESP_ERROR_CHECK(_set_dns_server(netif, ipaddr_addr("0.0.0.0"), ESP_NETIF_DNS_BACKUP));
}

/**
 * @brief Event handler for WiFi station events.
 *
 * This function handles various WiFi station events such as STA start, connection, disconnection, and obtaining IP address.
 *
 * @param arg Pointer to the arguments structure (_wifi_sta_args_t).
 * @param event_base Event base.
 * @param event_id Event ID.
 * @param event_data Pointer to the event data.
 */
static void _wifi_sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED && arg != NULL)
    {
        _wifi_sta_args_t *args = (_wifi_sta_args_t *)arg;
        _set_static_ip(args->netif, args->static_ip, args->static_netmask, args->static_gw);
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < 5)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_STA_TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_STA_TAG,"connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_STA_TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief Initialize WiFi station mode.
 *
 * This function initializes the WiFi station mode with the provided SSID and password.
 *
 * @param ssid The SSID of the WiFi network.
 * @param password The password of the WiFi network.
 */
void wifi_init_sta(const char *ssid, const char *password)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &_wifi_sta_event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &_wifi_sta_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_STA_TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(WIFI_STA_TAG, "connected to ap SSID:%s password:%s", ssid, password);
    } 
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(WIFI_STA_TAG, "Failed to connect to SSID:%s, password:%s", ssid, password);
    } 
    else
    {
        ESP_LOGE(WIFI_STA_TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

/**
 * @brief Initialize WiFi station mode with static IP configuration.
 *
 * This function initializes the WiFi station mode with the provided SSID, password, and static IP configuration.
 *
 * @param ssid The SSID of the WiFi network.
 * @param password The password of the WiFi network.
 * @param static_ip The static IP address.
 * @param static_netmask The static network mask.
 * @param static_gw The static gateway IP address.
 */
void wifi_init_sta_static_ip(const char *ssid, const char *password, const char *static_ip, const char *static_netmask, const char *static_gw)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    _wifi_sta_args_t args = {
        .netif = esp_netif_create_default_wifi_sta(),
        .static_ip = static_ip,
        .static_netmask = static_netmask,
        .static_gw = static_gw
    };
    assert(args.netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &_wifi_sta_event_handler,
                                                        &args,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &_wifi_sta_event_handler,
                                                        &args,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_STA_TAG, "wifi_init_sta_static_ip finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(WIFI_STA_TAG, "connected to ap SSID:%s password:%s", ssid, password);
    } 
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(WIFI_STA_TAG, "Failed to connect to SSID:%s, password:%s", ssid, password);
    } 
    else
    {
        ESP_LOGE(WIFI_STA_TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}