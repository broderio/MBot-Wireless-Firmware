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

void usb_device_init(void);
void usb_device_deinit(void);
int usb_device_send(uint8_t* buffer, int size);
int usb_device_read(uint8_t* data, int len);

#endif
