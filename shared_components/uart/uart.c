#include <stdio.h>
#include "driver/uart.h"

#include "uart.h"

void uart_init(int port, int baud_rate, int data_bits, int parity, int stop_bits) {
    // TODO: Implement
}

void uart_deinit(int port) {
    // TODO: Implement
}

void uart_send(int port, char *data, int len) {
    // TODO: Implement
}

void uart_send_char(int port, char data) {
    // TODO: Implement
}

void uart_send_string(int port, char *data) {
    // TODO: Implement
}

void uart_read(int port, char *data, int len) {
    // TODO: Implement
}

void uart_read_char(int port, char *data) {
    // TODO: Implement
}
