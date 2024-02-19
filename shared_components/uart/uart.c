#include <stdio.h>
#include "driver/uart.h"

#include "uart.h"

void uart_init(int port, int baud_rate, int data_bits, int parity, int stop_bits) {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = data_bits,
        .parity = parity,
        .stop_bits = stop_bits,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(port, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
}

void uart_deinit(int port) {
    uart_driver_delete(port);
}

void uart_write(int port, const char *data, int len) {
    uart_write_bytes(port, data, len);
}

void uart_write_char(int port, const char data) {
    // TODO: Implement
}

void uart_write_string(int port, const char *data) {
    // TODO: Implement
}

int uart_read(int port, char *data, int len, int timeout_ms) {
    return uart_read_bytes(port, data, len, timeout_ms / portTICK_PERIOD_MS);
}

void uart_read_char(int port, char *data) {
    // TODO: Implement
}

int uart_in_waiting(int port) {
    int len;
    uart_get_buffered_data_len(port, (size_t*)&len);
    return len;
}
