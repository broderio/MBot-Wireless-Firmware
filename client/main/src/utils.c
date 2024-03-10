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

#include "utils.h"

int max_conn_attempts = -1;
int conn_attempts = 0;

EventGroupHandle_t s_wifi_event_group;

esp_netif_t *sta_netif = NULL;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (max_conn_attempts > 0 && conn_attempts >= max_conn_attempts) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI("CLIENT", "Failed to connect to the AP");
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
        conn_attempts = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void utils_init() {
    gpio_config_t pair_config = {
        .pin_bit_mask = (0b1 << PAIR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = 0,
        .pull_up_en = 1,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&pair_config);

    s_wifi_event_group = xEventGroupCreate();
    assert(s_wifi_event_group);
}

int find_paired_ssid(char* ssid)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("CLIENT", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return -1;
    }

    // Read
    ESP_LOGI("CLIENT", "Reading SSID from NVS ... ");
    size_t len = SSID_LEN;
    err = nvs_get_str(my_handle, "ssid", ssid, &len);
    switch (err) {
        case ESP_OK:
            ssid[len] = 0;
            ESP_LOGI("CLIENT", "Found paired SSID: %s", ssid);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI("CLIENT", "The SSID is not found in NVS");
            break;
        default :
            ESP_LOGE("CLIENT", "Error (%s) reading!\n", esp_err_to_name(err));
            break;
    }

    // Close
    nvs_close(my_handle);
    return 0;
}

void set_paired_ssid(char* ssid)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("CLIENT", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    // Write
    err = nvs_set_str(my_handle, "ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE("CLIENT", "Error (%s) writing!\n", esp_err_to_name(err));
    }

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("CLIENT", "Error (%s) committing!\n", esp_err_to_name(err));
    }

    // Close
    nvs_close(my_handle);
}

void get_sta_config(wifi_config_t *wifi_config) 
{
    // Init STA netif object
    if (sta_netif != NULL) {
        esp_netif_destroy(sta_netif);
    }
    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_config_t wifi_sta_config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .threshold.authmode = WIFI_AUTH_OPEN,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    uint16_t number = 5;
    wifi_ap_record_t ap_info[5];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_LOGI("CLIENT", "Scanning for MBotWireless APs");
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 11,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_PASSIVE,
        .scan_time.passive = 100,
    };
    char *host_ssid;
    int found_ap = 0;
    do {
        esp_wifi_scan_start(&scan_config, true);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        esp_wifi_scan_stop();
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

        for (int i = 0; i < ap_count; i++) {
            if (strstr((char*)ap_info[i].ssid, SSID_PREFIX)) {
                host_ssid = (char*)ap_info[i].ssid;
                found_ap = 1;
                break;
            }
        }
    } while (!found_ap);
    ESP_LOGI("CLIENT", "Stopping scan.");
    esp_wifi_stop();

    strcpy((char*)wifi_sta_config.sta.ssid, host_ssid);
    strcpy((char*)wifi_sta_config.sta.password, AP_PASSWORD);
    *wifi_config = wifi_sta_config;
}

void set_sta_config(wifi_config_t *wifi_config, char* ssid) 
{
    if (sta_netif != NULL) {
        esp_netif_destroy(sta_netif);
    }
    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_config_t wifi_sta_config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .threshold.authmode = WIFI_AUTH_OPEN,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    strcpy((char*)wifi_sta_config.sta.ssid, ssid);
    strcpy((char*)wifi_sta_config.sta.password, AP_PASSWORD);
    *wifi_config = wifi_sta_config;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, wifi_config));
}

void start_wifi_event_handler()
{
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
}

int wait_for_connect(int attempts)
{
    max_conn_attempts = attempts;
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    max_conn_attempts = -1;
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("CLIENT", "Connected to AP!");
        return 0;
    } else {
        ESP_LOGW("CLIENT", "Failed to connect to AP!");
        return -1;
    }
}