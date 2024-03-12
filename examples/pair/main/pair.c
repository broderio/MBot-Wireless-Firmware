#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "pairing.h"
#include "buttons.h"
#include "usb_device.h"

#define BTN_PIN 17

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    usb_device_init();

    button_t *btn = button_create(BTN_PIN);
    if (btn == NULL) {
        ESP_LOGE("BUTTON", "Failed to create button");
        return;
    }
    button_interrupt_enable(btn);
    
    uint8_t ack_recv;
    int bytes_read = 0;
    while (!bytes_read) {
        bytes_read = usb_device_read(&ack_recv, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    pair_config_t pair_cfg = get_pair_config();
    if (strlen(pair_cfg.ssid) >= 3 || strlen(pair_cfg.password) != 0) {
        usb_device_send((uint8_t*)pair_cfg.ssid, sizeof(pair_cfg.ssid));
        button_wait_for_press(btn);
        button_wait_for_release(btn);
    }

    pair_config_t new_pair_cfg = {0};
    uint8_t ack_send = 0xFF;
    while (true) {
        usb_device_send(&ack_send, 1);

        vTaskDelay(500 / portTICK_PERIOD_MS);
        
        uint8_t buffer[sizeof(pair_config_t)];
        usb_device_read(buffer, sizeof(pair_config_t));
        memcpy(&new_pair_cfg, buffer, sizeof(pair_config_t));

        vTaskDelay(500 / portTICK_PERIOD_MS);

        usb_device_send(buffer, sizeof(pair_config_t));

        vTaskDelay(500 / portTICK_PERIOD_MS);
        
        usb_device_read(&ack_recv, 1);

        if (ack_recv == 0xFF) {
            break;
        }
    }
    set_pair_config(&new_pair_cfg);
}