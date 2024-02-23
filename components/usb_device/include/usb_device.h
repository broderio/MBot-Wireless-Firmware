#include <stdio.h>

#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/message_buffer.h"

#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#define USB_DEVICE              TINYUSB_USBDEV_0
#define USB_DEVICE_CDC_ACM      TINYUSB_CDC_ACM_0
#define USB_DEVICE_RX_BUFSIZE   256

typedef struct usb_packet_t {
    uint16_t len;
    uint8_t* data;
} usb_packet_t;

void usb_device_init(void);
void usb_device_deinit(void);
void usb_device_send(usb_packet_t* packet);
int usb_device_read_bytes(uint8_t* data, int len, int timeout_ms);

#endif
