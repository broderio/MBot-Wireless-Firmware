#include <stdio.h>
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "espnow.h"

typedef struct _espnow_data_t {
    uint8_t len;       
    uint16_t crc;      
    uint8_t packet[0];
} __attribute__((packed)) _espnow_data_t;

QueueHandle_t espnow_send_queue;
QueueHandle_t espnow_recv_queue;

void _espnow_data_prepare(espnow_packet_t *packet, _espnow_data_t *data);
void _espnow_data_parse(_espnow_data_t *data, espnow_packet_t *packet);
void _espnow_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status);
void _espnow_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len);

void _espnow_data_prepare(espnow_packet_t *packet, _espnow_data_t *data) {
    // TODO: Implement
}

void _espnow_data_parse(_espnow_data_t *data, espnow_packet_t *packet) {
    // TODO: Implement
}

void _espnow_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status) {
    // TODO: Implement
}

void _espnow_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    // TODO: Implement
}

void espnow_init(void) {
    // TODO: Implement
}

void espnow_deinit(void) {
    // TODO: Implement
}

int espnow_read(espnow_packet_t* data, int timeout_ms) {
    // TODO: Implement
    return 0;
}

int espnow_read_blocking(espnow_packet_t* data) {
    // TODO: Implement
    return 0;
}

int espnow_send(espnow_packet_t* data) {
    // TODO: Implement
    return 0;
}