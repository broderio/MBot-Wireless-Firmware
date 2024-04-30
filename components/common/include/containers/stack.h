/**
 * @file stack.h
 * @brief This file contains functions for initializing and managing a stack.
 * This implemntation is based on the C++ STL. The stack is a wrapper around a vector.
 */

#pragma once

#include "common.h"
#include "vector.h"

typedef struct stack_t stack_t;

stack_t *stack_create(type_t type);
void stack_destroy(stack_t *stack);

void* stack_top(stack_t *stack);

int stack_empty(stack_t *stack);
int stack_size(stack_t *stack);

void stack_push(stack_t *stack, void *data);
void stack_push_range(stack_t *stack, void *data, int n);
void stack_pop(stack_t *stack);
void stack_swap(stack_t *stack, stack_t *other);