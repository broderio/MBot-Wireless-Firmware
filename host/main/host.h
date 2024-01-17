#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "buttons.h"
#include "fram.h"
#include "host_messaging.h"
#include "joystick.h"

#ifndef HOST_H
#define HOST_H

#define PILOT_PIN     6
#define PAIR_PIN      17
#define LED1_PIN      15                        /**< LED 1 pin on board (GPIO)*/
#define LED2_PIN      16                        /**< LED 2 pin on board (GPIO)*/

#endif