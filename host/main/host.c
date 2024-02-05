#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "serializer.h"
#include "lcm_types.h"

#include "buttons.h"
#include "fram.h"
#include "joystick.h"

#include "host.h"

STATE state;
STATE prev_state;
uint8_t client_mac[MAC_ADDR_LEN];

void host_init() {
    int is_paired = host_get_paired_client(client_mac);
    state = is_paired ? PILOT : PAIR;

    state_change_sem = xSemaphoreCreateBinary();
    suspend_task_sem = xSemaphoreCreateBinary();
    suspend_task_confirm_sem = xSemaphoreCreateBinary();

    client_send_queue = xQueueCreate(5, sizeof(wifi_packet_t));
    ui_send_queue = xQueueCreate(5, sizeof(usb_packet_t));

    buttons_init();
    fram_init();
    wifi_init();
    usb_device_init();

    // TODO: Configure joystick
    joystick_config_t js_config = {0};
    joystick_init(&js_config);

    gpio_config_t led_config = {
        .pin_bit_mask = (0b1 << LED1_PIN) | (0b1 << LED2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&led_config);

    gpio_isr_handler_add(PAIR_PIN, host_pair_button_isr, NULL);
    gpio_isr_handler_add(PILOT_PIN, host_pilot_button_isr, NULL);
}

void host_deinit() {
    // TODO: Implement
}

int host_get_paired_client(uint8_t* mac) {
    // TODO: Implement once FRAM is done
}

void host_update_state(HOST_STATE new_state) {
    if (new_state == state) return;
    
    prev_state = state;
    state = new_state;
    xSemaphoreGive(state_change_sem);
}

void host_send_timesync() {
    serial_timestamp_t tsync = {
        .utime = esp_timer_get_time();
    };

    wifi_packet_t packet = {0};
    memcpy(packet.mac, client_mac, MAC_ADDR_LEN)
    packet.len = sizeof(serial_twist2D_t);
    packet.data = malloc(packet.len);
    timestamp_t_serialize(&tsync, packet.data);
    xQueueSend(client_send_queue, &packet, portMAX_DELAY);
}

void host_read_packet(wifi_packet_t* packet) {
    uint8_t trigger_val = 0x00;
    while (trigger_val != 0xff) {
        usb_read_bytes(&trigger_val, 1, 1);
    }

    uint8_t pkt_len[2] = {0};
    usb_read_bytes(pkt_len, 2, 10);
    usb_read_bytes(packet->mac, MAC_ADDR_LEN, 10);

    packet->len = pkt_len[0] << 8 | pkt_len[1];
    packet->data = malloc(packet->len);
    usb_read_bytes(packet->data, packet->len, 10);
}

void host_pilot_button_isr() {
    if (state == PAIR) return;

    HOST_STATE new_state = (state == PILOT) ? SERIAL : PILOT;
    host_update_state(new_state);
}

void host_pair_button_isr() {
    HOST_STATE new_state = (state != PAIR) ? PAIR : prev_state;
    host_update_state(new_state);
}

void host_pilot_task() {
    while (true) {
        if (xSemaphoreTake(suspend_task_sem, 1) == pdTRUE) {
            xSemaphoreGive(suspend_task_confirm_sem);
            vTaskSuspend(NULL);
        }
        serial_twist2D_t vel_cmd = {0};      
        joystick_get_output(&vel_cmd.wz, &vel_cmd.vx);

        wifi_packet_t packet = {0};
        memcpy(packet.mac, client_mac, MAC_ADDR_LEN)
        packet.len = sizeof(serial_twist2D_t);
        packet.data = malloc(packet.len);
        twist2D_t_serialize(vel_cmd, packet.data);

        xQueueSend(client_send_queue, &packet, portMAX_DELAY);
    }
}

void host_serial_task() {
    while (true) {
        if (xSemaphoreTake(suspend_task_sem, 1) == pdTRUE) {
            xSemaphoreGive(suspend_task_confirm_sem);
            vTaskSuspend(NULL);
        }
        wifi_packet_t packet = {0};
        host_read_packet(&packet);
        xQueueSend(client_send_queue, &packet, portMAX_DELAY);
    }
}

void host_pair_task() {
    TickType_t last_wake_time;
    int led_state = 0;
    while (true) {
        last_wake_time = xTaskGetTickCount();
        
        if (xSemaphoreTake(suspend_task_sem, 1) == pdTRUE) {
            xSemaphoreGive(suspend_task_confirm_sem);
            vTaskSuspend(NULL);
        }

        // TODO: Implement

        led_state = !led_state;
        gpio_set_level(LED2_PIN, led_state);
        vTaskDelayUntil(last_wake_time, pdMS_TO_TICKS(250));
    }
}

void host_read_wifi_task() {
    while (true) {
        wifi_packet_t in_packet = {0};
        int timeout = wifi_read_blocking(&in_packet);

        usb_packet_t out_packet = {0};
        
        // out_packet.len = in_packet.len + MAC_ADDR_LEN + 3;
        // out_packet.data = malloc(out_packet.len);
        // out_packet.data[0] = SYNC_FLAG;
        // packet[1] = (uint8_t)((in_packet.len - 1) % 255);
        // packet[2] = (uint8_t)((in_packet.len - 1) >> 8);
        // memcpy(packet + 3, evt.mac_addr, MAC_ADDR_LEN);
        // memcpy(packet + MAC_ADDR_LEN + 3, in_packet.data, in_packet.len);
        
        // TODO: Restrucutre packet types to support LIDAR, Camera, and MBot data

        xQueueSend(ui_send_queue, &out_packet, portMAX_DELAY);
    }
}

void host_send_client_task() {
    while (true) {
        wifi_packet_t wifi_packet = {0};
        if (xQueueReceive(client_send_queue, &wifi_packet, portMAX_DELAY) == pdTRUE) {
            wifi_send(&wifi_packet);
            free(wifi_packet.data);
        }
    }
}

void host_send_ui_task() {
    while (true) {
        usb_packet_t usb_packet = {0};
        if (xQueueReceive(ui_send_queue, &usb_packet, portMAX_DELAY) == pdTRUE) {
            usb_device_send(&usb_packet);
            free(usb_packet.data);
        }
    }
}

void host_main_task() {
    TickType_t last_wake_time;
    while (true) {
        last_wake_time = xTaskGetTickCount();

        // Check for state change
        if (xSemaphoreTake(state_change_sem, pdMS_TO_TICKS(100)) == pdTRUE) {

            xSemaphoreGive(suspend_task_sem);
            if (xSemaphoreTake(suspend_task_confirm_sem, pdMS_TO_TICKS(1000)) == pdFALSE) {
                host_update_state(ERROR);
            }

            switch (state) {
                case PILOT:
                    host_pilot_task();
                    break;
                case SERIAL:
                    host_serial_task();
                    break;
                case PAIR:
                    host_pair_task();
                    break;
                case ERROR:
                    host_deinit();
                    break;
                default:
                    break;
            }
        }

        // Send timesync message to client
        host_send_timesync();

        vTaskDelayUntil(last_wake_time, pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    host_init();

    xTaskCreate(host_main_task, "host_main_task", 4096, NULL, 5, NULL);
    xTaskCreate(host_send_client_task, "host_send_client_task", 4096, NULL, 5, NULL);
    xTaskCreate(host_send_ui_task, "host_send_ui_task", 4096, NULL, 5, NULL);
}
