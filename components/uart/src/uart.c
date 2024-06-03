#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "driver/uart.h"

#include "uart.h"

struct uart_t {
    uart_port_t _port;
    uint8_t _rx_pin;
    uint8_t _tx_pin;
    uint32_t _baud_rate;
    uint32_t _buffer_size;
};

uart_t *uart_init(uint8_t port, uint8_t rx_pin, uint8_t tx_pin, uint32_t baud_rate, uint32_t buffer_size) {
    uart_t *uart = (uart_t*)malloc(sizeof(uart_t));
    if (uart == NULL) {
        ESP_LOGE("UART", "Failed to allocate memory for UART");
        return NULL;
    }
    uart->_port = port;
    uart->_rx_pin = rx_pin;
    uart->_tx_pin = tx_pin;
    uart->_baud_rate = baud_rate;
    uart->_buffer_size = buffer_size;

    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err;
    if (!uart_is_driver_installed(uart->_port)) {
        err = uart_driver_install(uart->_port, buffer_size, 0, 0, NULL, 0);
        if (err != ESP_OK) {
            ESP_LOGE("UART", "Failed to install UART driver. Error: %s", esp_err_to_name(err));
            free(uart);
            return NULL;
        }
    }

    err = uart_param_config(uart->_port, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE("UART", "Failed to configure UART parameters. Error: %s", esp_err_to_name(err));
        free(uart);
        return NULL;
    }

    err = uart_set_pin(uart->_port, uart->_tx_pin, uart->_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE("UART", "Failed to set UART pins. Error: %s", esp_err_to_name(err));
        free(uart);
        return NULL;
    }

    return uart;
}

void uart_deinit(uart_t *uart) {
    if (uart == NULL) {
        return;
    }
    uart_driver_delete(uart->_port);
    free(uart);
}

uint8_t uart_get_port(uart_t *uart) {
    if (uart == NULL) {
        return UART_NUM_MAX;
    }
    return uart->_port;
}

uint8_t uart_get_rx_pin(uart_t *uart) {
    if (uart == NULL) {
        return 0;
    }
    return uart->_rx_pin;
}

uint8_t uart_get_tx_pin(uart_t *uart) {
    if (uart == NULL) {
        return 0;
    }
    return uart->_tx_pin;
}

uint32_t uart_get_baud(uart_t *uart) {
    if (uart == NULL) {
        return 0;
    }
    return uart->_baud_rate;
}

uint32_t uart_write(uart_t *uart, uint8_t *data, uint32_t len) {
    if (uart == NULL) {
        return 0;
    }
    int bytes_sent = uart_write_bytes(uart->_port, data, len);
    if (bytes_sent < 0) {
        ESP_LOGE("UART", "Failed to write to UART");
        return 0;
    }
    return bytes_sent;
}

uint8_t uart_write_byte(uart_t *uart, uint8_t data) {
    if (uart == NULL) {
        return 0;
    }
    return (uint8_t)uart_write(uart, &data, 1);
}

uint32_t uart_write_string(uart_t *uart, char *data) {
    if (uart == NULL) {
        return 0;
    }
    return uart_write(uart, (uint8_t*)data, strlen(data));
}

uint32_t uart_read(uart_t *uart, uint8_t *data, uint32_t len, uint32_t timeout_ms) {
    if (uart == NULL) {
        return 0;
    }
    int bytes_read = uart_read_bytes(uart->_port, data, len, timeout_ms / portTICK_PERIOD_MS);
    if (bytes_read < 0) {
        ESP_LOGE("UART", "Failed to read from UART");
        return 0;
    }
    return bytes_read;
}

uint8_t uart_read_byte(uart_t *uart, uint8_t *data) {
    if (uart == NULL) {
        return 0;
    }
    return (uint8_t)uart_read(uart, data, 1, portMAX_DELAY);
}

uint32_t uart_in_waiting(uart_t *uart) {
    if (uart == NULL) {
        return 0;
    }
    uint32_t len;
    uart_get_buffered_data_len(uart->_port, (size_t*)&len);
    return len;
}

uint8_t uart_flush_buffer(uart_t *uart) {
    if (uart == NULL) {
        return 0;
    }
    esp_err_t err = uart_flush(uart->_port);
    if (err != ESP_OK) {
        ESP_LOGE("UART", "Failed to flush UART buffer. Error: %s", esp_err_to_name(err));
        return 1;
    }
    return 0;
}