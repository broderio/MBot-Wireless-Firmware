#include <stdio.h>
#include "camera.h"

esp_err_t camera_init(camera_pins_t *pins) {
    camera_config_t camera_config = {
        .pin_pwdn = -1,
        .pin_reset = -1,

        .pin_xclk = pins->xclk,
        .pin_sccb_sda = pins->sda,
        .pin_sccb_scl = pins->scl,
        .pin_d7 = pins->d7,
        .pin_d6 = pins->d6,
        .pin_d5 = pins->d5,
        .pin_d4 = pins->d4,
        .pin_d3 = pins->d3,
        .pin_d2 = pins->d2,
        .pin_d1 = pins->d1,
        .pin_d0 = pins->d0,
        .pin_vsync = pins->vsync,
        .pin_href = pins->href,
        .pin_pclk = pins->pclk,

        //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
        .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

        .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
        .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY
    };
    return esp_camera_init(&camera_config);
}

esp_err_t camera_deinit() {
     return esp_camera_deinit();
}

void camera_capture_frame(camera_fb_t** frame) {
    *frame = esp_camera_fb_get();
}

void camera_return_frame(camera_fb_t* frame) {
    esp_camera_fb_return(frame);
}