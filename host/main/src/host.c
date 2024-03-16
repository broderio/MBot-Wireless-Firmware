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
#include "pairing.h"
#include "serializer.h"
#include "lcm_types.h"

#include "host.h"

#define MAX_EMPTY_READS 64
#define BUFFER_SIZE 64

typedef struct connection_args_t
{
    socket_connection_t *connection;
    uint8_t robot_id;
} connection_args_t;

static connection_args_t connections[AP_MAX_CONN];
static int num_connections_socket = 0;

static host_state_t state;

socket_server_t *server;

QueueHandle_t usb_recv_queue[AP_MAX_CONN];

static EventGroupHandle_t control_mode_event_group;

static uint8_t curr_robot_id = 0;

static led_t *led1;
static led_t *led2;
static button_t *pair_btn;
static button_t *pilot_btn;
static joystick_t *js;

void connection_task(void *args)
{
    if (args == NULL)
    {
        ESP_LOGE("HOST", "Error: NULL connection args.");
        vTaskDelete(NULL);
    }

    connection_args_t* conn_args = (connection_args_t *)args;
    socket_connection_t *connection = conn_args->connection;
    uint8_t robot_id = conn_args->robot_id;
    uint8_t trigger = 0x0;

    while (true)
    {
        packet_t usb_packet;
        if (xQueueReceive(usb_recv_queue[robot_id], &usb_packet, 0) == pdTRUE)
        {
            uint32_t bytes_sent = socket_connection_send(connection, usb_packet.data, usb_packet.len);
            free(usb_packet.data);
            if (socket_connection_is_closed(connection))
            {
                goto end;
            }
            ESP_LOGI("HOST", "Sent %lu bytes to client to id %d", bytes_sent, robot_id);
        }

        uint32_t bytes_read = socket_connection_recv(connection, &trigger, 1);
        if (socket_connection_is_closed(connection))
        {
            goto end;
        }
        if (trigger == SYNC_FLAG)
        {
            ESP_LOGI("HOST", "Received SYNC_FLAG from client with id %d", robot_id);
            uint8_t header[ROS_HEADER_LEN];
            header[0] = SYNC_FLAG;

            bytes_read = 0;
            while (bytes_read < ROS_HEADER_LEN - 1)
            {
                bytes_read += socket_connection_recv(connection, header + 1 + bytes_read, ROS_HEADER_LEN - 1 - bytes_read);
                if (socket_connection_is_closed(connection))
                {
                    goto end;
                }
            }

            uint16_t msg_len = header[2] + ((uint16_t)header[3] << 8);
            uint16_t topic = header[5] + ((uint16_t)header[6] << 8);
            uint8_t cs = header[4];
            uint8_t version = header[1];

            if (cs != checksum(header + 2, 2)) {
                ESP_LOGE("SOCKET_TASK", "Error: Checksum over message length failed.");
                goto delay;
            }
            // else if (version != VERSION_FLAG) {
            //     ESP_LOGE("SOCKET_TASK", "Error: Version flag is incompatible.");
            //     goto delay;
            // }

            uint8_t *packet = (uint8_t *)malloc(msg_len + 4 + ROS_PKG_LEN);

            packet[0] = SYNC_FLAG;
            packet[1] = robot_id;
            packet[2] = (msg_len + ROS_PKG_LEN) & 0xFF; // LSB
            packet[3] = ((msg_len + ROS_PKG_LEN) >> 8) & 0xFF; // MSB

            memcpy(packet + 4, header, ROS_HEADER_LEN);

            bytes_read = 0;
            while (bytes_read < msg_len + 1)
            {
                bytes_read += socket_connection_recv(connection, packet + 4 + ROS_HEADER_LEN + bytes_read, msg_len + 1 - bytes_read);
                if (socket_connection_is_closed(connection))
                {
                    goto end;
                }
            }
            // usb_device_send(packet, msg_len + 4 + ROS_PKG_LEN);
            ESP_LOGI("HOST", "Received %d bytes from client with id %d", msg_len + 4 + ROS_PKG_LEN, robot_id);
            free(packet);
        }
        delay:
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }
    end:
    ESP_LOGW("HOST", "Client disconnected. Closing connection...");
    num_connections_socket--;
    socket_connection_free(connection);
    connections[robot_id].connection = NULL;
    vTaskDelete(NULL);
}

void socket_task(void *args)
{
    while (true)
    {
        socket_connection_t *connection = socket_server_accept(server);
        if (connection != NULL)
        {
            if (num_connections_socket == AP_MAX_CONN) {
                socket_connection_close(connection);
                ESP_LOGI("HOST", "Max number of connections reached.");
                goto delay;
            }

            ESP_LOGI("HOST", "Client connected.");

            for (int i = 0; i < AP_MAX_CONN; ++i) {
                if (connections[i].connection == NULL) {
                    connections[i].connection = connection;
                    connections[i].robot_id = i;
                    ESP_LOGI("HOST", "Creating connection task for client with id %d", i);
                    num_connections_socket++;
                    xTaskCreate(connection_task, "connection_task", 4096, (void *)&connections[i], 5, NULL);
                    break;
                }
            }
        }
        delay:
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

// This task will read input data from the USB and push it to the corresponding USB queue
// Packet structure: [SYNC_FLAG, ROBOT_ID, MSG_LEN, [PACKET]]
void serial_task(void *args)
{
    led_on(led2);
    uint8_t trigger = 0x0;
    while (true)
    {
        if (xEventGroupGetBits(control_mode_event_group) & SERIAL_STOP)
        {
            xEventGroupClearBits(control_mode_event_group, SERIAL_STOP);
            state = PILOT;
            xTaskCreate(pilot_task, "pilot_task", 4096, NULL, 4, NULL);
            vTaskDelete(NULL);
        }

        int bytes_read = usb_device_read(&trigger, 1);
        if (bytes_read < 0)
        {
            ESP_LOGE("SERIAL_TASK", "Error: Failed to read data from USB.");
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
                ESP_LOGE("SERIAL_TASK", "Error: Failed to send packet to message queue.");
                free(packet.data);
            }
        }
    }
}

void pilot_task(void *args)
{
    led_off(led2);
    serial_twist2D_t vel_cmd = {0};
    float vx_prev = 0.0, wz_prev = 0.0;
    while (true)
    {
        if (xEventGroupGetBits(control_mode_event_group) & PILOT_STOP)
        {
            xEventGroupClearBits(control_mode_event_group, PILOT_STOP);
            state = SERIAL; 
            xTaskCreate(serial_task, "serial_task", 4096, NULL, 4, NULL);
            vTaskDelete(NULL);
        }

        vel_cmd.utime = esp_timer_get_time();
        joystick_read(js);
        float vx = joystick_get_y(js);
        float wz = joystick_get_x(js);

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
        if (packet.data == NULL)
        {
            ESP_LOGE("PILOT_TASK", "Error: Failed to allocate memory for packet.");
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

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

void heartbeat_task(void *args) 
{
    serial_timestamp_t timestamp = {0};
    TickType_t xLastWakeTime;
    while (true)
    {
        xLastWakeTime = xTaskGetTickCount();

        for (int i = 0; i < AP_MAX_CONN; i++) {
            if (connections[i].connection == NULL) {
                continue;
            }

            timestamp.utime = esp_timer_get_time();
            uint8_t msg[sizeof(serial_timestamp_t)];
            timestamp_t_serialize(&timestamp, msg);

            packet_t packet;
            packet.len = sizeof(serial_timestamp_t) + ROS_PKG_LEN;
            packet.data = (uint8_t *)malloc(packet.len);
            if (packet.data == NULL)
            {
                ESP_LOGE("HEARTBEAT_TASK", "Error: Failed to allocate memory for packet.");
                goto delay;
            }

            encode_rospkt(msg, sizeof(serial_timestamp_t), MBOT_TIMESYNC, packet.data);

            BaseType_t err = xQueueSend(usb_recv_queue[i], &packet, portMAX_DELAY);
            if (err != pdTRUE)
            {
                ESP_LOGE("HEARTBEAT_TASK", "Error: Failed to send packet to message queue.");
                free(packet.data);
            }
        }

        delay:
        xTaskDelayUntil(&xLastWakeTime, 500 / portTICK_PERIOD_MS);
    }
}

pair_config_t pair_wifi() {
    uint8_t ack_recv;
    int bytes_read = 0;
    while (!bytes_read) {
        bytes_read = usb_device_read(&ack_recv, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    pair_config_t pair_cfg = get_pair_config();
    
    button_wait_for_press(pair_btn);
    button_wait_for_release(pair_btn);

    if (strlen(pair_cfg.ssid) >= 3 || strlen(pair_cfg.password) != 0) {
        usb_device_send((uint8_t*)pair_cfg.ssid, sizeof(pair_cfg.ssid));
    }

    pair_config_t new_pair_cfg = {0};
    uint8_t ack_send = 0xFF;
    while (true) {
        usb_device_send(&ack_send, 1);

        vTaskDelay(500 / portTICK_PERIOD_MS);
        
        uint8_t buffer[sizeof(pair_config_t)];
        usb_device_read(buffer, sizeof(pair_config_t));
        memcpy(&new_pair_cfg, buffer, sizeof(pair_config_t));

        vTaskDelay(500 / portTICK_PERIOD_MS);

        usb_device_send(buffer, sizeof(pair_config_t));

        vTaskDelay(500 / portTICK_PERIOD_MS);
        
        usb_device_read(&ack_recv, 1);

        if (ack_recv == 0xFF) {
            break;
        }
    }
    set_pair_config(&new_pair_cfg);
    return new_pair_cfg;
}

void pilot_btn_callback(void* args) {
    if (state == PILOT) {
        xEventGroupSetBitsFromISR(control_mode_event_group, PILOT_STOP, NULL);
    }
    else if (state == SERIAL) {
        xEventGroupSetBitsFromISR(control_mode_event_group, SERIAL_STOP, NULL);
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

    for (int i = 0; i < AP_MAX_CONN; i++)
    {
        usb_recv_queue[i] = xQueueCreate(128, sizeof(packet_t));
    }

    control_mode_event_group = xEventGroupCreate();

    // usb_device_init(); 

    js = joystick_create(JOYSTICK_X_PIN, JOYSTICK_Y_PIN);

    led1 = led_create(LED1_PIN);
    led2 = led_create(LED2_PIN);

    pair_btn = button_create(PAIR_PIN);
    button_interrupt_enable(pair_btn);

    pilot_btn = button_create(PILOT_PIN);
    button_interrupt_enable(pilot_btn);

    wifi_init_config_t *wifi_cfg = wifi_start();

    int pairing_mode = 0;
    if (button_is_pressed(pair_btn))
    {
        ESP_LOGI("HOST", "Pair button is pressed. Going into pairing mode.");
        pairing_mode = 1;
        button_wait_for_release(pair_btn);
    }

    pair_config_t pair_cfg = {0};
    if (pairing_mode) {
        led_blink(led1, 500);
        pair_cfg = pair_wifi();
        led_stop_blink(led1);
    }
    else {
        pair_cfg = get_pair_config();
    }

    ESP_LOGI("HOST", "Pairing config: SSID: %s, Password: %s", pair_cfg.ssid, pair_cfg.password);

    wifi_config_t *wifi_ap_cfg = access_point_init(pair_cfg.ssid, pair_cfg.password, AP_CHANNEL, AP_IS_HIDDEN, AP_MAX_CONN);

    server = socket_server_create(AP_PORT);
    xTaskCreate(socket_task, "socket_task", 4096, NULL, 4, NULL);

    xTaskCreate(heartbeat_task, "heartbeat_task", 4096, NULL, 5, NULL);

    state = PILOT;
    xTaskCreate(pilot_task, "pilot_task", 4096, NULL, 4, NULL);

    button_set_held_threshold(pilot_btn, 1000);
    button_on_pressed_for(pilot_btn, pilot_btn_callback, NULL);
}