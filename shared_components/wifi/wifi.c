#include <stdio.h>
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "wifi.h"

typedef struct _wifi_data_t {
    uint8_t len;       
    uint16_t crc;      
    uint8_t packet[0];
} __attribute__((packed)) _wifi_data_t;

QueueHandle_t wifi_send_queue;
QueueHandle_t wifi_recv_queue;

void _wifi_data_prepare(wifi_packet_t *packet, _wifi_data_t *data);
void _wifi_data_parse(_wifi_data_t *data, wifi_packet_t *packet);
void _wifi_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status);
void _wifi_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len);

void _wifi_data_prepare(wifi_packet_t *packet, _wifi_data_t *data) {
    // TODO: Implement
}

void _wifi_data_parse(_wifi_data_t *data, wifi_packet_t *packet) {
    // TODO: Implement
}

void _wifi_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status) {
    // TODO: Implement
}

void _wifi_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    // TODO: Implement
}

void wifi_init(void) {
    // TODO: Implement
}

void wifi_deinit(void) {
    // TODO: Implement
}

int wifi_read(wifi_packet_t* data, int timeout_ms) {
    // TODO: Implement
    return 0;
}

int wifi_read_blocking(wifi_packet_t* data) {
    // TODO: Implement
    return 0;
}

int wifi_send(wifi_packet_t* data) {
    // TODO: Implement
    return 0;
}