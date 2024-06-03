/**
 * @file array.h
 * @brief This file contains functions for initializing and managing an array.
 * This implemntation is based on the C++ STL. The array is a dynamically allocated array.
 */

#pragma once

#include <string.h>
#include <stdlib.h>

#include "common.h"

#define ARRAY_T_SIZE    24

typedef struct array_t array_t;
typedef struct array_iterator_t {
    array_t *array;
    int index;
    int dir;
} array_iterator_t;

array_t *array_create(type_t type, int size);
void array_destroy(array_t *array);

type_t array_type(array_t *array);

void *array_at(array_t *array, int index);
void *array_front(array_t *array);
void *array_back(array_t *array);

array_iterator_t array_begin(array_t *array);
array_iterator_t array_rbegin(array_t *array);
array_iterator_t array_end(array_t *array);
array_iterator_t array_rend(array_t *array);

int array_empty(array_t *array);
int array_size(array_t *array);

void array_fill(array_t *array, void *value);
void array_swap(array_t *array, array_t *other);

void array_iterator_next(array_iterator_t *iterator);
void array_iterator_prev(array_iterator_t *iterator);
int array_iterator_equal(array_iterator_t *iterator, array_iterator_t *other);
void *array_iterator_value(array_iterator_t *iterator);