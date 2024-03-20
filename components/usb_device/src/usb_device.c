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

#define USB_RX_BUFFER_SIZE 2048

struct usb_device_t
{
    SemaphoreHandle_t _tx_lock;
    QueueHandle_t _rx_queue;
    tinyusb_cdcacm_itf_t _cdcacm_itf;
};

usb_device_t usb_devs[2];
uint8_t usb_dev_count = 0;

void _usb_device_rx_cb(int itf, cdcacm_event_t *event)
{
    uint8_t buffer[USB_RX_BUFFER_SIZE];
    int bytes_read = tud_cdc_n_read(itf, buffer, USB_RX_BUFFER_SIZE);
    if (bytes_read < 0)
    {
        ESP_LOGE("USB", "Failed to read from USB");
        return;
    }
    for (size_t i = 0; i < bytes_read; i++)
    {
        BaseType_t err = xQueueSend(usb_devs[itf]._rx_queue, buffer + i, 10);
        if (err != pdTRUE)
        {
            ESP_LOGE("USB", "Failed to write to USB RX queue %d", itf);
            break;
        }
    }
}

usb_device_t *usb_device_create(void)
{
    if (usb_dev_count >= 2)
    {
        ESP_LOGE("USB", "Maximum number of USB devices reached");
        return NULL;
    }

    usb_device_t *dev = &usb_devs[usb_dev_count++];

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };
    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE("USB", "Failed to install tinyusb driver. Error: %s", esp_err_to_name(err));
        return NULL;
    }

    dev->_tx_lock = xSemaphoreCreateBinary();
    if (dev->_tx_lock == NULL)
    {
        ESP_LOGE("USB", "Failed to create USB TX lock");
        return NULL;
    }
    xSemaphoreGive(dev->_tx_lock);

    dev->_rx_queue = xQueueCreate(2048, sizeof(uint8_t));
    if (dev->_rx_queue == NULL)
    {
        ESP_LOGE("USB", "Failed to create USB device RX queue");
        return NULL;
    }

    tinyusb_cdcacm_itf_t cdcacm_itf = (usb_dev_count == 1) ? TINYUSB_CDC_ACM_0 : TINYUSB_CDC_ACM_1;
    dev->_cdcacm_itf = cdcacm_itf;

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = cdcacm_itf,
        .rx_unread_buf_sz = 1024,
        .callback_rx = &_usb_device_rx_cb,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    err = tusb_cdc_acm_init(&acm_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE("USB", "Failed to initialize CDC-ACM. Error: %s", esp_err_to_name(err));
        return NULL;
    }

    return dev;
}

void usb_device_free(usb_device_t *dev)
{
    if (dev == NULL)
    {
        return;
    }
    if (dev->_tx_lock != NULL)
    {
        vSemaphoreDelete(dev->_tx_lock);
    }
    if (dev->_rx_queue != NULL)
    {
        vQueueDelete(dev->_rx_queue);
    }
    tusb_cdc_acm_deinit(dev->_cdcacm_itf);
}

uint32_t usb_device_send(usb_device_t *dev, uint8_t *buffer, uint32_t buffer_len)
{
    if (dev == NULL || buffer == NULL || buffer_len == 0)
    {
        return 0;
    }

    if (!tud_cdc_n_connected(dev->_cdcacm_itf))
    {
        ESP_LOGE("USB", "USB device not connected");
        return 0;
    }

    if (xPortInIsrContext())
    {
        BaseType_t taskWoken = false;
        if (xSemaphoreTakeFromISR(dev->_tx_lock, &taskWoken) != pdPASS)
        {
            return 0;
        }
    }
    else if (xSemaphoreTake(dev->_tx_lock, portMAX_DELAY) != pdPASS)
    {
        return 0;
    }

    size_t to_send = buffer_len, so_far = 0;
    while (to_send)
    {
        if (!tud_cdc_n_connected(dev->_cdcacm_itf))
        {
            buffer_len = so_far;
            break;
        }

        size_t space = tud_cdc_n_write_available(dev->_cdcacm_itf);
        if (!space)
        {
            tud_cdc_n_write_flush(dev->_cdcacm_itf);
            continue;
        }

        if (space > to_send)
        {
            space = to_send;
        }

        size_t sent = tud_cdc_n_write(dev->_cdcacm_itf, buffer + so_far, space);
        if (sent)
        {
            so_far += sent;
            to_send -= sent;
            tud_cdc_n_write_flush(dev->_cdcacm_itf);
        }
        else
        {
            buffer_len = so_far;
            break;
        }
    }

    if (xPortInIsrContext())
    {
        BaseType_t taskWoken = false;
        xSemaphoreGiveFromISR(dev->_tx_lock, &taskWoken);
    }
    else
    {
        xSemaphoreGive(dev->_tx_lock);
    }
    return buffer_len;
}

uint32_t usb_device_read(usb_device_t *dev, uint8_t *buffer, uint32_t buffer_len)
{
    if (dev->_rx_queue == NULL || buffer == NULL || buffer_len == 0)
    {
        return 0;
    }
    int so_far = 0;
    while (so_far < buffer_len)
    {
        uint8_t byte;
        if (xQueueReceive(dev->_rx_queue, &byte, 10) == pdTRUE)
        {
            buffer[so_far++] = byte;
        }
        else
        {
            break;
        }
    }
    return so_far;
}