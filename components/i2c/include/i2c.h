#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "driver/i2c.h"

#ifndef I2C_H
#define I2C_H

#define I2C_NUM              0
#define I2C_FREQ_HZ          400000
#define I2C_TIMEOUT_MS       1000

void i2c_init(int sda, int scl);
void i2c_deinit();
int i2c_read(uint8_t addr, uint8_t *data, size_t len);
int i2c_write(uint8_t addr, uint8_t *data, size_t len);

#endif