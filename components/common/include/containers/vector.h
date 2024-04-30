/**
 * @file vector.h
 * @brief This file contains functions for initializing and managing a vector.
 */

#pragma once

#include "common.h"

typedef struct vector_t vector_t;
typedef struct vector_iterator_t vector_iterator_t;

vector_t *vector_create(type_t type);
void vector_destroy(vector_t *vector);

void *vector_at(vector_t *vector, int index);
void *vector_front(vector_t *vector);
void *vector_back(vector_t *vector);
void *vector_data(vector_t *vector);

vector_iterator_t *vector_begin(vector_t *vector);
vector_iterator_t *vector_end(vector_t *vector);
vector_iterator_t *vector_rbegin(vector_t *vector);
vector_iterator_t *vector_rend(vector_t *vector);

int vector_empty(vector_t *vector);
int vector_size(vector_t *vector);
void vector_reserve(vector_t *vector, int n);
int vector_capacity(vector_t *vector);
void vector_shrink_to_fit(vector_t *vector);

void vector_clear(vector_t *vector);
vector_iterator_t *vector_insert(vector_t *vector, void *data, int index);
vector_iterator_t *vector_insert_range(vector_t *vector, void *data, int index, int n);
void vector_erase(vector_t *vector, int index);
void vector_push_back(vector_t *vector, void *data);
void vector_append_range(vector_t *vector, void *data, int n);
void vector_pop_back(vector_t *vector);
void vector_resize(vector_t *vector, int n, void *value);
void vector_swap(vector_t *vector, vector_t *other);

vector_iterator_t *vector_iterator_next(vector_t *vector, vector_iterator_t *iterator);
vector_iterator_t *vector_iterator_prev(vector_t *vector, vector_iterator_t *iterator);
int vector_iterator_equal(vector_iterator_t *iterator1, vector_iterator_t *iterator2);
void *vector_iterator_data(vector_t *vector, vector_iterator_t *iterator);