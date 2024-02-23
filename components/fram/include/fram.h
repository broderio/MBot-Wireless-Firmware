#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "i2c.h"

#ifndef FRAM_H
#define FRAM_H

#define FUJITSU_MANUF_ID        0x00A

#define FRAM_MAX_ADDRESS        512
#define FRAM_PROD_ID            0x010	// 4k version
#define FRAM_DENSITY            0x0	// 4k version
#define FRAM_ADDRESS_A00        0x50
#define FRAM_ADDRESS_A01        0x52
#define FRAM_ADDRESS_A10        0x54
#define FRAM_ADDRESS_A11        0x56
#define FRAM_DEFAULT_ADDRESS    FRAM_ADDRESS_A00

#define FRAM_MASTER_CODE	    0xF8

int fram_init();
int fram_read(uint16_t addr, size_t length, uint8_t* data);
int fram_write(uint16_t addr, size_t length, uint8_t* data);
int fram_read_word(uint16_t addr, uint16_t* data);
int fram_write_word(uint16_t addr, uint16_t data);
int fram_erase();

#endif