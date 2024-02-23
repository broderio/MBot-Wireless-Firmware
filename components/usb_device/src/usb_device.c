#include <stdio.h>

#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/message_buffer.h"

#include "usb_device.h"

static MessageBufferHandle_t usb_device_message_buffer;

void _usb_device_send_cb(MessageBufferHandle_t xMessageBuffer, BaseType_t xIsInsideISR, BaseType_t * const pxHigherPriorityTaskWoken) {

}

void _usb_device_read_cb(MessageBufferHandle_t xMessageBuffer, BaseType_t xIsInsideISR, BaseType_t * const pxHigherPriorityTaskWoken) {

}         

void usb_device_init(void) {
    // TODO: Implement
}

void usb_device_deinit(void) {
    // TODO: Implement
}

void usb_device_send(usb_packet_t* packet) {
    // TODO: Implement
}

int usb_device_read(usb_packet_t* packet, int len, int timeout_ms) {
    // TODO: Implement
    return 0;
}