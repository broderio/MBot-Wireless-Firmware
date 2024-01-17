#include "esp_camera.h"

#ifndef CAMERA_H
#define CAMERA_H

#define CAM_MCLK_PIN                18                  /**< GPIO Pin for I2S master clock */
#define CAM_PCLK_PIN                8                   /**< GPIO Pin for I2S peripheral clock */
#define CAM_VSYNC_PIN               46                  /**< GPIO Pin for I2S vertical sync */
#define CAM_HSYNC_PIN               9                   /**< GPIO Pin for I2S horizontal sync */
#define CAM_D0_PIN                  14                  /**< GPIO Pin for I2S 0th bit data line */
#define CAM_D1_PIN                  47                  /**< GPIO Pin for I2S 1st bit data line */
#define CAM_D2_PIN                  48                  /**< GPIO Pin for I2S 2nd bit data line */
#define CAM_D3_PIN                  21                  /**< GPIO Pin for I2S 3rd bit data line */
#define CAM_D4_PIN                  13                  /**< GPIO Pin for I2S 4th bit data line */
#define CAM_D5_PIN                  12                  /**< GPIO Pin for I2S 5th bit data line */
#define CAM_D6_PIN                  11                  /**< GPIO Pin for I2S 6th bit data line */
#define CAM_D7_PIN                  10                  /**< GPIO Pin for I2S 7th bit data line */
#define CAM_SDA_PIN                 3                   /**< GPIO Pin for I2C data line */
#define CAM_SCL_PIN                 2                   /**< GPIO Pin for I2C clock line */

void camera_init();
void camera_deinit();
void camera_capture_frame(camera_fb_t* frame);
void camera_return_frame(camera_fb_t* frame);

#endif
