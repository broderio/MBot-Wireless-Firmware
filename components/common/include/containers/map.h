/**
 * @file map.h
 * @brief This file contains functions for initializing and managing a map.
 * This implemntation is based on the C++ STL. The map is a red-black tree.
 */

#pragma once

#include "common.h"

typedef struct map_t map_t;
typedef struct map_node_t map_node_t;
typedef struct map_iterator_t map_iterator_t;

map_t *map_create(type_t key_type, type_t value_type, compare_func_t compare);
void map_destroy(map_t *map);

void *map_at(map_t *map, void *key);

map_iterator_t *map_begin(map_t *map);
map_iterator_t *map_rbegin(map_t *map);
map_iterator_t *map_end(map_t *map);
map_iterator_t *map_rend(map_t *map);

int map_empty(map_t *map);
int map_size(map_t *map);

void map_clear(map_t *map);
void map_insert(map_t *map, void *key, void *value);
void map_insert_range(map_t *map, void *key, void *value, int n);
void map_insert_or_assign(map_t *map, void *key, void *value);
void map_erase(map_t *map, void *key);
void map_swap(map_t *map, map_t *other);
map_node_t *map_extract(map_t *map, void *key);
void map_merge(map_t *map1, map_t *map2);

int map_count(map_t *map, void *key);
void *map_find(map_t *map, void *key);
int map_contains(map_t *map, void *key);
void map_equal_range(map_t *map, void *key, map_iterator_t **first, map_iterator_t **last);
map_iterator_t *map_lower_bound(map_t *map, void *key);
map_iterator_t *map_upper_bound(map_t *map, void *key);

compare_func_t map_key_compare(map_t *map);

map_iterator_t *map_iterator_next(map_t *map, map_iterator_t *iterator);
map_iterator_t *map_iterator_prev(map_t *map, map_iterator_t *iterator);
map_node_t *map_iterator_node(map_t *map, map_iterator_t *iterator);

void *map_node_key(map_t *map, map_node_t *node);
void *map_node_value(map_t *map, map_node_t *node);