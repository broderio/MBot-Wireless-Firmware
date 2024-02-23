#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "driver/gpio.h"

void buttons_init();
void buttons_deinit();
void buttons_set_callback(int pin, void (*callback)(void *), void *args);

#endif