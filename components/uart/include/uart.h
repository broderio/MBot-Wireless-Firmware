#pragma once

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "driver/uart.h"

/**
 * @brief Structure representing a UART instance.
 */
typedef struct uart_t uart_t;

/**
 * @brief Initializes a UART port with the specified configuration.
 *
 * @param port The UART port number.
 * @param rx_pin The RX pin number.
 * @param tx_pin The TX pin number.
 * @param baud_rate The baud rate for the UART communication.
 * @param buffer_size The size of the UART receive buffer.
 * @return A pointer to the initialized UART structure.
 */
uart_t *uart_init(uint8_t port, uint8_t rx_pin, uint8_t tx_pin, uint32_t baud_rate, uint32_t buffer_size); // Default: 8N1

/**
 * @brief Deinitializes a UART port.
 *
 * @param uart A pointer to the UART structure to deinitialize.
 */
void uart_deinit(uart_t *uart);

/**
 * @brief Get the port number of the UART.
 *
 * @param uart Pointer to the UART structure.
 * @return The port number of the UART.
 */
uint8_t uart_get_port(uart_t *uart);

/**
 * @brief Get the RX pin number of the UART.
 *
 * @param uart Pointer to the UART structure.
 * @return The RX pin number of the UART.
 */
uint8_t uart_get_rx_pin(uart_t *uart);

/**
 * @brief Get the TX pin number of the UART.
 *
 * @param uart Pointer to the UART structure.
 * @return The TX pin number of the UART.
 */
uint8_t uart_get_tx_pin(uart_t *uart);

/**
 * @brief Get the baud rate of the UART.
 *
 * @param uart Pointer to the UART structure.
 * @return The baud rate of the UART.
 */
uint32_t uart_get_baud(uart_t *uart);

/**
 * @brief Writes data to the UART port.
 *
 * @param uart A pointer to the UART structure.
 * @param data A pointer to the data to be written.
 * @param len The length of the data to be written.
 * @return The number of bytes written.
 */
uint32_t uart_write(uart_t *uart, uint8_t *data, uint32_t len);

/**
 * @brief Writes a single byte to the UART port.
 *
 * @param uart A pointer to the UART structure.
 * @param data The byte to be written.
 * @return The number of bytes written (always 1).
 */
uint8_t uart_write_byte(uart_t *uart, uint8_t data);

/**
 * @brief Writes a null-terminated string to the UART port.
 *
 * @param uart A pointer to the UART structure.
 * @param data A pointer to the string to be written.
 * @return The number of bytes written (excluding the null terminator).
 */
uint32_t uart_write_string(uart_t *uart, char *data);

/**
 * @brief Reads data from the UART port.
 *
 * @param uart A pointer to the UART structure.
 * @param data A pointer to the buffer to store the read data.
 * @param len The maximum number of bytes to read.
 * @param timeout_ms The timeout value in milliseconds.
 * @return The number of bytes read.
 */
uint32_t uart_read(uart_t *uart, uint8_t *data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief Reads a single byte from the UART port.
 *
 * @param uart A pointer to the UART structure.
 * @param data A pointer to store the read byte.
 * @return 1 if a byte was read successfully, 0 otherwise.
 */
uint8_t uart_read_byte(uart_t *uart, uint8_t *data);

/**
 * @brief Returns the number of bytes waiting in the UART receive buffer.
 *
 * @param uart A pointer to the UART structure.
 * @return The number of bytes waiting in the receive buffer.
 */
uint32_t uart_in_waiting(uart_t *uart);

/**
 * @brief Flushes the UART receive buffer.
 *
 * @param uart A pointer to the UART structure.
 * @return 1 if the buffer was flushed successfully, 0 otherwise.
 */
uint8_t uart_flush_buffer(uart_t *uart);