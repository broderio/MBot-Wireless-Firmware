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

#include "buttons.h"
#include "joystick.h"
#include "tcp_socket.h"
#include "led.h"
#include "usb_device.h"
#include "pairing.h"
#include "wifi.h"
#include "serializer.h"
#include "lcm_types.h"

#include "command_link.h"

#define MAX_EMPTY_READS 64
#define BUFFER_SIZE 64

static tcp_connection_t *connections[AP_MAX_CONN];
static int num_connections_socket = 0;

static host_state_t state;

tcp_server_t *server;

QueueHandle_t usb_recv_queue[AP_MAX_CONN];

static EventGroupHandle_t control_mode_event_group;

static uint8_t curr_robot_id = 0;

static led_t *led1;
static led_t *led2;
static button_t *pair_btn;
static button_t *pilot_btn;
static joystick_t *js;
static usb_device_t *usb_dev;

// There is a bug that is causing the socket to disconnect
// This could be one of two issues: either signal stregth/interference (which isn't something we can control, unless it is being caused by power supply issues)
// or the socket send/recv are blocking when they shouldn't be, which is causing the client socket to timeout and disconnect
// This bug is probably the result of an invalid packet being sent and then the sync flag detection takes a long time to parse the rest of the invalid packet
// This could be resolved by implementing a larger buffer for the sync flag so that it reads more bytes per recv call
// To improve speed once more, we could create a separte task for the sockets so that we poll the recv function and store the bytes into the buffer so that when a user calls
//     socket_xxx_recv(), it will just read from the buffer instead of the socket


// There is another bug where if the client disconnects and reconnects before we detect the disconnect, we assign a new robot_id to the same client before deleting its connection task
// This should be resolved by dynamically allocating connection pointers for each unique MAC address that connects to the access point
// This would allow the connection task to resume on the same client if it disconnects and reconnects without uncertainty in robot_id maintainence

uint8_t read_exact(tcp_connection_t *connection, uint8_t *buffer, uint32_t len)
{
    uint32_t bytes_read = 0;
    while (bytes_read < len)
    {
        bytes_read += tcp_connection_recv(connection, buffer + bytes_read, len - bytes_read);
        if (tcp_connection_is_closed(connection))
        {
            return 1;
        }
    }
    return 0;
}

void connection_task(void *args)
{
    tcp_connection_t *connection;
    uint8_t robot_id = 0;

    TickType_t count = 0;
    TickType_t start_time = xTaskGetTickCount();

    while (true)
    {
        while (true)
        {
            if (num_connections_socket == 0)
            {
                vTaskDelay(500 / portTICK_PERIOD_MS);
                continue;
            }

            robot_id = (robot_id + 1) % num_connections_socket;
            connection = connections[robot_id];

            packet_t usb_packet;
            if (xQueueReceive(usb_recv_queue[robot_id], &usb_packet, 0) == pdTRUE)
            {
                tcp_connection_send(connection, usb_packet.data, usb_packet.len);
                free(usb_packet.data);
                if (tcp_connection_is_closed(connection))
                {
                    goto end;
                }
                // ESP_LOGI("HOST", "Sent %lu bytes to client with id %d", bytes_sent, robot_id);
            }

            uint8_t header[ROS_HEADER_LEN];
            uint8_t err = read_exact(connection, header, ROS_HEADER_LEN);
            if (err)
            {
                goto end;
            }

            // if (header[0] != SYNC_FLAG) continue;

            // err = read_exact(connection, header + 1, ROS_HEADER_LEN - 1);
            // if (err)
            // {
            //     goto end;
            // }

            uint8_t start_idx = 0;
            uint8_t trigger = 0x0;
            for (; start_idx < ROS_HEADER_LEN; start_idx++) {
                if (header[start_idx] == SYNC_FLAG) {
                    trigger = SYNC_FLAG;
                    break;
                }
            }
            if (trigger != SYNC_FLAG) {
                continue;
            }

            // ESP_LOGI("HOST", "start_idx %d", start_idx);
            if (start_idx != 0) {
                uint8_t temp[ROS_HEADER_LEN];
                memcpy(temp, header + start_idx, ROS_HEADER_LEN - start_idx);
                err = read_exact(connection, temp + ROS_HEADER_LEN - start_idx, start_idx);
                if (err)
                {
                    goto end;
                }
                memcpy(header, temp, ROS_HEADER_LEN);
            }
            // ESP_LOGI("HOST", "header[0]: %d", header[0]);
            // ESP_LOGI("HOST", "header[1]: %d", header[1]);

            uint8_t version = header[1];
            if (version != VERSION_FLAG) {
                ESP_LOGE("server_task", "Error: Version flag is incompatible.");
                continue;
            }

            uint16_t msg_len = header[2] + ((uint16_t)header[3] << 8);
            uint16_t topic = header[5] + ((uint16_t)header[6] << 8);
            uint8_t cs = header[4];
            uint8_t calc_cs = checksum(header + 2, 2);

            if (topic == MBOT_LIDAR_SCAN) {
                count += 1;
                ESP_LOGI("HOST", "Receiving lidar at %f Hz", (float)count / (((xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS) / 1000.0));
            }

            // ESP_LOGI("HOST", "msg_len: %d", msg_len);
            // ESP_LOGI("HOST", "topic: %d", topic);
            // ESP_LOGI("HOST", "cs: %d", cs);
            // ESP_LOGI("HOST", "calc_cs: %d", calc_cs);

            if (cs != calc_cs) {
                ESP_LOGE("server_task", "Error: Checksum over message length failed.");
                continue;
            }
            // else if (version != VERSION_FLAG) {
            //     ESP_LOGE("server_task", "Error: Version flag is incompatible.");
            //     goto delay;
            // }

            uint8_t *packet = (uint8_t *)malloc(msg_len + 4 + ROS_PKG_LEN);

            packet[0] = SYNC_FLAG;
            packet[1] = robot_id;
            packet[2] = (msg_len + ROS_PKG_LEN) & 0xFF; // LSB
            packet[3] = ((msg_len + ROS_PKG_LEN) >> 8) & 0xFF; // MSB

            memcpy(packet + 4, header, ROS_HEADER_LEN);

            err = read_exact(connection, packet + 4 + ROS_HEADER_LEN, msg_len);
            if (err)
            {
                free(packet);
                goto end;
            }

            // usb_device_send(usb_dev, packet, msg_len + 4 + ROS_PKG_LEN);

            // ESP_LOGI("HOST", "Received %d bytes from client with id %d", msg_len + 4 + ROS_PKG_LEN, robot_id);
            free(packet);
            // vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        end:
        ESP_LOGW("HOST", "Client disconnected. Closing connection...");
        num_connections_socket--;
        tcp_connection_free(connection);
        connections[robot_id] = NULL;
    }
}

void server_task(void *args)
{
    while (true)
    {
        tcp_connection_t *connection = tcp_server_accept(server);
        if (connection != NULL)
        {
            if (num_connections_socket == AP_MAX_CONN) {
                tcp_connection_close(connection);
                ESP_LOGI("HOST", "Max number of connections reached.");
                vTaskDelay(500 / portTICK_PERIOD_MS);
                continue;
            }

            ESP_LOGI("HOST", "Client connected.");

            for (int i = 0; i < AP_MAX_CONN; ++i) {
                if (connections[i] == NULL) {
                    connections[i] = connection;
                    ESP_LOGI("HOST", "Creating connection task for client with id %d", i);
                    num_connections_socket++;
                    break;
                }
            }
        }
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

        int bytes_read = usb_device_read(usb_dev, &trigger, 1);
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
                int bytes = usb_device_read(usb_dev, header + 1 + bytes_read, 3 - bytes_read);
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
                int bytes = usb_device_read(usb_dev, packet.data + bytes_read, msg_len - bytes_read);
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
            if (connections[i] == NULL) {
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
            // ESP_LOGI("HEARTBEAT_TASK", "Sent heartbeat to client with id %d", i);
        }

        delay:
        xTaskDelayUntil(&xLastWakeTime, 500 / portTICK_PERIOD_MS);
    }
}

pair_config_t pair_wifi() {
    uint8_t ack_recv;
    int bytes_read = 0;
    while (!bytes_read) {
        bytes_read = usb_device_read(usb_dev, &ack_recv, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    pair_config_t pair_cfg = get_pair_config();
    
    button_wait_for_press(pair_btn);
    button_wait_for_release(pair_btn);

    if (strlen(pair_cfg.ssid) >= 3 || strlen(pair_cfg.password) != 0) {
        usb_device_send(usb_dev, (uint8_t*)pair_cfg.ssid, sizeof(pair_cfg.ssid));
    }

    pair_config_t new_pair_cfg = {0};
    uint8_t ack_send = 0xFF;
    while (true) {
        usb_device_send(usb_dev, &ack_send, 1);

        vTaskDelay(500 / portTICK_PERIOD_MS);
        
        uint8_t buffer[sizeof(pair_config_t)];
        usb_device_read(usb_dev, buffer, sizeof(pair_config_t));
        memcpy(&new_pair_cfg, buffer, sizeof(pair_config_t));

        vTaskDelay(500 / portTICK_PERIOD_MS);

        usb_device_send(usb_dev, buffer, sizeof(pair_config_t));

        vTaskDelay(500 / portTICK_PERIOD_MS);
        
        usb_device_read(usb_dev, &ack_recv, 1);

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

    // usb_dev = usb_device_create(); 

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

    for (int i = 0; i < AP_MAX_CONN; i++)
    {
        connections[i] = NULL;
    }

    server = tcp_server_create(AP_PORT);
    xTaskCreate(server_task, "server_task", 4096, NULL, 4, NULL);
    xTaskCreate(connection_task, "connection_task", 4096, NULL, 4, NULL);

    xTaskCreate(heartbeat_task, "heartbeat_task", 4096, NULL, 5, NULL);

    state = PILOT;
    xTaskCreate(pilot_task, "pilot_task", 4096, NULL, 4, NULL);

    button_set_held_threshold(pilot_btn, 1000);
    button_on_pressed_for(pilot_btn, pilot_btn_callback, NULL);
}