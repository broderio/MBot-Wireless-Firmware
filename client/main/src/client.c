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
#include "sockets.h"
#include "uart.h"
#include "serializer.h"
#include "lcm_types.h"
#include "lidar.h"
#include "camera.h"
#include "usb_device.h"
#include "pairing.h"

#include "client.h"

static QueueHandle_t message_queue;

static EventGroupHandle_t connection_event_group;
static EventGroupHandle_t lidar_event_group;
static EventGroupHandle_t camera_event_group;

static SemaphoreHandle_t lidar_sem;

static uint16_t ranges[360];

static button_t *pair_btn;

socket_client_t *client;

uart_t uart;

void tasks_init(void) {
    message_queue = xQueueCreate(64, sizeof(packet_t));

    connection_event_group = xEventGroupCreate();
    lidar_event_group = xEventGroupCreate();

    lidar_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(lidar_sem);
}

typedef struct sender_task_args {
    socket_client_t *client;
    uart_t *uart;
} sender_task_args_t;

void sender_task(void *args) {
    sender_task_args_t *sender_args = (sender_task_args_t *)args;
    socket_client_t *client = sender_args->client;
    uart_t *uart_ptr = sender_args->uart;

    while (true) {
        if (xEventGroupGetBits(connection_event_group) & DISCONNECT) {
            ESP_LOGI("SENDER_TASK", "Waiting for reconnection.");
            xQueueReset(message_queue);
            vTaskDelete(NULL);
        }

        packet_t message;
        BaseType_t err = xQueueReceive(message_queue, &message, 100 / portTICK_PERIOD_MS);
        if (err != pdTRUE) {
            continue;
        }

        switch (message.dest) {
            case HOST:
                ESP_LOGI("SENDER_TASK", "Sending message to host of length: %d", message.len);
                uint32_t bytes_sent = socket_client_send(client, message.data, message.len);
                break;
            case MBOT:
                ESP_LOGI("SENDER_TASK", "Sending message to mbot of length: %d", message.len);
                uart_write(*uart_ptr, (char*)message.data, message.len);
                break;
            default:
                break;
        }
        free(message.data);
    }
    free(sender_args);
}

void mbot_task(void *args) {
    uart_t *uart_ptr = (uart_t *)args;

    while (true) {
        if (xEventGroupGetBits(connection_event_group) & DISCONNECT) {
            ESP_LOGI("MBOT_TASK", "Waiting for reconnection.");
            vTaskDelete(NULL);
        }

        uint8_t header[ROS_HEADER_LEN] = {0};

        int bytes_read = uart_read(*uart_ptr, (char*)header, 1, 0);

        if (bytes_read == 0)
        {
            goto delay;
        }
        else if (bytes_read < 0)
        {
            ESP_LOGE("MBOT_TASK", "UART read error");
            goto delay;
        }
        else if (header[0] == SYNC_FLAG)
        {
            bytes_read = uart_read(*uart_ptr, (char*)(header + 1), ROS_HEADER_LEN - 1, 0);
            if (bytes_read < 0) {
                ESP_LOGE("MBOT_TASK", "UART read error");
                goto delay;
            }

            uint16_t msg_len = header[2] + (header[3] << 8);
            uint16_t topic = header[5] + (header[6] << 8);
            uint8_t cs = header[4];
            uint8_t version = header[1];

            if (version != VERSION_FLAG) {
                ESP_LOGE("MBOT_TASK", "Error: Version flag is incompatible.");
                goto delay;
            }
            else if (cs != checksum(header + 2, 2)) {
                ESP_LOGE("MBOT_TASK", "Error: Checksum over message length failed.");
                goto delay;
            }

            packet_t packet;
            packet.dest = HOST;
            packet.len = msg_len + ROS_PKG_LEN;
            packet.data = (uint8_t *)malloc(packet.len);
            if (packet.data == NULL) {
                ESP_LOGE("MBOT_TASK", "Error: Failed to allocate memory for packet.");
                goto delay;
            }

            memcpy(packet.data, header, ROS_HEADER_LEN);

            bytes_read = uart_read(*uart_ptr, (char *)(packet.data + ROS_HEADER_LEN), msg_len + 1, 0);

            if (bytes_read < 0) {
                ESP_LOGE("MBOT_TASK", "UART read error");
                goto delay;
            }
            
            BaseType_t err = xQueueSend(message_queue, &packet, 50 / portTICK_PERIOD_MS);
            if (err != pdTRUE) {
                ESP_LOGE("MBOT_TASK", "Error: Failed to send packet to message queue.");
                free(packet.data);
            }
            delay:
            vTaskDelay(25 / portTICK_PERIOD_MS);
        }
    }
}

void socket_task(void *args) {
    socket_client_t *client = (socket_client_t *)args;
    while (true) {
        if (xEventGroupGetBits(connection_event_group) & DISCONNECT) {
            ESP_LOGI("SOCKET_TASK", "Waiting for reconnection.");
            vTaskDelete(NULL);
        }

        uint8_t header[ROS_HEADER_LEN] = {0};
        uint32_t bytes_read = socket_client_recv(client, header, 1);

        if (bytes_read == 0)
        {
            goto delay;
        }
        else if (header[0] == SYNC_FLAG)
        {
            bytes_read = 0;
            do {
                bytes_read += socket_client_recv(client, header + 1 + bytes_read, ROS_HEADER_LEN - 1 - bytes_read);
            } while (bytes_read < ROS_HEADER_LEN - 1);

            uint16_t msg_len = header[2] + ((uint16_t)header[3] << 8);
            uint16_t topic = header[5] + ((uint16_t)header[6] << 8);
            uint8_t cs = header[4];
            uint8_t version = header[1];

            if (version != VERSION_FLAG) {
                ESP_LOGE("SOCKET_TASK", "Error: Version flag is incompatible.");
                goto delay;
            }
            else if (cs != checksum(header + 2, 2)) {
                ESP_LOGE("SOCKET_TASK", "Error: Checksum over message length failed.");
                goto delay;
            }

            packet_t packet;
            packet.dest = MBOT;
            packet.len = msg_len + ROS_PKG_LEN;
            packet.data = (uint8_t *)malloc(packet.len);
            if (packet.data == NULL) {
                ESP_LOGE("SOCKET_TASK", "Error: Failed to allocate memory for packet.");
                goto delay;
            }

            memcpy(packet.data, header, ROS_HEADER_LEN);

            bytes_read = 0;
            do {
                bytes_read += socket_client_recv(client, packet.data + ROS_HEADER_LEN + bytes_read, msg_len + 1 - bytes_read);
            } while (bytes_read < msg_len + 1);

            ESP_LOGI("SOCKET_TASK", "Received message from host of length: %d", packet.len);
            BaseType_t err = xQueueSend(message_queue, &packet, 50 / portTICK_PERIOD_MS);
            if (err != pdTRUE) {
                ESP_LOGE("SOCKET_TASK", "Error: Failed to send packet to message queue.");
                free(packet.data);
            }
            delay:
            vTaskDelay(25 / portTICK_PERIOD_MS);
        }
    }
}

void lidar_read_task(void *args) {
    ESP_LOGI("LIDAR_TASK", "Starting lidar task");
    while (1)
    {
        lidar_t *lidar = lidar_create(UART_NUM_0, LIDAR_TX_PIN, LIDAR_RX_PIN, LIDAR_PWM_PIN);
        if (lidar == NULL)
        {
            ESP_LOGE("LIDAR_TASK", "Error initializing lidar");
            xEventGroupSetBits(lidar_event_group, LIDAR_INIT);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        start:
        uint8_t error = lidar_start_motor(lidar, LIDAR_DEFAULT_MOTOR_PWM);
        if (error)
        {
            ESP_LOGE("LIDAR_TASK", "Error starting motor");
            lidar_free(lidar);
            continue;
        }

        error = lidar_start_scan(lidar, SCAN_TYPE_EXPRESS);
        if (error)
        {
            ESP_LOGE("LIDAR_TASK", "Error starting scan");
            xEventGroupSetBits(lidar_event_group, LIDAR_START_SCAN);
            lidar_free(lidar);
            continue;
        }

        while (1) 
        {
            if (xEventGroupGetBits(connection_event_group) & DISCONNECT)
            {
                lidar_stop_scan(lidar);
                lidar_free(lidar);
                ESP_LOGI("LIDAR_READ_TASK", "Waiting for reconnection.");
                vTaskDelete(NULL);
            }

            xSemaphoreTake(lidar_sem, portMAX_DELAY);
            error = lidar_get_scan_360(lidar, ranges);
            xSemaphoreGive(lidar_sem);

            if (error != 0)
            {
                ESP_LOGE("LIDAR_TASK", "Error getting scan");
                lidar_stop_scan(lidar);
                lidar_reset(lidar);
                goto start;
            }
        }
    }
}

void lidar_task(void* args) {
    TickType_t xLastWakeTime;
    serial_lidar_scan_t scan = {0};
    while (1) {
        if (xEventGroupGetBits(connection_event_group) & DISCONNECT) {
            ESP_LOGI("LIDAR_TASK", "Waiting for reconnection.");
            vTaskDelete(NULL);
        }

        xLastWakeTime = xTaskGetTickCount();
        scan.utime = esp_timer_get_time();
        xSemaphoreTake(lidar_sem, portMAX_DELAY);
        memcpy(scan.ranges, ranges, sizeof(ranges));
        xSemaphoreGive(lidar_sem);

        packet_t packet;
        packet.dest = HOST;
        packet.len = sizeof(serial_lidar_scan_t) + ROS_PKG_LEN;
        packet.data = (uint8_t *)malloc(packet.len);
        if (packet.data == NULL)
        {
            ESP_LOGE("LIDAR_TASK", "Error: Failed to allocate memory for packet.");
            goto delay;
        }

        encode_rospkt((uint8_t*)&scan, sizeof(serial_lidar_scan_t), MBOT_LIDAR_SCAN, packet.data);
        BaseType_t err = xQueueSend(message_queue, &packet, 100 / portTICK_PERIOD_MS);
        if (err != pdTRUE)
        {
            ESP_LOGE("LIDAR_TASK", "Error: Failed to send packet to message queue.");
            free(packet.data);
        }

        delay:
        xTaskDelayUntil(&xLastWakeTime, 100 / portTICK_PERIOD_MS);
    }
}

void camera_task(void*) {
    camera_pins_t camera_pins = {
        .xclk = CAM_MCLK_PIN,
        .sda = CAM_SDA_PIN,
        .scl = CAM_SCL_PIN,
        .d7 = CAM_D7_PIN,
        .d6 = CAM_D6_PIN,
        .d5 = CAM_D5_PIN,
        .d4 = CAM_D4_PIN,
        .d3 = CAM_D3_PIN,
        .d2 = CAM_D2_PIN,
        .d1 = CAM_D1_PIN,
        .d0 = CAM_D0_PIN,
        .vsync = CAM_VSYNC_PIN,
        .href = CAM_HSYNC_PIN,
        .pclk = CAM_PCLK_PIN
    };
    while (1)
    {
        TickType_t xLastWakeTime;
        camera_fb_t *frame;
        esp_err_t esp_err = camera_init(&camera_pins);
        if (esp_err != ESP_OK)
        {
            ESP_LOGE("CAMERA_TASK", "Error initializing camera");
            xEventGroupSetBits(camera_event_group, CAMERA_INIT);
            vTaskDelete(NULL);
        }
        while (1) 
        {
            xLastWakeTime = xTaskGetTickCount();

            if (xEventGroupGetBits(connection_event_group) & DISCONNECT)
            {
                camera_deinit();
                ESP_LOGE("CAMERA_TASK", "Disconnected from server");
                vTaskDelete(NULL);
            }

            camera_capture_frame(&frame);

            size_t frame_size = frame->len;
            serial_camera_frame_t *msg = (serial_camera_frame_t *)malloc(sizeof(serial_camera_frame_t) + frame_size);
            if (msg == NULL)
            {
                ESP_LOGE("CAMERA_TASK", "Error: Failed to allocate memory for frame data.");
                xEventGroupSetBits(camera_event_group, CAMERA_MALLOC);
                camera_return_frame(frame);
                goto delay;
            }
            memcpy(msg->data, frame->buf, frame_size);
            msg->utime = esp_timer_get_time();
            msg->width = frame->width;
            msg->height = frame->height;
            msg->format = frame->format;

            packet_t packet;
            packet.dest = HOST;
            packet.len = sizeof(serial_camera_frame_t) + frame_size + ROS_PKG_LEN;
            packet.data = (uint8_t *)malloc(packet.len);
            if (packet.data == NULL)
            {
                ESP_LOGE("CAMERA_TASK", "Error: Failed to allocate memory for packet.");
                xEventGroupSetBits(camera_event_group, CAMERA_MALLOC);
                camera_return_frame(frame);
                goto delay;
            }
            
            encode_rospkt((uint8_t*)msg, sizeof(serial_camera_frame_t) + frame_size, MBOT_CAMERA_FRAME, packet.data);
            free(msg);

            BaseType_t rtos_err = xQueueSend(message_queue, &packet, 50 / portTICK_PERIOD_MS);
            if (rtos_err != pdTRUE) {
                ESP_LOGE("CAMERA_TASK", "Error: Failed to send packet to message queue.");
                free(packet.data);
            }

            camera_return_frame(frame);

            delay:
            xTaskDelayUntil(&xLastWakeTime, 100 / portTICK_PERIOD_MS);
        }
    }
}

void heartbeat_task(void *args)
{
    serial_timestamp_t timestamp = {0};
    TickType_t xLastWakeTime;
    while (true)
    {
        xLastWakeTime = xTaskGetTickCount();
        if (xEventGroupGetBits(connection_event_group) & DISCONNECT) {
            ESP_LOGI("HEARTBEAT_TASK", "Waiting for reconnection.");
            vTaskDelete(NULL);
        }

        timestamp.utime = esp_timer_get_time();
        uint8_t msg[sizeof(serial_timestamp_t)];
        timestamp_t_serialize(&timestamp, msg);

        packet_t packet;
        packet.dest = MBOT;
        packet.len = sizeof(serial_timestamp_t) + ROS_PKG_LEN;
        packet.data = (uint8_t *)malloc(packet.len);
        if (packet.data == NULL)
        {
            ESP_LOGE("HEARTBEAT_TASK", "Error: Failed to allocate memory for packet.");
            goto delay;
        }

        encode_rospkt(msg, sizeof(serial_timestamp_t), MBOT_TIMESYNC, packet.data);

        BaseType_t err = xQueueSend(message_queue, &packet, 50 / portTICK_PERIOD_MS);
        if (err != pdTRUE)
        {
            ESP_LOGE("HEARTBEAT_TASK", "Error: Failed to send packet to message queue.");
            free(packet.data);
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

    tasks_init();

    // usb_device_init();

    uart = UART_NUM_1;
    uart_init(uart, RC_TX_PIN, RC_RX_PIN, 115200, 1024);

    pair_btn = button_create(PAIR_PIN);
    button_interrupt_enable(pair_btn);

    wifi_init_config_t *wifi_cfg = wifi_start();

    int pairing_mode = 0;
    if (button_is_pressed(pair_btn))
    {
        ESP_LOGI("CLIENT", "Pair button is pressed. Going into pairing mode.");
        pairing_mode = 1;
        button_wait_for_release(pair_btn);
    }

    pair_config_t pair_cfg = {0};
    if (pairing_mode) {
        pair_cfg = pair_wifi();
    }
    else {
        pair_cfg = get_pair_config();
    }

    ESP_LOGI("CLIENT", "Pairing config: %s, %s", pair_cfg.ssid, pair_cfg.password);

    wifi_config_t *wifi_sta_cfg = station_init();
    station_connect(wifi_sta_cfg, pair_cfg.ssid, pair_cfg.password);
    station_wait_for_connection(-1);

    client = socket_client_create(AP_IP_ADDR, AP_PORT);
    while (client == NULL)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        client = socket_client_create(AP_IP_ADDR, AP_PORT);
    }

    TaskHandle_t sender_task_handle, lidar_task_handle, lidar_read_task_handle, socket_task_handle, mbot_task_handle, heartbeat_task_handle;

    sender_task_args_t *sender_args = (sender_task_args_t*)malloc(sizeof(sender_task_args_t));
    sender_args->client = client;
    sender_args->uart = &uart;
    xTaskCreate(sender_task, "sender_task", 8192, sender_args, 4, &sender_task_handle);

    xTaskCreate(lidar_read_task, "lidar_read_task", 8192, NULL, 5, &lidar_read_task_handle);
    xTaskCreate(lidar_task, "lidar_task", 8192, NULL, 4, &lidar_task_handle);
    xTaskCreate(socket_task, "socket_task", 8192, client, 4, &socket_task_handle);
    xTaskCreate(mbot_task, "mbot_task", 8192, &uart, 4, &mbot_task_handle);
    xTaskCreate(heartbeat_task, "heartbeat_task", 8192, NULL, 4, &heartbeat_task_handle);

    while (true) {
        if (station_is_disconnected() || socket_client_is_closed(client)) {
            ESP_LOGW("CLIENT", "Disconnected from AP. Attempting to reconnect...");
            xEventGroupSetBits(connection_event_group, DISCONNECT);
            vTaskDelay(100 / portTICK_PERIOD_MS);

             while (eTaskGetState(sender_task_handle) != eDeleted &&
                    eTaskGetState(lidar_read_task_handle) != eDeleted &&
                    eTaskGetState(lidar_task_handle) != eDeleted && 
                    eTaskGetState(socket_task_handle) != eDeleted && 
                    eTaskGetState(mbot_task_handle) != eDeleted && 
                    eTaskGetState(heartbeat_task_handle) != eDeleted)
            {
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }

            xEventGroupClearBits(connection_event_group, DISCONNECT);

            station_wait_for_connection(-1);

            socket_client_free(client);
            client = NULL;

            client = socket_client_create(AP_IP_ADDR, AP_PORT);
            while (client == NULL)
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                client = socket_client_create(AP_IP_ADDR, AP_PORT);
            }

            sender_args->client = client;
            xTaskCreate(sender_task, "sender_task", 8192, sender_args, 4, &sender_task_handle);

            xTaskCreate(lidar_read_task, "lidar_read_task", 8192, NULL, 5, NULL);
            xTaskCreate(lidar_task, "lidar_task", 8192, NULL, 4, &lidar_task_handle);
            xTaskCreate(socket_task, "socket_task", 8192, client, 4, &socket_task_handle);
            xTaskCreate(mbot_task, "mbot_task", 8192, &uart, 4, &mbot_task_handle);
            xTaskCreate(heartbeat_task, "heartbeat_task", 8192, NULL, 4, &heartbeat_task_handle);
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);  
    }
}