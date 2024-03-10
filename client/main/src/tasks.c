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
#include "client.h"
#include "utils.h"

static QueueHandle_t message_queue;

static EventGroupHandle_t connection_event_group;
static EventGroupHandle_t lidar_event_group;
static EventGroupHandle_t camera_event_group;

static SemaphoreHandle_t lidar_sem;

static uint16_t ranges[360];

client_socket_t client;
uart_t uart;

void tasks_init(void) {
    message_queue = xQueueCreate(64, sizeof(packet_t));

    connection_event_group = xEventGroupCreate();
    lidar_event_group = xEventGroupCreate();

    lidar_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(lidar_sem);
}

typedef struct sender_task_args {
    client_socket_t *client_ptr;
    uart_t *uart_ptr;
} sender_task_args_t;

void sender_task(void *args) {
    sender_task_args_t *sender_task_args = (sender_task_args_t *)args;
    client_socket_t *client_ptr = sender_task_args->client_ptr;
    uart_t *uart_ptr = sender_task_args->uart_ptr;
    while (true) {
        packet_t message;
        xQueueReceive(message_queue, &message, portMAX_DELAY);
        // ESP_LOGI("SENDER_TASK", "Received message from queue of length: %d", message.len);

        switch (message.dest) {
            case HOST:
                // ESP_LOGI("SENDER_TASK", "Sending message to host of length: %d", message.len);
                socket_client_send(*client_ptr, (char*)message.data, message.len);
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
}

void mbot_task(void *args) {
    uart_t *uart_ptr = (uart_t *)args;

    while (true) {
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
    client_socket_t *client_ptr = (uart_t *)args;

    while (true) {
        uint8_t header[ROS_HEADER_LEN] = {0};
        int bytes_read = socket_client_recv(*client_ptr, (char*)header, 1);

        if (bytes_read == 0)
        {
            goto delay;
        }
        else if (bytes_read < 0)
        {
            ESP_LOGE("SOCKET_TASK", "Client read error");
            goto delay;
        }
        else if (header[0] == SYNC_FLAG)
        {
            bytes_read = socket_client_recv(*client_ptr, (char*)(header + 1), ROS_HEADER_LEN - 1);
            if (bytes_read < 0) {
                ESP_LOGE("SOCKET_TASK", "Client read error");
                goto delay;
            }

            uint16_t msg_len = header[2] + (header[3] << 8);
            uint16_t topic = header[5] + (header[6] << 8);
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

            bytes_read = socket_client_recv(*client_ptr, (char *)(packet.data + ROS_HEADER_LEN), msg_len + 1);
            if (bytes_read < 0) {
                ESP_LOGE("SOCKET_TASK", "Client read error");
                goto delay;
            }

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
        lidar_t lidar;
        int error = lidar_init(&lidar, LIDAR_TX_PIN, LIDAR_RX_PIN, LIDAR_PWM_PIN);
        if (error != 0)
        {
            ESP_LOGE("LIDAR_TASK", "Error initializing lidar");
            xEventGroupSetBits(lidar_event_group, LIDAR_INIT);
            goto suspend;
        }

        start:
        error = lidar_start_exp_scan(&lidar);
        if (error != 0)
        {
            ESP_LOGE("LIDAR_TASK", "Error starting scan");
            xEventGroupSetBits(lidar_event_group, LIDAR_START_SCAN);
            lidar_deinit(&lidar);
            goto suspend;
        }

        while (1) 
        {
            if (xEventGroupGetBits(connection_event_group) & DISCONNECT)
            {
                ESP_LOGE("LIDAR_TASK", "Disconnected from server");
                lidar_stop_scan(&lidar);
                lidar_deinit(&lidar);
                goto suspend;
            }

            xSemaphoreTake(lidar_sem, portMAX_DELAY);
            error = lidar_get_exp_scan_360(&lidar, ranges);
            xSemaphoreGive(lidar_sem);

            if (error != 0)
            {
                ESP_LOGE("LIDAR_TASK", "Error getting scan");
                lidar_stop_scan(&lidar);
                lidar_reset(&lidar);
                goto start;
            }
        }
        suspend:
        vTaskSuspend(NULL);
    }
}

void lidar_task(void* args) {
    xTaskCreate(lidar_read_task, "lidar_read_task", 8192, NULL, 5, NULL);
    TickType_t xLastWakeTime;
    serial_lidar_scan_t scan = {0};
    while (1) {
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
            goto suspend;
        }
        while (1) 
        {
            xLastWakeTime = xTaskGetTickCount();

            if (xEventGroupGetBits(connection_event_group) & DISCONNECT)
            {
                camera_deinit();
                ESP_LOGE("CAMERA_TASK", "Disconnected from server");
                goto suspend;
            }

            camera_capture_frame(&frame);

            size_t frame_size = frame->len;
            serial_camera_frame_t *msg = (serial_camera_frame_t *)malloc(sizeof(serial_camera_frame_t) + frame_size);
            if (msg == NULL)
            {
                ESP_LOGE("CAMERA_TASK", "Error: Failed to allocate memory for frame data.");
                xEventGroupSetBits(camera_event_group, CAMERA_MALLOC);
                camera_return_frame(frame);
                goto suspend;
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
                goto suspend;
            }
            
            encode_rospkt((uint8_t*)msg, sizeof(serial_camera_frame_t) + frame_size, MBOT_CAMERA_FRAME, packet.data);
            free(msg);

            BaseType_t rtos_err = xQueueSend(message_queue, &packet, 50 / portTICK_PERIOD_MS);
            if (rtos_err != pdTRUE) {
                ESP_LOGE("CAMERA_TASK", "Error: Failed to send packet to message queue.");
                free(packet.data);
            }

            camera_return_frame(frame);
            xTaskDelayUntil(&xLastWakeTime, 100 / portTICK_PERIOD_MS);
        }
        suspend:
        vTaskSuspend(NULL);
    }
}

void heartbeat_task(void *args)
{
    serial_timestamp_t timestamp = {0};
    while (true)
    {
        timestamp.utime = esp_timer_get_time();
        uint8_t msg[sizeof(serial_timestamp_t)];
        timestamp_t_serialize(&timestamp, msg);

        packet_t packet;
        packet.dest = MBOT;
        packet.len = sizeof(serial_timestamp_t) + ROS_PKG_LEN;
        packet.data = (uint8_t *)malloc(packet.len);
        encode_rospkt(msg, sizeof(serial_timestamp_t), MBOT_TIMESYNC, packet.data);

        BaseType_t err = xQueueSend(message_queue, &packet, 50 / portTICK_PERIOD_MS);
        if (err != pdTRUE)
        {
            ESP_LOGE("HEARTBEAT_TASK", "Error: Failed to send packet to message queue.");
            free(packet.data);
        }

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
    tasks_init();

    uart = UART_NUM_1;
    uart_init(uart, RC_TX_PIN, RC_RX_PIN, 115200, 1024);

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

    client = socket_client_init(MBOT_HOST_IP_ADDR, 8000);
    while (client < 0)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        client = socket_client_init(MBOT_HOST_IP_ADDR, 8000);
    }

    TaskHandle_t sender_task_handle, lidar_task_handle;
    sender_task_args_t sender_task_args = {&client, &uart};
    xTaskCreate(sender_task, "sender_task", 8192, &sender_task_args, 3, &sender_task_handle);
    xTaskCreate(lidar_task, "lidar_task", 8192, NULL, 4, &lidar_task_handle);
    xTaskCreate(socket_task, "socket_task", 8192, &client, 4, NULL);
    xTaskCreate(mbot_task, "mbot_task", 8192, &uart, 4, NULL);
    xTaskCreate(heartbeat_task, "heartbeat_task", 8192, NULL, 5, NULL);
}