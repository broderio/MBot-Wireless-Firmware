#pragma once

#include <stdio.h>

#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/message_buffer.h"

#define USB_DEVICE              TINYUSB_USBDEV_0
#define USB_DEVICE_CDC_ACM      TINYUSB_CDC_ACM_0
#define USB_DEVICE_RX_BUFSIZE   256

/**
 * @brief Structure representing a USB device.
 */
typedef struct usb_device_t usb_device_t;

/**
 * @brief Creates a new USB device.
 *
 * @return A pointer to the newly created USB device.
 */
usb_device_t *usb_device_create(void);

/**
 * @brief Frees the memory allocated for a USB device.
 *
 * @param dev The USB device to free.
 */
void usb_device_free(usb_device_t *dev);

/**
 * @brief Sends data over the USB device.
 *
 * @param dev The USB device to send data through.
 * @param buffer The buffer containing the data to send.
 * @param buffer_len The length of the data buffer.
 * @return The number of bytes sent.
 */
uint32_t usb_device_send(usb_device_t *dev, uint8_t* buffer, uint32_t buffer_len);

/**
 * @brief Reads data from the USB device.
 *
 * @param dev The USB device to read data from.
 * @param buffer The buffer to store the read data.
 * @param buffer_len The length of the data buffer.
 * @return The number of bytes read.
 */
uint32_t usb_device_read(usb_device_t *dev, uint8_t* buffer, uint32_t buffer_len);