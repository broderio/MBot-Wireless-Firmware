#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "usb_device.h"

#include "pairing.h"

pair_config_t get_pair_config() {
    pair_config_t default_cfg = DEFAULT_PAIR_CFG;
    pair_config_t pair_cfg = {0};
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("PAIRING", "Failed to open NVS handle. Error: %s", esp_err_to_name(err));
        return default_cfg;
    }

    size_t len = sizeof(pair_config_t);
    err = nvs_get_blob(nvs_handle, "pair_cfg", &pair_cfg, &len);
    if (err != ESP_OK) {
        ESP_LOGE("PAIRING", "Failed to read ssid from NVS. Error: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return default_cfg;
    }

    nvs_close(nvs_handle);
    return pair_cfg;
}

void set_pair_config(pair_config_t *pair_cfg) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE("PAIRING", "Failed to open NVS handle. Error: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(nvs, "pair_cfg", pair_cfg, sizeof(pair_config_t));
    if (err != ESP_OK) {
        ESP_LOGE("PAIRING", "Failed to write ssid to NVS. Error: %s", esp_err_to_name(err));
    }

    nvs_close(nvs);
}