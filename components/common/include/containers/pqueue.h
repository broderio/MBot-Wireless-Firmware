/**
 * @file pqueue.h
 * @brief This file contains functions for initializing and managing a priority queue.
 * This implemntation is based on the C++ STL. The priority queue is a binary heap.
 */

#pragma once

#include "common.h"

typedef struct pqueue_t pqueue_t;

pqueue_t *pqueue_create(type_t type, compare_func_t compare);
void pqueue_destroy(pqueue_t *pqueue);

void *pqueue_top(pqueue_t *pqueue);

int pqueue_empty(pqueue_t *pqueue);
int pqueue_size(pqueue_t *pqueue);

void pqueue_push(pqueue_t *pqueue, void *data);
void pqueue_push_range(pqueue_t *pqueue, void *data, int n);
void pqueue_pop(pqueue_t *pqueue);
void pqueue_swap(pqueue_t *pqueue, pqueue_t *other);
