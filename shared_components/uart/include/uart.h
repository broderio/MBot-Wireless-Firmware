#include <stdio.h>
#include "driver/uart.h"

#ifndef UART_H
#define UART_H

void uart_init(int port, int baud_rate, int data_bits, int parity, int stop_bits); // Defualt: 8N1
void uart_deinit(int port);
void uart_send(int port, char *data, int len);
void uart_send_char(int port, char data);
void uart_send_string(int port, char *data);
void uart_read(int port, char *data, int len);
void uart_read_char(int port, char *data);

#endif