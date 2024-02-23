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

#include "buttons.h"
#include "fram.h"
#include "joystick.h"
#include "sockets.h"

#include "client.h"
#include "utils.h"

void socket_task(void *args) {
    client_socket_t client = *(client_socket_t*) args;
    char buffer[256];
    while (true) {
        vTaskDelay(250 / portTICK_PERIOD_MS);
        int bytes_read = socket_client_recv(client, buffer, 256);
        if (bytes_read == 0) {
            continue;
        }
        else if (bytes_read < 0) {
            ESP_LOGW("CLIENT", "Server disconnected. Reconnecting...");
            client = socket_client_close(client);
            do {
                client = socket_client_init(MBOT_HOST_IP_ADDR, 8000);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            } while (client < 0);
        }
        else {
            buffer[bytes_read] = 0;
            ESP_LOGI("CLIENT", "Received %d bytes: %s", bytes_read, buffer);
        }
    }
}

void app_main(void)
{
    // Init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    utils_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    int was_paired = 0;
    char paired_ssid[SSID_LEN];
    if (find_paired_ssid(paired_ssid) == 0) {
        was_paired = 1;
    }

    // Init Wi-Fi in STA mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    int connected = 0;
    while (!connected) {
        wifi_config_t wifi_sta_config;
        if (!was_paired)
        {
            get_sta_config(&wifi_sta_config);
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
            // First pairing to connect to public broadcast of AP
            wait_for_pair();
            start_wifi_event_handler();
            esp_wifi_start();
            wait_for_connect(5);

            // Second pair to connect to private broadcast of AP
            wait_for_pair();

            esp_wifi_disconnect();
            wait_for_connect(5);
            set_paired_ssid((char*)wifi_sta_config.sta.ssid);
            connected = 1;
        }
        else 
        {
            set_sta_config(&wifi_sta_config, paired_ssid);
            start_wifi_event_handler();
            esp_wifi_start();
            int timeout = wait_for_connect(3);
            if (timeout < 0) {
                ESP_LOGE("CLIENT", "Failed to connect to AP. Starting pairing protocol...");
                was_paired = 0;
            }
            else {
                connected = 1;
            }
        }
    }

    client_socket_t client = socket_client_init(MBOT_HOST_IP_ADDR, 8000);
    xTaskCreate(socket_task, "socket_task", 4096, (void*) &client, 5, NULL);
}
