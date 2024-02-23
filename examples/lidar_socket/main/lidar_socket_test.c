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
#include "lidar.h"

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

#define DENSITY                 1

#define LIDAR_TX_PIN                43                  /**< GPIO Pin for UART transmit line */
#define LIDAR_RX_PIN                44                  /**< GPIO Pin for UART receive line */
#define LIDAR_PWM_PIN               1                   /**< GPIO Pin for LIDAR PWM signal */

static const char *NETWORK_TEST_TAG = "NETWORK_TEST";

uint16_t scan[360 * DENSITY + 1] = {0};
TaskHandle_t lidar_task_handle;
int connected = 0;

void lidar_task(void*)
{
    while (1)
    {
        lidar_t lidar;
        int error = lidar_init(&lidar, LIDAR_TX_PIN, LIDAR_RX_PIN, LIDAR_PWM_PIN);
        if (error != 0)
        {
            return;
        }
        scan[0] = 0xFFFF;
        lidar_start_exp_scan(&lidar);
        while (1) 
        {
            if (!connected)
            {
                lidar_stop_scan(&lidar);
                lidar_deinit(&lidar);
                vTaskSuspend(NULL);
                break;
            }
            int ret = lidar_get_exp_scan_360(&lidar, scan + 1);
            if (ret < 0)
            {
                ESP_LOGE(NETWORK_TEST_TAG, "Error getting scan");
                lidar_stop_scan(&lidar);
                lidar_reset(&lidar);
                ret = lidar_start_exp_scan(&lidar);
                if (ret < 0)
                {
                    ESP_LOGE(NETWORK_TEST_TAG, "Error starting scan");
                    break;
                }
            }
        }
    }
}

void socket_task(void*)
{
    server_socket_t server = socket_server_init(PORT);
    while (true) {
        connection_socket_t connection = socket_server_accept(server);
        if (connection > 0) {
            connected = 1;
            vTaskResume(lidar_task_handle);
            TickType_t last_wake_time;
            while (true)
            {
                last_wake_time = xTaskGetTickCount();
                int ret = socket_connection_send(connection, (char*)scan, sizeof(scan));
                if (ret < 0) {
                    ESP_LOGI(NETWORK_TEST_TAG, "Client disconnected.");
                    connected = 0;
                    break;
                }
                xTaskDelayUntil(&last_wake_time, 100 / portTICK_PERIOD_MS);
            }
        }
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

    ESP_LOGI(NETWORK_TEST_TAG, "ESP_WIFI_MODE_STA");

    wifi_init_sta_static_ip(ESP_WIFI_SSID, 
                            ESP_WIFI_PASS, 
                            STATIC_IP_ADDR, 
                            STATIC_NETMASK_ADDR, 
                            STATIC_GW_ADDR);

    xTaskCreate(socket_task, "socket_task", 4096, NULL, 5, NULL);
    xTaskCreate(lidar_task, "lidar_task", 4096, NULL, 5, &lidar_task_handle);
}