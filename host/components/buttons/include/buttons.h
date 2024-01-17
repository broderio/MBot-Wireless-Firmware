#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "driver/gpio.h"

#ifndef BUTTONS_H
#define BUTTONS_H

#define BUTTONS_UP_PIN          10                        /**< Controller button 1 (Up) pin on board (GPIO)*/
#define BUTTONS_RIGHT_PIN       9                         /**< Controller Button 2 (Right) pin on board (GPIO)*/
#define BUTTONS_DOWN_PIN        46                        /**< Controller button 3 (Down) pin on board (GPIO)*/
#define BUTTONS_LEFT_PIN        11                        /**< Controller button 4 (Left) pin on board (GPIO)*/
#define BUTTONS_JS_PIN          12                        /**< Controller button 5 (Joystick) pin on board (GPIO)*/

void buttons_init();
void buttons_deinit();
void buttons_set_callback(int pin, void (*callback)(void *), void *args);

#endif