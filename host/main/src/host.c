#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

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

#define MAX_EMPTY_READS 64
#define BUFFER_SIZE 64

typedef struct connection_args_t
{
    connection_socket_t connection;
    uint8_t robot_id;
} connection_args_t;

static connection_args_t *connections[MAX_CONN];

server_socket_t server;

QueueHandle_t usb_recv_queue[MAX_CONN];

static EventGroupHandle_t control_mode_event_group;

static uint8_t curr_robot_id = 0;

static led_t *led;

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
    int empty_reads = 0;

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
                goto end;
            }
        }

        int bytes_read = socket_connection_recv(connection, &trigger, 1);
        if (bytes_read == 0)
        {
            empty_reads++;
            if (empty_reads >= MAX_EMPTY_READS)
            {
                ESP_LOGW("HOST", "Client %d not sending data. Assuming disconnection...", robot_id);
                goto end;
            }
            vTaskDelay(25 / portTICK_PERIOD_MS);
            continue;
        }
        else if (bytes_read < 0)
        {
            goto end;
        }
        else if (trigger == SYNC_FLAG)
        {
            empty_reads = 0;
            uint8_t header[ROS_HEADER_LEN];
            header[0] = SYNC_FLAG;

            bytes_read = 0;
            while (bytes_read < ROS_HEADER_LEN - 1)
            {
                int bytes = socket_connection_recv(connection, (char *)(header + 1 + bytes_read), ROS_HEADER_LEN - 1 - bytes_read);
                if (bytes < 0)
                {
                    ESP_LOGE("HOST", "Error receiving header from client %d", robot_id);
                    goto end;
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
                    free(packet);
                    goto end;
                }
                bytes_read += bytes;
            }
            usb_device_send(packet, msg_len + 4 + ROS_PKG_LEN);
            free(packet);
        }
    }
    end:
    ESP_LOGW("HOST", "Client disconnected. Closing connection...");
    socket_connection_close(connection);
    num_connections_socket--;
    free(conn_args);
    conn_args = NULL;
    vTaskDelete(NULL);
}

void socket_task(void *args)
{
    while (true)
    {
        connection_socket_t connection = socket_server_accept(server);
        if (connection > 0)
        {
            if (num_connections_socket == MAX_CONN) {
                socket_connection_close(connection);
                ESP_LOGI("HOST", "Max number of connections reached.");
                goto delay;
            }

            ESP_LOGI("HOST", "Client connected.");

            for (int i = 0; i < MAX_CONN; ++i) {
                if (connections[i] == NULL) {
                    connections[i] = malloc(sizeof(connection_args_t));
                    if (connections[i] == NULL) {
                        ESP_LOGE("HOST", "Error allocating memory for connection args.");
                        break;
                    }
                    connections[i]->connection = connection;
                    connections[i]->robot_id = i;
                    num_connections_socket++;
                    xTaskCreate(connection_task, "connection_task", 4096, (void *)connections[i], 5, NULL);
                    break;
                }
            }
        }
        else if (connection == -1)
        {
            ESP_LOGE("HOST", "Error accepting connection.");
        }
        delay:
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

// This task will read input data from the USB and push it to the corresponding USB queue
// Packet structure: [SYNC_FLAG, ROBOT_ID, MSG_LEN, [PACKET]]
void usb_task(void *args)
{
    led_on(led);
    uint8_t trigger = 0x0;
    while (true)
    {
        if (xEventGroupGetBits(control_mode_event_group) & SERIAL_STOP)
        {
            xEventGroupClearBits(control_mode_event_group, SERIAL_STOP);
            xEventGroupSetBits(control_mode_event_group, SERIAL_CONFIRM);
            vTaskDelete(NULL);
        }

        int bytes_read = usb_device_read(&trigger, 1);
        if (bytes_read < 0)
        {
            ESP_LOGE("USB_TASK", "Error: Failed to read data from USB.");
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        else if (bytes_read == 0)
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

            BaseType_t err = xQueueSend(usb_recv_queue[robot_id], &packet, portMAX_DELAY);
            if (err != pdTRUE)
            {
                ESP_LOGE("USB_TASK", "Error: Failed to send packet to message queue.");
                free(packet.data);
            }
        }
    }
}

void pilot_task(void *args)
{
    led_off(led);
    serial_twist2D_t vel_cmd = {0};
    float vx_prev = 0.0, wz_prev = 0.0;
    while (true)
    {
        if (xEventGroupGetBits(control_mode_event_group) & PILOT_STOP)
        {
            xEventGroupClearBits(control_mode_event_group, PILOT_STOP);
            xEventGroupSetBits(control_mode_event_group, PILOT_CONFIRM);
            vTaskDelete(NULL);
        }

        vel_cmd.utime = esp_timer_get_time();
        float vx, wz;
        joystick_get_output(&wz, &vx);

        if (fabs(vx) < 0.05) vx = 0;
        if (fabs(wz) < 0.05) wz = 0;

        if (vx == vx_prev && wz == wz_prev)
        {
            vx_prev = vx;
            wz_prev = wz;
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        vel_cmd.vx = vx * PILOT_VX_SCALAR;
        vel_cmd.wz = wz * PILOT_WZ_SCALAR;

        uint8_t msg[sizeof(serial_twist2D_t)];
        twist2D_t_serialize(&vel_cmd, msg);

        packet_t packet;
        packet.len = sizeof(serial_twist2D_t) + ROS_PKG_LEN;
        packet.data = (uint8_t *)malloc(packet.len);
        encode_rospkt(msg, sizeof(serial_twist2D_t), MBOT_VEL_CMD, packet.data);

        BaseType_t err = xQueueSend(usb_recv_queue[curr_robot_id], &packet, portMAX_DELAY);
        if (err != pdTRUE)
        {
            ESP_LOGE("PILOT_TASK", "Error: Failed to send packet to message queue.");
            free(packet.data);
        }

        vx_prev = vx;
        wz_prev = wz;
        vTaskDelay(100 / portTICK_PERIOD_MS);
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

    control_mode_event_group = xEventGroupCreate();

    // usb_device_init();

    joystick_config_t js_config = {
        .x_pin = 5,
        .y_pin = 4,
        .x_in_max = 916,
        .x_in_min = 22,
        .x_out_max = 1,
        .x_out_min = -1,
        .y_in_max = 907,
        .y_in_min = 0,
        .y_out_max = 1,
        .y_out_min = -1,
    };
    joystick_init(&js_config);

    led = led_create(LED2_PIN);

    button_t *pair_button = button_create(PAIR_PIN);
    button_interrupt_enable(pair_button);

    button_t *pilot_button = button_create(PILOT_PIN);
    button_interrupt_enable(pilot_button);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    int pairing_mode = 0;
    if (button_is_pressed(pair_button))
    {
        ESP_LOGI("HOST", "Pair button is pressed. Going into pairing mode.");
        pairing_mode = 1;
        button_wait_for_release(pair_button);
    }

    // Init Wi-Fi
    ESP_LOGI("HOST", "Initializing Wi-Fi");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    if (pairing_mode)
    {
        led_blink(led, 500);
        wifi_config_t wifi_sta_config;
        init_sta(&wifi_sta_config);
        wait_for_no_hosts();
    }

    // Init AP config
    wifi_config_t wifi_ap_config;
    if (!pairing_mode)
    {
        wifi_ap_config.ap.ssid_hidden = 1;
    }
    init_ap(&wifi_ap_config);

    // Once the MBotWireless AP is started, wait for a client to connect to the AP and the socket
    if (pairing_mode)
    {
        led_on(led);
        ESP_LOGI("HOST", "Press the pair button once all clients are connected.");
        button_wait_for_press(pair_button);

        if (num_connections_ap == 0)
        {
            ESP_LOGW("HOST", "Warning! No clients connected.");
        }

        wifi_ap_config.ap.ssid_hidden = 1;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    }

    server = socket_server_init(8000);

    if (pairing_mode)
    {
        ESP_LOGI("HOST", "Press the pair button to start the tasks.");
        button_wait_for_press(pair_button);
    }
    led_off(led);

    xTaskCreate(pilot_task, "pilot_task", 4096, NULL, 5, NULL);
    xTaskCreate(socket_task, "socket_task", 4096, NULL, 4, NULL);

    int state = 0;
    while (true)
    {
        button_wait_for_press(pilot_button);
        if (state == 0) {
            xEventGroupSetBits(control_mode_event_group, PILOT_STOP);
            xEventGroupWaitBits(control_mode_event_group, PILOT_CONFIRM, pdTRUE, pdTRUE, portMAX_DELAY);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            xTaskCreate(usb_task, "usb_task", 4096, NULL, 5, NULL);
            state = 1;
        }
        else if (state == 1) {
            xEventGroupSetBits(control_mode_event_group, SERIAL_STOP);
            xEventGroupWaitBits(control_mode_event_group, SERIAL_CONFIRM, pdTRUE, pdTRUE, portMAX_DELAY);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            xTaskCreate(pilot_task, "pilot_task", 4096, NULL, 5, NULL);
            state = 0;
        }
    }
}