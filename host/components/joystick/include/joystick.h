#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#ifndef JOYSTICK_H
#define JOYSTICK_H

#define JS_Y_PIN            4                     /**< Joystick Y pin on board (ADC1_CHANNEL3)*/
#define JS_X_PIN            5                     /**< Joystick X pin on board (ADC1_CHANNEL4)*/

#define JS_Y_ADC_CHANNEL    ADC1_CHANNEL_3        /**< Joystick Y pin on ADC1 */
#define JS_X_ADC_CHANNEL    ADC1_CHANNEL_4        /**< Joystick X pin on ADC1 */

typedef struct joystick_config_t {
    int x_in_max; // Voltage
    int x_in_min;
    int x_out_max; // Output Unit
    int x_out_min;
    int y_in_max;
    int y_in_min;
    int y_out_max;
    int y_out_min;
} joystick_config_t;

void joystick_init(joystick_config_t* config);
void joystick_deinit();
void joystick_get_output(int *x, int *y);
void joystick_get_raw_position(int *x, int *y);

#endif