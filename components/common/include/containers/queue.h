/**
 * @file queue.h
 * @brief This file contains functions for initializing and managing a queue.
 * This implemntation is based on the C++ STL. The queue is a wrapper around a list.
 */

#pragma once

#include "common.h"
#include "list.h"

typedef struct queue_t queue_t;

queue_t *queue_create(type_t type);
void queue_destroy(queue_t *queue);

void *queue_front(queue_t *queue);
void *queue_back(queue_t *queue);

int queue_empty(queue_t *queue);
int queue_size(queue_t *queue);

void queue_push(queue_t *queue, void *data);
void queue_push_range(queue_t *queue, void *data, int n);
void queue_pop(queue_t *queue);
void queue_swap(queue_t *queue, queue_t *other);