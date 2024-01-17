#include "usb_device.h"
#include "wifi.h"
#include "serializer.h"

#ifndef HOST_MESSAGING_H
#define HOST_MESSAGING_H

void host_messaging_init();
void host_messaging_deinit();
void host_message_client_to_ui();
void host_message_ui_to_client();

#endif