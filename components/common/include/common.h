#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

typedef int (*compare_func_t)(const void*, const void*);
typedef int (*predicate_func_t)(const void*);

typedef enum {
    TYPE_CHAR,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_POINTER,
    TYPE_STRING,
    TYPE_ERROR
} type_t;

#define TYPE_SIZE(type) \
    (type == TYPE_CHAR ? sizeof(char) : \
    (type == TYPE_INT ? sizeof(int) : \
    (type == TYPE_FLOAT ? sizeof(float) : \
    (type == TYPE_DOUBLE ? sizeof(double) : \
    (type == TYPE_POINTER ? sizeof(void*) : \
    (type == TYPE_STRING ? sizeof(char*) : \
    0))))))

uint32_t get_time_ms();
void delay_ms(uint32_t ms);