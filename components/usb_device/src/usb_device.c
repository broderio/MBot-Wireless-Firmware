#include <stdio.h>

#include "tinyusb.h"
#include "tusb.h"
#include "tusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/message_buffer.h"

#include "esp_log.h"

#include "usb_device.h"

#define USB_RX_BUFFER_SIZE      2048

SemaphoreHandle_t tx_lock = NULL;

QueueHandle_t _usb_rx_queue = NULL;

void _usb_device_rx_cb(int itf, cdcacm_event_t *event) {
    uint8_t buffer[USB_RX_BUFFER_SIZE];
    int bytes_read = tud_cdc_n_read(itf, buffer, USB_RX_BUFFER_SIZE);
    if (bytes_read < 0) {
        ESP_LOGE("USB", "Failed to read from USB");
        return;
    }
    for (size_t i = 0; i < bytes_read; i++) {
        BaseType_t err = xQueueSend(_usb_rx_queue, buffer + i, 10);
        if (err != pdTRUE) {
            ESP_LOGE("USB", "Failed to write to _usb_rx_queue");
            break;
        }
    }
}

void usb_device_init(void) {
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 1024,
        .callback_rx = &_usb_device_rx_cb,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    tx_lock = xSemaphoreCreateBinary();
    if(tx_lock == NULL){
        ESP_LOGE("USB", "Failed to create tx_lock");
    }
    xSemaphoreGive(tx_lock);

    _usb_rx_queue = xQueueCreate(2048, sizeof(uint8_t));
    if(_usb_rx_queue == NULL){
        ESP_LOGE("USB", "Failed to create _usb_rx_queue");
    }
}

void usb_device_deinit(void) {
    // TODO: Implement
}

int usb_device_send(uint8_t* buffer, int size) {
    if(tx_lock == NULL || buffer == NULL || size == 0 || !tud_cdc_n_connected(TINYUSB_CDC_ACM_0)){
        return 0;
    }
    if(xPortInIsrContext()){
        BaseType_t taskWoken = false;
        if(xSemaphoreTakeFromISR(tx_lock, &taskWoken) != pdPASS){
            return 0;
        }
    } else if (xSemaphoreTake(tx_lock, portMAX_DELAY) != pdPASS){
        return 0;
    }
    size_t to_send = size, so_far = 0;
    while(to_send){
        if(!tud_cdc_n_connected(TINYUSB_CDC_ACM_0)){
            size = so_far;
            break;
        }
        size_t space = tud_cdc_n_write_available(TINYUSB_CDC_ACM_0);
        if(!space){
            tud_cdc_n_write_flush(TINYUSB_CDC_ACM_0);
            continue;
        }
        if(space > to_send){
            space = to_send;
        }
        size_t sent = tud_cdc_n_write(TINYUSB_CDC_ACM_0, buffer+so_far, space);
        if(sent){
            so_far += sent;
            to_send -= sent;
            tud_cdc_n_write_flush(TINYUSB_CDC_ACM_0);
        } else {
            size = so_far;
            break;
        }
    }
    if(xPortInIsrContext()){
        BaseType_t taskWoken = false;
        xSemaphoreGiveFromISR(tx_lock, &taskWoken);
    } else {
        xSemaphoreGive(tx_lock);
    }
    return size;
}

int usb_device_read(uint8_t* buffer, int size) {
    if(_usb_rx_queue == NULL || buffer == NULL || size == 0){
        return 0;
    }
    int so_far = 0;
    while(so_far < size){
        uint8_t byte;
        if(xQueueReceive(_usb_rx_queue, &byte, 10) == pdTRUE){
            buffer[so_far++] = byte;
        } else {
            break;
        }
    }
    return so_far;
}