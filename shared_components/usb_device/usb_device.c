#include <stdio.h>
#include "usb_device.h"

void usb_device_init(void) {
    // TODO: Implement
}

void usb_device_deinit(void) {
    // TODO: Implement
}

void usb_device_send(uint8_t *buf, int len) {
    // TODO: Implement
}

int usb_device_read(uint8_t *buf, int len) {
    // TODO: Implement
    return 0;
}

void usb_set_rx_callback(void (*callback)(int itf, cdcacm_event_t *event)) {
    // TODO: Implement
}