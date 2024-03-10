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
#include "esp_timer.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "buttons.h"
#include "joystick.h"
#include "sockets.h"
#include "led.h"
#include "usb_device.h"
#include "serializer.h"
#include "lcm_types.h"

#include "host.h"
#include "utils.h"

#define BUFFER_SIZE 64

typedef struct connection_args_t
{
    connection_socket_t connection;
    uint8_t robot_id;
} connection_args_t;

server_socket_t server;

QueueHandle_t usb_recv_queue[MAX_CONN];

void connection_task(void *args)
{
    if (args == NULL)
    {
        ESP_LOGE("HOST", "Error: NULL connection args.");
        vTaskDelete(NULL);
    }

    connection_args_t* conn_args = (connection_args_t *)args;
    connection_socket_t connection = conn_args->connection;
    uint8_t robot_id = conn_args->robot_id;
    char trigger = 0x0;

    while (true)
    {
        packet_t usb_packet;
        if (xQueueReceive(usb_recv_queue[robot_id], &usb_packet, 0) == pdTRUE)
        {
            int bytes_sent = socket_connection_send(connection, (char *)usb_packet.data, usb_packet.len);
            free(usb_packet.data);
            if (bytes_sent < 0)
            {
                ESP_LOGE("HOST", "Error sending data to client %d", robot_id);
                break;
            }
            led_on(LED2_PIN);
        }

        int bytes_read = socket_connection_recv(connection, &trigger, 1);
        if (bytes_read == 0)
        {
            continue;
        }
        else if (bytes_read < 0)
        {
            break;
        }
        else if (trigger == SYNC_FLAG)
        {
            uint8_t header[ROS_HEADER_LEN];
            header[0] = SYNC_FLAG;

            bytes_read = 0;
            while (bytes_read < ROS_HEADER_LEN - 1)
            {
                int bytes = socket_connection_recv(connection, (char *)(header + 1 + bytes_read), ROS_HEADER_LEN - 1 - bytes_read);
                if (bytes < 0)
                {
                    ESP_LOGE("HOST", "Error receiving header from client %d", robot_id);
                    break;
                }
                bytes_read += bytes;
            }

            uint16_t msg_len = ((uint16_t)header[3] << 8) | (header[2] & 0xFF);

            uint8_t *packet = (uint8_t *)malloc(msg_len + 4 + ROS_PKG_LEN);

            packet[0] = SYNC_FLAG;
            packet[1] = robot_id;
            packet[2] = (msg_len + ROS_PKG_LEN) & 0xFF; // LSB
            packet[3] = ((msg_len + ROS_PKG_LEN) >> 8) & 0xFF; // MSB

            memcpy(packet + 4, header, ROS_HEADER_LEN);

            bytes_read = 0;
            while (bytes_read < msg_len + 1)
            {
                int bytes = socket_connection_recv(connection, (char *)(packet + 4 + ROS_HEADER_LEN + bytes_read), msg_len + 1 - bytes_read);
                if (bytes < 0)
                {
                    ESP_LOGE("HOST", "Error receiving data from client %d", robot_id);
                    break;
                }
                bytes_read += bytes;
            }
            usb_device_send(packet, msg_len + 4 + ROS_PKG_LEN);
            free(packet);
        }
    }
    ESP_LOGW("HOST", "Client disconnected. Closing connection...");
    socket_connection_close(connection);
    num_connections_socket--;
    free(conn_args);
    vTaskDelete(NULL);
}

void socket_task(void *args)
{
    while (true)
    {
        connection_socket_t connection = socket_server_accept(server);
        if (connection > 0 && num_connections_socket < MAX_CONN)
        {
            ESP_LOGI("HOST", "Client connected.");
            connection_args_t* connection_args = malloc(sizeof(connection_args_t));
            if (connection_args == NULL)
            {
                ESP_LOGE("HOST", "Error allocating memory for connection args.");
                continue;
            }
            connection_args->connection = connection;
            connection_args->robot_id = num_connections_socket;
            num_connections_socket++;
            xTaskCreate(connection_task, "connection_task", 4096, (void *)connection_args, 5, NULL);
        }
        else if (connection == -1)
        {
            ESP_LOGE("HOST", "Error accepting connection.");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// This task will read input data from the USB and push it to the corresponding USB queue
// Packet structure: [SYNC_FLAG, ROBOT_ID, MSG_LEN, [PACKET]]
void usb_task(void *args)
{
    uint8_t trigger = 0x0;
    led_on(LED2_PIN);
    while (true)
    {
        int bytes_read = usb_device_read(&trigger, 1);
        if (bytes_read == 0)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        else if (trigger == SYNC_FLAG)
        {
            uint8_t header[4];
            header[0] = trigger;
            bytes_read = 0;
            while (bytes_read < 3)
            {
                int bytes = usb_device_read(header + 1 + bytes_read, 3 - bytes_read);
                if (bytes < 0)
                {
                    break;
                }
                bytes_read += bytes;
            }
            uint8_t robot_id = header[1];
            uint16_t msg_len = ((uint16_t)header[3] << 8) | (header[2] & 0xFF);
            
            packet_t packet;
            packet.data = (uint8_t *)malloc(msg_len);
            packet.len = msg_len;

            bytes_read = 0;
            while (bytes_read < msg_len)
            {
                int bytes = usb_device_read(packet.data + bytes_read, msg_len - bytes_read);
                if (bytes < 0)
                {
                    break;
                }
                bytes_read += bytes;
            }

            xQueueSend(usb_recv_queue[robot_id], &packet, portMAX_DELAY);
            led_off(LED2_PIN);
        }
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    for (int i = 0; i < MAX_CONN; i++)
    {
        usb_recv_queue[i] = xQueueCreate(32, sizeof(packet_t));
    }

    usb_device_init();

    led_init( (1 << LED1_PIN) | (1 << LED2_PIN) );

    button_config_t buttons_config = {
        .pin_bit_mask = (0b1 << PAIR_PIN),
        .long_press_time_ms = 1000,
        .hold_time_ms = 2000,
        .double_press_time_ms = 250};
    int buttons = buttons_init(&buttons_config);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    int pair_immediate = 0;
    uint64_t pin_mask;
    buttons_get_state(buttons, &pin_mask);
    if (~pin_mask & (0b1 << PAIR_PIN))
    {
        ESP_LOGI("HOST", "Pair button is pressed. Hosting private AP immediately.");
        pair_immediate = 1;
        led_on(LED1_PIN);
    }

    // Init Wi-Fi
    ESP_LOGI("HOST", "Initializing Wi-Fi");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    if (!pair_immediate)
    {
        led_blink(LED1_PIN, 100);
        wifi_config_t wifi_sta_config;
        init_sta(&wifi_sta_config);
        wait_for_no_hosts();
    }

    // Init AP config
    wifi_config_t wifi_ap_config;
    if (pair_immediate)
    {
        wifi_ap_config.ap.ssid_hidden = 1;
    }
    init_ap(&wifi_ap_config);

    // Once the MBotWireless AP is started, wait for a client to connect to the AP and the socket
    button_event_t event = {0};
    if (!pair_immediate)
    {
        led_on(LED1_PIN);
        ESP_LOGI("HOST", "Press the pair button once all clients are connected.");
        while (event.pin != PAIR_PIN && event.type != SHORT_PRESS)
        {
            buttons_wait_for_event(buttons, &event, portMAX_DELAY);
        }
        event.pin = -1;
        event.type = NO_PRESS;

        if (num_connections_ap == 0)
        {
            ESP_LOGW("HOST", "Warning! No clients connected.");
        }

        wifi_ap_config.ap.ssid_hidden = 1;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    }

    server = socket_server_init(8000);

    if (!pair_immediate)
    {
        while (event.pin != PAIR_PIN && event.type != SHORT_PRESS)
        {
            buttons_wait_for_event(buttons, &event, portMAX_DELAY);
        }
        event.pin = -1;
        event.type = NO_PRESS;
    }
    led_off(LED1_PIN);

    xTaskCreate(usb_task, "usb_task", 4096, NULL, 5, NULL);
    xTaskCreate(socket_task, "socket_task", 4096, NULL, 4, NULL);
}