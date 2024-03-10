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
#include "esp_timer.h"
#include "nvs_flash.h"

#include "buttons.h"
#include "fram.h"
#include "joystick.h"
#include "sockets.h"
#include "uart.h"
#include "serializer.h"
#include "lcm_types.h"

#include "client.h"
#include "utils.h"

#define S2U_BIT BIT0
#define U2S_BIT BIT1
#define DISCONNECT_BIT BIT2
#define CONNECT_BIT BIT3

SemaphoreHandle_t uart_sem;
SemaphoreHandle_t reconnect_sem;
EventGroupHandle_t connected_group;

client_socket_t connect_socket()
{
    client_socket_t client = socket_client_init(MBOT_HOST_IP_ADDR, 8000);
    while (client < 0)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        client = socket_client_init(MBOT_HOST_IP_ADDR, 8000);
    }
    return client;
}

void socket_to_uart_task(void *args)
{
    client_socket_t client = *(client_socket_t *)args;
    char buffer[256];
    while (true)
    {
        int bytes_read = socket_client_recv(client, buffer, 256);
        if (bytes_read == 0)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        else if (bytes_read < 0 || xEventGroupGetBits(connected_group) & DISCONNECT_BIT)
        {
            xEventGroupSetBits(connected_group, S2U_BIT);
            ESP_LOGW("CLIENT", "Deleting socket_to_uart_task.");
            vTaskDelete(NULL);
        }
        else
        {
            ESP_LOGI("CLIENT", "Read %d bytes from socket.", bytes_read);
            xSemaphoreTake(uart_sem, portMAX_DELAY);
            uart_write(1, buffer, bytes_read);
            xSemaphoreGive(uart_sem);
        }
    }
}

void uart_to_socket_task(void *args)
{
    client_socket_t client = *(client_socket_t *)args;
    char buffer[256];
    while (true)
    {
        int bytes_read = uart_read(1, buffer, 256, 100);
        if (bytes_read > 0)
        {
            ESP_LOGI("CLIENT", "Read %d bytes from UART.", bytes_read);

            int ret = socket_client_send(client, buffer, bytes_read);

            if (ret < 0 || xEventGroupGetBits(connected_group) & DISCONNECT_BIT)
            {
                xEventGroupSetBits(connected_group, U2S_BIT);
                ESP_LOGW("CLIENT", "Deleting uart_to_socket_task.");
                vTaskDelete(NULL);
            }
        }
    }
}

void heartbeat_task(void *args)
{
    serial_timestamp_t timestamp = {0};
    while (true)
    {
        if (xEventGroupGetBits(connected_group) & DISCONNECT_BIT)
        {
            ESP_LOGW("CLIENT", "Suspending heartbeat_task.");
            vTaskSuspend(NULL);
        }

        timestamp.utime = esp_timer_get_time();
        uint8_t msg[sizeof(serial_timestamp_t)];
        timestamp_t_serialize(&timestamp, msg);

        uint8_t pkt[sizeof(serial_timestamp_t) + ROS_PKG_LEN];
        encode_rospkt(msg, sizeof(serial_timestamp_t), MBOT_TIMESYNC, pkt);

        xSemaphoreTake(uart_sem, portMAX_DELAY);
        uart_write(1, (char *)pkt, sizeof(pkt));
        xSemaphoreGive(uart_sem);

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    // Init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    utils_init();

    connected_group = xEventGroupCreate();

    uart_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(uart_sem);

    uart_init(1, RC_TX_PIN, RC_RX_PIN, 115200, 1024);

    button_config_t buttons_config = {
        .pin_bit_mask = (0b1 << PAIR_PIN),
        .long_press_time_ms = 1000,
        .hold_time_ms = 2000,
        .double_press_time_ms = 250};
    int buttons = buttons_init(&buttons_config);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    int was_paired = 0;
    char paired_ssid[SSID_LEN];
    if (find_paired_ssid(paired_ssid) == 0)
    {
        was_paired = 1;
    }

    // Init Wi-Fi in STA mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    int connected = 0;
    while (!connected)
    {
        wifi_config_t wifi_sta_config;
        if (!was_paired)
        {
            get_sta_config(&wifi_sta_config);
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
            // First pairing to connect to public broadcast of AP
            button_event_t event = {0};
            while (event.pin != PAIR_PIN && event.type != SHORT_PRESS)
            {
                buttons_wait_for_event(buttons, &event, portMAX_DELAY);
            }
            event.pin = -1;
            event.type = NO_PRESS;

            start_wifi_event_handler();
            esp_wifi_start();
            wait_for_connect(5);

            // Second pair to connect to private broadcast of AP
            while (event.pin != PAIR_PIN && event.type != SHORT_PRESS)
            {
                buttons_wait_for_event(buttons, &event, portMAX_DELAY);
            }
            event.pin = -1;
            event.type = NO_PRESS;

            esp_wifi_disconnect();
            wait_for_connect(5);
            set_paired_ssid((char *)wifi_sta_config.sta.ssid);
            connected = 1;
        }
        else
        {
            set_sta_config(&wifi_sta_config, paired_ssid);
            start_wifi_event_handler();
            esp_wifi_start();
            int timeout = wait_for_connect(3);
            if (timeout < 0)
            {
                ESP_LOGE("CLIENT", "Failed to connect to AP. Starting pairing protocol...");
                was_paired = 0;
            }
            else
            {
                connected = 1;
            }
        }
    }

    TaskHandle_t socket_to_uart_task_handle, uart_to_socket_task_handle, heartbeat_task_handle;

    xTaskCreate(heartbeat_task, "heartbeat_task", 4096, NULL, 5, &heartbeat_task_handle);

    client_socket_t client = connect_socket();

    xTaskCreate(socket_to_uart_task, "socket_to_uart_task", 4096, &client, 5, &socket_to_uart_task_handle);
    xTaskCreate(uart_to_socket_task, "uart_to_socket_task", 4096, &client, 5, &uart_to_socket_task_handle);

    while (true)
    {
        // Wait for the socket to disconnect
        xEventGroupWaitBits(connected_group,
                            S2U_BIT | U2S_BIT,
                            pdFALSE,
                            pdFALSE,
                            portMAX_DELAY);
        ESP_LOGW("CLIENT", "Server disconnected.");
        xEventGroupSetBits(connected_group, DISCONNECT_BIT);

        // Ensure that the tasks are stopped
        while (eTaskGetState(socket_to_uart_task_handle) != eDeleted &&
               eTaskGetState(uart_to_socket_task_handle) != eDeleted && 
               eTaskGetState(heartbeat_task_handle) != eSuspended)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        xEventGroupClearBits(connected_group, S2U_BIT | U2S_BIT | DISCONNECT_BIT);

        // Reconnect to the server
        socket_client_close(client);
        client = connect_socket();

        // Restart the tasks
        xTaskCreate(socket_to_uart_task, "socket_to_uart_task", 4096, &client, 5, &socket_to_uart_task_handle);
        xTaskCreate(uart_to_socket_task, "uart_to_socket_task", 4096, &client, 5, &uart_to_socket_task_handle);
        vTaskResume(heartbeat_task_handle);
    }
}
