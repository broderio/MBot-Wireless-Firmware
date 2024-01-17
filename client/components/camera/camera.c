#include <stdio.h>
#include "camera.h"

static camera_config_t camera_config = {
    .pin_pwdn = -1,
    .pin_reset = -1,
    .pin_xclk = CAM_MCLK_PIN,
    .pin_sccb_sda = CAM_SDA_PIN,
    .pin_sccb_scl = CAM_SCL_PIN,

    .pin_d7 = CAM_D7_PIN,
    .pin_d6 = CAM_D6_PIN,
    .pin_d5 = CAM_D5_PIN,
    .pin_d4 = CAM_D4_PIN,
    .pin_d3 = CAM_D3_PIN,
    .pin_d2 = CAM_D2_PIN,
    .pin_d1 = CAM_D1_PIN,
    .pin_d0 = CAM_D0_PIN,
    .pin_vsync = CAM_VSYNC_PIN,
    .pin_href = CAM_HSYNC_PIN,
    .pin_pclk = CAM_PCLK_PIN,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

void camera_init() {
    // TODO: Implement
}

void camera_deinit() {
    // TODO: Implement
}

void camera_capture_frame(camera_fb_t* frame) {
    // TODO: Implement
}

void camera_return_frame(camera_fb_t* frame) {
    // TODO: Implement
}