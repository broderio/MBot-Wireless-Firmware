#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "wifi_sta.h"
#include "sockets.h"
#include "camera.h"

#define ESP_WIFI_SSID           "930Catherine"
#define ESP_WIFI_PASS           "Catherine2024!"
#define PORT                    1234
#define STATIC_IP_ADDR          "10.0.0.2"
#define STATIC_NETMASK_ADDR     "255.255.255.0"
#define STATIC_GW_ADDR          "10.0.0.1"

#define CAM_MCLK_PIN                18                  /**< GPIO Pin for I2S master clock */
#define CAM_PCLK_PIN                8                   /**< GPIO Pin for I2S peripheral clock */
#define CAM_VSYNC_PIN               46                  /**< GPIO Pin for I2S vertical sync */
#define CAM_HSYNC_PIN               9                   /**< GPIO Pin for I2S horizontal sync */
#define CAM_D0_PIN                  14                  /**< GPIO Pin for I2S 0th bit data line */
#define CAM_D1_PIN                  47                  /**< GPIO Pin for I2S 1st bit data line */
#define CAM_D2_PIN                  48                  /**< GPIO Pin for I2S 2nd bit data line */
#define CAM_D3_PIN                  21                  /**< GPIO Pin for I2S 3rd bit data line */
#define CAM_D4_PIN                  13                  /**< GPIO Pin for I2S 4th bit data line */
#define CAM_D5_PIN                  12                  /**< GPIO Pin for I2S 5th bit data line */
#define CAM_D6_PIN                  11                  /**< GPIO Pin for I2S 6th bit data line */
#define CAM_D7_PIN                  10                  /**< GPIO Pin for I2S 7th bit data line */
#define CAM_SDA_PIN                 3                   /**< GPIO Pin for I2C data line */
#define CAM_SCL_PIN                 2                   /**< GPIO Pin for I2C clock line */

static const char *CAMERA_SOCKET_TAG = "CAMERA_SOCKET";

SemaphoreHandle_t frame_ready;

camera_fb_t* frame[2];
TaskHandle_t camera_task_handle;
int connected = 0;

void camera_task(void*)
{
    camera_pins_t camera_pins = {
        .xclk = CAM_MCLK_PIN,
        .sda = CAM_SDA_PIN,
        .scl = CAM_SCL_PIN,
        .d7 = CAM_D7_PIN,
        .d6 = CAM_D6_PIN,
        .d5 = CAM_D5_PIN,
        .d4 = CAM_D4_PIN,
        .d3 = CAM_D3_PIN,
        .d2 = CAM_D2_PIN,
        .d1 = CAM_D1_PIN,
        .d0 = CAM_D0_PIN,
        .vsync = CAM_VSYNC_PIN,
        .href = CAM_HSYNC_PIN,
        .pclk = CAM_PCLK_PIN
    };
    while (1)
    {
        camera_init(&camera_pins);
        while (1) 
        {
            if (!connected) 
            {
                camera_deinit();
                vTaskSuspend(NULL);
                break;
            }
            camera_capture_frame(&frame[1]);
            
            xSemaphoreTake(frame_ready, portMAX_DELAY);
            camera_return_frame(frame[0]);
            frame[0] = frame[1];
            xSemaphoreGive(frame_ready);
        }
    }
}

void socket_task(void*)
{
    server_socket_t server = socket_server_init(PORT);
    while (true) {
        connection_socket_t connection = socket_server_accept(server);
        if (connection > 0) {
            connected = 1;

            vTaskResume(camera_task_handle);
            xSemaphoreGive(frame_ready);
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            TickType_t last_wake_time;
            while (true)
            {
                last_wake_time = xTaskGetTickCount();

                xSemaphoreTake(frame_ready, portMAX_DELAY);
                // int ret = socket_connection_send(connection, (char*)(frame[0]->buf), frame[0]->len);
                printf("Length: %d\n", frame[0]->len);
                printf("Width: %d\n", frame[0]->width);
                printf("Height: %d\n", frame[0]->height);
                xSemaphoreGive(frame_ready);
                // if (ret < 0) {
                //     ESP_LOGI(CAMERA_SOCKET_TAG, "Client disconnected.");
                //     connected = 0;
                //     break;
                // }
                xTaskDelayUntil(&last_wake_time, 100 / portTICK_PERIOD_MS);
            }
        }
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(CAMERA_SOCKET_TAG, "ESP_WIFI_MODE_STA");

    wifi_init_sta_static_ip(ESP_WIFI_SSID, 
                            ESP_WIFI_PASS, 
                            STATIC_IP_ADDR, 
                            STATIC_NETMASK_ADDR, 
                            STATIC_GW_ADDR);

    frame_ready = xSemaphoreCreateBinary();

    xTaskCreate(socket_task, "socket_task", 4096, NULL, 5, NULL);
    xTaskCreate(camera_task, "camera_task", 4096, NULL, 5, &camera_task_handle);
}