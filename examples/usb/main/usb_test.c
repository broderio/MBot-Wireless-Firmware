#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "usb_device.h"

void app_main(void)
{
    usb_device_init();

    while (true)
    {
        uint8_t data[32];
        int len = usb_device_read(data, 32);
        if (len > 0)
        {
            ESP_LOGI("USB", "Received %d bytes", len);
            usb_device_send(data, len);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}