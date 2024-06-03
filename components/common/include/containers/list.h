/**
 * @file list.h
 * @brief This file contains functions for initializing and managing a linked list.
 * This implemntation is based on the C++ STL. The list is a doubly linked list.
 * The list is NOT type safe. The user must keep track of the type of data in the list.
 */

#pragma once

#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "array.h"

typedef struct list_t list_t;
typedef struct list_node_t list_node_t;
typedef struct list_iterator_t {
    list_t *list;
    list_node_t *node;
    int dir; // direction flag: 0 for forward, 1 for reverse
} list_iterator_t;

list_t *list_create(type_t type);
void list_destroy(list_t *list);

type_t list_type(list_t *list);
int list_type_size(list_t *list);

void *list_front(list_t *list);
void *list_back(list_t *list);
void *list_at(list_t *list, int index);

list_iterator_t list_begin(list_t *list);
list_iterator_t list_rbegin(list_t *list);
list_iterator_t list_end(list_t *list);
list_iterator_t list_rend(list_t *list);

int list_empty(list_t *list);
int list_size(list_t *list);

void list_clear(list_t *list);
list_iterator_t list_insert(list_t *list, list_iterator_t pos, void *data);
list_iterator_t list_insert_range(list_t *list, list_iterator_t pos, void *data, int n);
list_iterator_t list_erase(list_t *list, list_iterator_t start, list_iterator_t end);
void list_push_back(list_t *list, void *data);
void list_append_range(list_t *list, void *data, int n);
void list_pop_back(list_t *list);
void list_push_front(list_t *list, void *data);
void list_prepend_range(list_t *list, void *data, int n);
void list_pop_front(list_t *list);
void list_resize(list_t *list, int n, void *data);
void list_swap(list_t *list, list_t *other);
void list_reverse(list_t *list);

void list_merge(list_t *list, list_t *other);
void list_splice(list_t *list, list_iterator_t pos, list_t *other, list_iterator_t start, list_iterator_t end);
void list_remove(list_t *list, void *data);
void list_remove_if(list_t *list, predicate_func_t predicate);
void list_unique(list_t *list);
void list_sort(list_t *list, compare_func_t compare);

list_iterator_t list_iterator_next(list_iterator_t iterator);
list_iterator_t list_iterator_prev(list_iterator_t iterator);
list_iterator_t list_iterator_add(list_iterator_t iterator, int n);
list_iterator_t list_iterator_sub(list_iterator_t iterator, int n);
list_iterator_t list_iterator_advance(list_iterator_t iterator, int n);
int list_iterator_equal(list_iterator_t iterator, list_iterator_t other);
void *list_iterator_value(list_iterator_t iterator);