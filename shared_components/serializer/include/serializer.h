#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifndef SERIALIZER_H
#define SERIALIZER_H

void encode_rospkt(uint8_t* data, uint16_t len, uint16_t topic, uint8_t* pkt);
int decode_rospkt(uint8_t* pkt, uint8_t* data, uint16_t* len, uint16_t* topic);

#endif