#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"

typedef struct camera_pins_t {
    int xclk;
    int sda;
    int scl;
    int d7;
    int d6;
    int d5;
    int d4;
    int d3;
    int d2;
    int d1;
    int d0;
    int vsync;
    int href;
    int pclk;
} camera_pins_t;

esp_err_t camera_init(camera_pins_t *pins);
esp_err_t camera_deinit();
void camera_capture_frame(camera_fb_t** frame);
void camera_return_frame(camera_fb_t* frame);

#endif
