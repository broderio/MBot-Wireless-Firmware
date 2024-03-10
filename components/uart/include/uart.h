#include <stdio.h>
#include "driver/uart.h"

#ifndef UART_H
#define UART_H

typedef int uart_t;

void uart_init(uart_t port, int rx_pin, int tx_pin, int baud_rate, size_t buffer_size); // Defualt: 8N1
void uart_deinit(uart_t port);
int uart_write(uart_t port, const char *data, int len);
int uart_write_char(uart_t port, const char data);
int uart_write_string(uart_t port, const char *data);
int uart_read(uart_t port, char *data, int len, int timeout_ms);
void uart_read_char(uart_t port, char *data);
int uart_in_waiting(uart_t port);
void uart_flush_buffer(uart_t port);

#endif