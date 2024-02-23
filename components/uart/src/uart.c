#include <stdio.h>
#include "string.h"
#include "driver/uart.h"

#include "uart.h"

void uart_init(int port, int rx_pin, int tx_pin, int baud_rate, size_t buffer_size) {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(port, buffer_size, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void uart_deinit(int port) {
    ESP_ERROR_CHECK(uart_driver_delete(port));
}

int uart_write(int port, const char *data, int len) {
    return uart_write_bytes(port, data, len);
}

int uart_write_char(int port, const char data) {
    return uart_write_bytes(port, &data, 1);
}

int uart_write_string(int port, const char *data) {
    return uart_write(port, data, strlen(data));
}

int uart_read(int port, char *data, int len, int timeout_ms) {
    return uart_read_bytes(port, data, len, timeout_ms / portTICK_PERIOD_MS);
}

void uart_read_char(int port, char *data) {
    ESP_ERROR_CHECK(uart_read_bytes(port, data, 1, portMAX_DELAY));
}

int uart_in_waiting(int port) {
    int len;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(port, (size_t*)&len));
    return len;
}

void uart_flush_buffer(int port) {
    ESP_ERROR_CHECK(uart_flush(port));
}