#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "wifi_sta.h"
#include "sockets.h"

#define ESP_WIFI_SSID           "930Catherine"
#define ESP_WIFI_PASS           "Catherine2024!"
#define ESP_MAXIMUM_RETRY       5

#define PORT                    1234
#define STATIC_IP_ADDR          "10.0.0.2"
#define STATIC_NETMASK_ADDR     "255.255.255.0"
#define STATIC_GW_ADDR          "10.0.0.1"
#define MAIN_DNS_SERVER         STATIC_GW_ADDR
#define BACKUP_DNS_SERVER       "0.0.0.0"

#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

static const char *NETWORK_TEST_TAG = "NETWORK_TEST";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(NETWORK_TEST_TAG, "ESP_WIFI_MODE_STA");

    wifi_init_sta_static_ip(ESP_WIFI_SSID, 
                            ESP_WIFI_PASS, 
                            STATIC_IP_ADDR, 
                            STATIC_NETMASK_ADDR, 
                            STATIC_GW_ADDR);

    // xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
    server_socket_t server = socket_server_init(PORT);
    while (true) {
        connection_socket_t connection = socket_server_accept(server);
        while (connection > 0) {
            char buffer[128];
            int len = socket_connection_recv(connection, buffer, sizeof(buffer));
            if (len < 0) {
                // ESP_LOGE(NETWORK_TEST_TAG, "Error occurred during receiving: errno %d", errno);
                ESP_LOGI(NETWORK_TEST_TAG, "Client disconnected.");
                socket_connection_close(connection);
                break;
            } 
            else
            {
                buffer[len] = 0;
                ESP_LOGI(NETWORK_TEST_TAG, "Received %d bytes: %s", len, buffer);
            }
        }
    }
}