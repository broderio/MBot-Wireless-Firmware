#include "spi_follower.h"
#include "wifi.h"
#include "serializer.h"

#ifndef CLIENT_MESSAGING_H
#define CLIENT_MESSAGING_H

void client_messaging_init();
void client_messaging_deinit();
void client_message_bot_to_host();
void client_message_host_to_bot();

#endif