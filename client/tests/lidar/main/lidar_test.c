#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "lidar.h"

void app_main(void)
{                   
    int error;
    lidar_t lidar;
    printf("Initializing lidar ... \n");
    lidar_init(&lidar);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    lidar_info_t info;
    printf("Reading lidar info ... \n");
    error = lidar_get_info(&info);
    if (error != 0)
    {
        printf("Error reading lidar info! \n");
    }
    else
    {
        printf("Model: %d \n", info.model);
        printf("Firmware Major: %d \n", info.firmware_major);
        printf("Firmware Minor: %d \n", info.firmware_minor);
        printf("Hardware: %d \n", info.hardware);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    lidar_health_t health;
    printf("Reading lidar health ... \n");
    error = lidar_get_health(&health);
    if (error != 0)
    {
        printf("Error reading lidar health! \n");
    }
    else
    {
        printf("Status: %d \n", health.status);
        printf("Error code: %d \n", health.error_code);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("Starting lidar scan ... \n");
    error = lidar_start_scan(&lidar);
    if (error != 0)
    {
        printf("Error starting lidar scan! \n");
        return;
    }
    printf("Lidar scanning: %d \n", lidar.scanning);
    printf("Lidar motor running: %d \n", lidar.motor_running);
    printf("Lidar descriptor size: %d \n", lidar.descriptor_size);
    lidar_stop_scan(&lidar);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("Reading lidar 360 scan ... \n");
    uint32_t scan[360] = {0};
    lidar_start_scan(&lidar);
    lidar_get_scan_360(&lidar, scan);
    lidar_stop_scan(&lidar);
    for (int i = 0; i < 360; i++)
    {
        printf("%d: %lu \n", i, scan[i]);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    printf("Starting lidar express scan ... \n");
    error = lidar_start_exp_scan(&lidar);
    if (error != 0)
    {
        printf("Error starting lidar scan! \n");
        return;
    }
    printf("Lidar scanning: %d \n", lidar.scanning);
    printf("Lidar motor running: %d \n", lidar.motor_running);
    printf("Lidar descriptor size: %d \n", lidar.descriptor_size);
    lidar_stop_scan(&lidar);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("Reading lidar 360 express scan ... \n");
    uint16_t scan_exp[360] = {0};
    lidar_start_exp_scan(&lidar);
    lidar_get_exp_scan_360(&lidar, scan_exp);
    lidar_stop_scan(&lidar);
    for (int i = 0; i < 360; i++)
    {
        printf("%d: %lu \n", i, scan_exp[i]);
    }
}