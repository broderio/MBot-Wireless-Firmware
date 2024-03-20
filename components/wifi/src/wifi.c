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

#include "wifi.h"


#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_DISCONNECTED_BIT  BIT1
#define WIFI_FAIL_BIT  BIT2

static uint8_t in_station_mode = 0;
static uint8_t station_connected = 0;
static uint8_t station_connecting = 0;
static uint8_t access_point_hidden = 0;
static uint8_t access_point_hosting = 0;
static int16_t conn_attempts = 0;
static int16_t max_conn_attempts = -1;

static EventGroupHandle_t sta_event_group;

void _sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        station_connected = 0;
        station_connecting = 1;
        if (conn_attempts == 0) {
            ESP_LOGI("CLIENT", "Disconnected from AP");
        }

        if (max_conn_attempts > 0 && conn_attempts >= max_conn_attempts) {
            ESP_LOGI("CLIENT", "Failed to connect to the AP");
            station_connecting = 0;
            return;
        }
        esp_wifi_connect();
        ESP_LOGI("CLIENT", "retry to connect to the AP");
        conn_attempts++;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("CLIENT", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        station_connected = 1;
        conn_attempts = 0;
    }
}

void _ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI("HOST", "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI("HOST", "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

void _start_sta_event_handler()
{
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_err_t err = esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &_sta_event_handler,
                                                        NULL,
                                                        &instance_any_id);
    if (err != ESP_OK) {
        ESP_LOGE("CLIENT", "Failed to register event handler");
    }
                                                
    err = esp_event_handler_instance_register(IP_EVENT,
                                              IP_EVENT_STA_GOT_IP,
                                              &_sta_event_handler,
                                              NULL,
                                              &instance_got_ip);
    if (err != ESP_OK) {
        ESP_LOGE("CLIENT", "Failed to register event handler");
    }                    
}

void _stop_sta_event_handler()
{
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, _sta_event_handler);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, _sta_event_handler);
}

void _start_ap_event_handler()
{
    esp_err_t err = esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &_ap_event_handler,
                                                        NULL,
                                                        NULL);
    if (err != ESP_OK) {
        ESP_LOGE("HOST", "Failed to register event handler");
    }                                                        
}

void _stop_ap_event_handler()
{
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, _ap_event_handler);
}

esp_err_t _ap_set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE)) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}

void _ap_set_static_ip(esp_netif_t *netif, const char *static_ip, const char* static_netmask, const char* static_gw)
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
    ESP_ERROR_CHECK(_ap_set_dns_server(netif, ipaddr_addr(static_gw), ESP_NETIF_DNS_MAIN));
    ESP_ERROR_CHECK(_ap_set_dns_server(netif, ipaddr_addr("0.0.0.0"), ESP_NETIF_DNS_BACKUP));
}


wifi_init_config_t *wifi_start() {
    wifi_init_config_t *wifi_cfg = (wifi_init_config_t *)malloc(sizeof(wifi_init_config_t));
    if (wifi_cfg == NULL) {
        ESP_LOGE("PAIRING", "Failed to allocate memory for wifi init config");
        return NULL;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE("PAIRING", "Failed to initialize netif. Error: %s", esp_err_to_name(err));
        free(wifi_cfg);
        return NULL;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE("PAIRING", "Failed to create event loop. Error: %s", esp_err_to_name(err));
        free(wifi_cfg);
        return NULL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE("PAIRING", "Failed to initialize wifi. Error: %s", esp_err_to_name(err));
        free(wifi_cfg);
        return NULL;
    }
    return wifi_cfg;
}

void wifi_stop(wifi_init_config_t *wifi_cfg) {
    esp_wifi_stop();
    esp_wifi_deinit();
    free(wifi_cfg);
}

wifi_config_t* station_init()
{
    if (in_station_mode) {
        ESP_LOGE("PAIRING", "Station is already initialized");
        return NULL;
    }

    if (access_point_hosting) {
        ESP_LOGE("PAIRING", "Device is in access point mode. Deinit the access point before initializing as station.");
        return NULL;
    }

    wifi_config_t *cfg = (wifi_config_t *)malloc(sizeof(wifi_config_t));
    if (cfg == NULL) {
        ESP_LOGE("PAIRING", "Failed to allocate memory for wifi config");
        return NULL;
    }
    memset(cfg, 0, sizeof(wifi_config_t));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        ESP_LOGE("PAIRING", "Failed to create default Wi-Fi STA netif");
        free(cfg);
        return NULL;
    }

    cfg->sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    cfg->sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    cfg->sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    esp_wifi_set_config(WIFI_IF_STA, cfg);
    in_station_mode = 1;
    sta_event_group = xEventGroupCreate();
    return cfg;
}

void station_deinit(wifi_config_t* wifi_sta_cfg)
{
    if (!in_station_mode) {
        ESP_LOGE("PAIRING", "Station is not initialized");
        return;
    }

    if (station_connected) {
        station_disconnect();
    }

    esp_wifi_stop();
    free(wifi_sta_cfg);
    in_station_mode = 0;
    max_conn_attempts = -1;
    conn_attempts = 0;
}

void station_set_max_conn_attempts(int16_t attempts)
{
    max_conn_attempts = attempts;
}

int16_t station_get_max_conn_attempts()
{
    return max_conn_attempts;
}

wifi_ap_record_t *station_scan(wifi_config_t* wifi_sta_cfg, uint16_t* num_ap_records, uint8_t show_hidden)
{
    if (!in_station_mode) {
        ESP_LOGE("PAIRING", "Station is not initialized");
        return NULL;
    }

    if (station_connected) {
        ESP_LOGE("PAIRING", "Station is connected, cannot scan for access points");
        return NULL;
    }

    if (esp_wifi_start() != ESP_OK) {
        ESP_LOGE("PAIRING", "Failed to start wifi");
        return NULL;
    }

    wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * *num_ap_records);
    if (ap_records == NULL) {
        ESP_LOGE("PAIRING", "Failed to allocate memory for ap records");
        return NULL;
    }

    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = show_hidden,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.passive = 100,
    };
    esp_wifi_scan_start(&scan_cfg, true);
    esp_wifi_scan_stop();
    esp_wifi_scan_get_ap_records(num_ap_records, ap_records);

    esp_wifi_stop();

    return ap_records;
}

void station_free_scan(wifi_ap_record_t *ap_records)
{
    free(ap_records);
}

void station_connect(wifi_config_t *wifi_sta_cfg, const char *ssid, const char *password)
{
    if (!in_station_mode) {
        ESP_LOGE("PAIRING", "Station is not initialized");
        return;
    }

    if (station_connected) {
        ESP_LOGE("PAIRING", "Station is already connected");
        return;
    }
    
    if (esp_wifi_start() != ESP_OK) {
        ESP_LOGE("PAIRING", "Failed to start wifi");
        return;
    }

    strcpy((char*)wifi_sta_cfg->sta.ssid, ssid);
    strcpy((char*)wifi_sta_cfg->sta.password, password);

    ESP_LOGI("PAIRING", "Attempting to connect to %s with password %s", wifi_sta_cfg->sta.ssid, wifi_sta_cfg->sta.password);

    esp_wifi_set_config(WIFI_IF_STA, wifi_sta_cfg);

    esp_wifi_start();
    _start_sta_event_handler();
    esp_wifi_connect();
}

uint8_t station_wait_for_connection(int32_t timeout_ms)
{
    if (!in_station_mode) {
        ESP_LOGE("PAIRING", "Station is not initialized");
        return 0;
    }

    if (station_connected) {
        ESP_LOGI("PAIRING", "Station is already connected");
        return 1;
    }

    TickType_t ticks_elapsed = 0;
    TickType_t ticks_to_wait;
    ticks_to_wait = (timeout_ms < 0) ? portMAX_DELAY : (timeout_ms / portTICK_PERIOD_MS);
    while (ticks_elapsed < ticks_to_wait && !station_connected) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ticks_elapsed += 100 / portTICK_PERIOD_MS;
    } 
    return station_connected;
}

void station_disconnect()
{
    if (!in_station_mode) {
        ESP_LOGE("PAIRING", "Station is not initialized");
        return;
    }

    if (!station_connected) {
        ESP_LOGW("PAIRING", "Station is not connected");
        return;
    }

    _stop_sta_event_handler();
    esp_wifi_disconnect();
    esp_wifi_stop();
    station_connected = 0;
}

uint8_t station_is_connected() {
    return station_connected;
}

uint8_t station_is_disconnected() {
    return !station_connected;
}

uint8_t station_is_connecting() {
    return station_connecting;
}

uint8_t station_connection_failed() {
    return !station_connected && !station_connecting;
}

wifi_config_t* access_point_init(const char *ssid, const char *password, uint8_t channel, uint8_t hidden, uint8_t max_connections)
{
    if (in_station_mode) {
        ESP_LOGE("PAIRING", "Device is in station mode. Deinit the station before initializing as access point.");
        return NULL;
    }
    
    if (access_point_hosting) {
        ESP_LOGE("PAIRING", "Access point is already hosting");
        return NULL;
    }

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == NULL) {
        ESP_LOGE("PAIRING", "Failed to create default Wi-Fi AP netif");
        return NULL;
    }

    wifi_config_t *wifi_ap_cfg = (wifi_config_t *)malloc(sizeof(wifi_config_t));
    if (wifi_ap_cfg == NULL) {
        ESP_LOGE("PAIRING", "Failed to allocate memory for wifi config");
        return NULL;
    }
    wifi_ap_cfg->ap.channel = channel;
    wifi_ap_cfg->ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_ap_cfg->ap.max_connection = max_connections;
    wifi_ap_cfg->ap.beacon_interval = 100;
    strcpy((char*)wifi_ap_cfg->ap.ssid, ssid);
    wifi_ap_cfg->ap.ssid_len = strlen(ssid);
    strcpy((char*)wifi_ap_cfg->ap.password, password);
    wifi_ap_cfg->ap.ssid_hidden = hidden;
    wifi_ap_cfg->ap.pmf_cfg.required = 0;
    access_point_hidden = hidden;
    
    esp_wifi_set_config(WIFI_IF_AP, wifi_ap_cfg);
    esp_wifi_start();
    _start_ap_event_handler();
    _ap_set_static_ip(ap_netif, MBOT_IP_ADDR, MBOT_NETMASK_ADDR, MBOT_GW_ADDR);
    access_point_hosting = 1;
    return wifi_ap_cfg;
}

void access_point_hide_ssid(wifi_config_t* wifi_ap_cfg)
{
    if (in_station_mode) {
        ESP_LOGE("PAIRING", "Device is in station mode. Deinit the station before hiding the access point.");
        return;
    }

    if (!access_point_hosting) {
        ESP_LOGW("PAIRING", "Access point is not hosting");
        return;
    }

    if (access_point_hidden) {
        ESP_LOGW("PAIRING", "Access point is already hidden");
        return;
    }
    esp_wifi_stop();
    wifi_ap_cfg->ap.ssid_hidden = 1;
    esp_wifi_set_config(WIFI_IF_AP, wifi_ap_cfg);
    esp_wifi_start();
    access_point_hidden = 1;
}

void access_point_show_ssid(wifi_config_t* wifi_ap_cfg)
{
    if (in_station_mode) {
        ESP_LOGE("PAIRING", "Device is in station mode. Deinit the station before showing the access point.");
        return;
    }

    if (!access_point_hosting) {
        ESP_LOGW("PAIRING", "Access point is not hosting. Cannot show access point.");
        return;
    }

    if (!access_point_hidden) {
        ESP_LOGW("PAIRING", "Access point is already showing");
        return;
    }

    esp_wifi_stop();
    wifi_ap_cfg->ap.ssid_hidden = 0;
    esp_wifi_set_config(WIFI_IF_AP, wifi_ap_cfg);
    esp_wifi_start();
    access_point_hidden = 0;
}

uint8_t access_point_get_hidden(wifi_config_t* wifi_ap_cfg)
{
    if (in_station_mode) {
        ESP_LOGE("PAIRING", "Device is in station mode. Cannot get hidden status of access point.");
        return 0;
    }

    if (!access_point_hosting) {
        ESP_LOGW("PAIRING", "Access point is not hosting. Cannot get hidden status of access point.");
        return 0;
    }

    return access_point_hidden;
}

const char* access_point_get_ssid(wifi_config_t* wifi_ap_cfg)
{
    if (in_station_mode) {
        ESP_LOGE("PAIRING", "Device is in station mode. Cannot get SSID of access point.");
        return NULL;
    }

    if (!access_point_hosting) {
        ESP_LOGW("PAIRING", "Access point is not hosting. Cannot get SSID of access point.");
        return NULL;
    }

    return (const char*)wifi_ap_cfg->ap.ssid;
}

const char* access_point_get_password(wifi_config_t* wifi_ap_cfg)
{
    if (in_station_mode) {
        ESP_LOGE("PAIRING", "Device is in station mode. Cannot get password of access point.");
        return NULL;
    }

    if (!access_point_hosting) {
        ESP_LOGW("PAIRING", "Access point is not hosting. Cannot get password of access point.");
        return NULL;
    }

    return (const char*)wifi_ap_cfg->ap.password;
}

void access_point_deinit(wifi_config_t* wifi_ap_cfg)
{
    if (in_station_mode) {
        ESP_LOGE("PAIRING", "Device is in station mode. Deinit the station before deinitializing the access point.");
        return;
    }

    if (!access_point_hosting) {
        ESP_LOGW("PAIRING", "Access point is not hosting. Cannot deinitialize access point.");
        return;
    }

    esp_wifi_stop();
    free(wifi_ap_cfg);
    access_point_hosting = 0;
}