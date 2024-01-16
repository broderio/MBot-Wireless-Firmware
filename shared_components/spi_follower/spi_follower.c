#include <stdio.h>
#include "spi_follower.h"

void spi_follower_init(int lofi, int lifo, int sclk, int cs) {
    // TODO: Implement
}

void spi_follower_deinit(void) {
    // TODO: Implement
}

int spi_follower_send(char *data, int len) {
    // TODO: Implement
    return 0;
}

int spi_follower_recv(char *data, int len) {
    // TODO: Implement
    return 0;
}

int spi_follower_send_and_recv(char *send_data, char *recv_data, int len) {
    // TODO: Implement
    return 0;
}

void spi_follower_set_post_setup_cb(void (*post_setup_cb)(spi_slave_transaction_t* trans)) {
    // TODO: Implement
}

void spi_follower_set_post_trans_cb(void (*post_trans_cb)(spi_slave_transaction_t* trans)) {
    // TODO: Implement
}
