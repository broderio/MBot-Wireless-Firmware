#include "spi_follower.h"
#include "espnow.h"
#include "serializer.h"

#ifndef CLIENT_MESSAGING_H
#define CLIENT_MESSAGING_H

#define LOFI_HS_PIN             4 
#define LIFO_HS_PIN             5 
#define LIFO_PIN                6 
#define CS_PIN                  7 
#define SCLK_PIN                15
#define LOFI_PIN                16

void client_messaging_init();
void client_messaging_deinit();
void client_message_bot_to_host();
void client_message_host_to_bot();

#endif