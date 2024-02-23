#include <stdio.h>
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifndef ESPNOW_H
#define ESPNOW_H

#define ESPNOW_WIFI_MODE            WIFI_MODE_AP
#define ESPNOW_WIFI_IF              ESP_IF_WIFI_AP
#define ESPNOW_WIFI_CHANNEL         11                  
#define ESPNOW_WIFI_PMK             "pmk1234567890123"  

#define ESPNOW_DATA_MAX_LEN         250                 
#define ESPNOW_WIFI_MAXDELAY        (size_t)0xffffffff  
#define ESPNOW_WIFI_QUEUE_SIZE      6                   
#define MAC_ADDR_LEN                6                   

typedef struct espnow_packet_t {
    uint8_t mac[MAC_ADDR_LEN];     
    uint16_t len;                            
    uint8_t* data;
} espnow_packet_t;

void espnow_init(void);
void espnow_deinit(void);
int espnow_read(espnow_packet_t* data, int timeout_ms);
int espnow_read_blocking(espnow_packet_t* data);
int espnow_send(espnow_packet_t* data);

#endif