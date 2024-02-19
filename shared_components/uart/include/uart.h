#include <stdio.h>
#include "driver/uart.h"

#ifndef UART_H
#define UART_H

void uart_init(int port, int baud_rate, int data_bits, int parity, int stop_bits); // Defualt: 8N1
void uart_deinit(int port);
void uart_write(int port, const char *data, int len);
void uart_write_char(int port, const char data);
void uart_write_string(int port, const char *data);
int uart_read(int port, char *data, int len, int timeout_ms);
void uart_read_char(int port, char *data);
int uart_in_waiting(int port);

#endif