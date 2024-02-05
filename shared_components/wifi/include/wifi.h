#include <stdio.h>
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifndef WIFI_H
#define WIFI_H

#define WIFI_MODE           WIFI_MODE_AP
#define WIFI_IF             ESP_IF_WIFI_AP
#define WIFI_CHANNEL        11                  
#define WIFI_PMK            "pmk1234567890123"  
#define WIFI_DATA_MAX_LEN   250                 
#define WIFI_MAXDELAY       (size_t)0xffffffff  
#define WIFI_QUEUE_SIZE     6                   
#define MAC_ADDR_LEN        6                   

typedef struct wifi_packet_t {
    uint8_t mac[MAC_ADDR_LEN];     
    uint16_t len;                            
    uint8_t* data;
} wifi_packet_t;

void wifi_init(void);
void wifi_deinit(void);
int wifi_read(wifi_packet_t* data, int timeout_ms);
int wifi_read_blocking(wifi_packet_t* data);
int wifi_send(wifi_packet_t* data);

#endif