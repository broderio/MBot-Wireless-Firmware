#include "tinyusb.h"
#include "tusb_cdc_acm.h"

#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#define USB_DEVICE              TINYUSB_USBDEV_0
#define USB_DEVICE_CDC_ACM      TINYUSB_CDC_ACM_0
#define USB_DEVICE_RX_BUFSIZE   256

void usb_device_init(void);
void usb_device_deinit(void);
void usb_device_send(uint8_t *buf, int len);
int usb_device_read(uint8_t *buf, int len);
void usb_set_rx_callback(void (*callback)(int itf, cdcacm_event_t *event));

#endif
