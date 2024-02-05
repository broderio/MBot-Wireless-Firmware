#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "serializer.h"
#include "lcm_types.h"
#include "wifi.h"
#include "usb_device.h"

#include "buttons.h"
#include "fram.h"
#include "joystick.h"

#ifndef HOST_H
#define HOST_H

#define PILOT_PIN 6
#define PAIR_PIN 17
#define LED1_PIN 15 /**< LED 1 pin on board (GPIO)*/
#define LED2_PIN 16 /**< LED 2 pin on board (GPIO)*/

typedef enum STATE
{
    INIT,
    PAIR,
    PILOT,
    SERIAL,
    ERROR
} HOST_STATE;

extern HOST_STATE state;
extern HOST_STATE prev_state;
extern uint8_t client_mac[MAC_ADDR_LEN];

static SemaphoreHandle_t state_change_sem;
static SemaphoreHandle_t suspend_task_sem;
static SemaphoreHandle_t suspend_task_confirm_sem;

static QueueHandle_t client_send_queue;
static QueueHandle_t ui_send_queue;

static TaskHandle_t pilot_task_handle;
static TaskHandle_t serial_task_handle;
static TaskHandle_t pair_task_handle;

void host_init();
void host_deinit();

int host_get_paired_client(uint8_t *mac);
void host_update_state(HOST_STATE new_state);
void host_send_timesync();
void host_read_packet(wifi_packet_t *packet);

void host_pilot_button_isr();
void host_pair_button_isr();

// These tasks will run depending on the state of the system
// They will queue data into the client_send_queue
void host_pilot_task();
void host_serial_task();
void host_pair_task();

// This task is always running and will queue data into the ui_send_queue
void host_read_wifi_task();

// These tasks are always running and dequeue data from the client_send_queue and ui_send_queue
void host_send_client_task();
void host_send_ui_task();

// This task is always running and is responsible for controlling the tasks based on the state of the system
void host_main_task();

#endif