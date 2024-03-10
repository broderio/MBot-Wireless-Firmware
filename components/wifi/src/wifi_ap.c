#include "string.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_ap.h"

const char* WIFI_AP_TAG = "WIFI_AP";

/**
 * @brief Event handler for the Wi-Fi access point.
 *
 * This function is called when a Wi-Fi access point event occurs.
 *
 * @param arg Pointer to the argument passed to the event handler.
 * @param event_base The event base associated with the event.
 * @param event_id The ID of the event.
 * @param event_data Pointer to the event data.
 */
static void _wifi_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(WIFI_AP_TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(WIFI_AP_TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

/**
 * @brief Initializes the soft AP mode for the WiFi module.
 *
 * This function sets up the WiFi module to work as a soft access point (AP).
 * It configures the SSID, password, channel, and maximum number of connections.
 *
 * @param ssid The SSID (network name) of the soft AP.
 * @param pass The password for the soft AP.
 * @param channel The channel to be used by the soft AP.
 * @param max_conn The maximum number of connections allowed for the soft AP.
 */
void wifi_init_softap(const char* ssid, const char* pass, int channel, int max_conn)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &_wifi_ap_event_handler,
                                                        NULL,
                                                        NULL));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ssid),
            .channel = channel,
            .max_connection = max_conn,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    strcpy((char*)wifi_config.ap.ssid, ssid);
    strcpy((char*)wifi_config.ap.password, pass);

    if (strlen(pass) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_AP_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", ssid, pass, channel);
}