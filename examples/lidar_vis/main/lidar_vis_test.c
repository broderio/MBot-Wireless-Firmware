#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

#include "lidar.h"

#define LIDAR_TX_PIN                43                  /**< GPIO Pin for UART transmit line */
#define LIDAR_RX_PIN                44                  /**< GPIO Pin for UART receive line */
#define LIDAR_PWM_PIN               1                   /**< GPIO Pin for LIDAR PWM signal */

void app_main(void)
{
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = "mbot_wireless_client",
        .external_phy = false,
        .configuration_descriptor = NULL,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = NULL,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    lidar_t lidar;
    int error = lidar_init(&lidar, LIDAR_TX_PIN, LIDAR_RX_PIN, LIDAR_PWM_PIN);
    if (error != 0)
    {
        return;
    }

    uint16_t scan[361] = {0};
    scan[0] = 0xFFFF;
    lidar_start_exp_scan(&lidar);
    while (true)
    {
        lidar_get_exp_scan_360(&lidar, scan + 1);
        tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, (uint8_t*)scan, sizeof(scan));
        tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    }
    lidar_stop_scan(&lidar);
}