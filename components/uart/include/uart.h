#include <stdio.h>
#include "driver/uart.h"

#ifndef UART_H
#define UART_H

void uart_init(int port, int rx_pin, int tx_pin, int baud_rate, size_t buffer_size); // Defualt: 8N1
void uart_deinit(int port);
int uart_write(int port, const char *data, int len);
int uart_write_char(int port, const char data);
int uart_write_string(int port, const char *data);
int uart_read(int port, char *data, int len, int timeout_ms);
void uart_read_char(int port, char *data);
int uart_in_waiting(int port);

#endif