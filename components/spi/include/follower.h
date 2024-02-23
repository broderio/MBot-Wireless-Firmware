#include "driver/gpio.h"
#include "driver/spi_slave.h"

#ifndef SPI_FOLLOWER_H
#define SPI_FOLLOWER_H

#define SPI_HOST    SPI2_HOST

void spi_follower_init(int lofi, int lifo, int sclk, int cs);
void spi_follower_deinit(void);
int spi_follower_send(char *data, int len);
int spi_follower_recv(char *data, int len);
int spi_follower_send_and_recv(char *send_data, char *recv_data, int len);
void spi_follower_set_post_setup_cb(void (*post_setup_cb)(spi_slave_transaction_t* trans));
void spi_follower_set_post_trans_cb(void (*post_trans_cb)(spi_slave_transaction_t* trans));

#endif