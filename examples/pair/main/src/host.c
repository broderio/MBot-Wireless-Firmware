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

#include "buttons.h"
#include "fram.h"
#include "joystick.h"
#include "sockets.h"

#include "host.h"
#include "utils.h"

void connection_task(void *args) {
    connection_socket_t connection = *(connection_socket_t*) args;
    while (true) {
        int len = socket_connection_send(connection, "Hello MBot!", 11);
        if (len > 0) {
            ESP_LOGI("HOST", "Sent [%d] bytes to [%d]", len, connection);
        }
        else if (len < 0) {
            ESP_LOGE("HOST", "Error sending to [%d]", connection);
            socket_connection_close(connection);
            num_connections_socket--;
            vTaskDelete(NULL);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void socket_task(void *args) {
    server_socket_t server = *(server_socket_t*) args;
    while (true) {
        connection_socket_t connection = socket_server_accept(server);
        if (connection > 0) {
            ESP_LOGI("HOST", "Client connected.");
            num_connections_socket++;
            xTaskCreate(connection_task, "connection_task", 4096, (void*) &connection, 5, NULL);
        }
        else if (connection == -1) {
            ESP_LOGE("HOST", "Error accepting connection.");
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    utils_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Init Wi-Fi
    ESP_LOGI("HOST", "Initializing Wi-Fi");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_sta_config;
    init_sta(&wifi_sta_config);

    start_blink(LED1_PIN, 100);
    wait_for_no_hosts();

     // Init AP config
    wifi_config_t wifi_ap_config;
    init_ap(&wifi_ap_config);
    stop_blink();

    // Once the MBotWireless AP is started, wait for a client to connect to the AP and the socket
    gpio_set_level(LED1_PIN, 1);
    ESP_LOGI("HOST", "Press the pair button once all clients are connected.");
    wait_for_pair();

    if (num_connections_ap == 0) {
        ESP_LOGW("HOST", "Warning! No clients connected.");
    }

    wifi_ap_config.ap.ssid_hidden = 1;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    server_socket_t server = socket_server_init(8000);
    wait_for_pair();
    gpio_set_level(LED1_PIN, 0);

    xTaskCreate(socket_task, "socket_task", 4096, (void*) &server, 5, NULL);
}