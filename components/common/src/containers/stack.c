#include "common.h"
#include "vector.h"

#include "stack.h"

struct stack_t {
    vector_t *vector;
};

stack_t *stack_create(type_t type) {
    stack_t *stack = (stack_t*)malloc(sizeof(stack_t));
    if (stack == NULL) {
        return NULL;
    }

    stack->vector = vector_create(type);
    if (stack->vector == NULL) {
        free(stack);
        return NULL;
    }

    return stack;
}

void stack_destroy(stack_t *stack) {
    if (stack == NULL) {
        return;
    }

    vector_destroy(stack->vector);
    free(stack);
}


void* stack_top(stack_t *stack) {
    if (stack == NULL) {
        return NULL;
    }

    return vector_back(stack->vector);
}

int stack_empty(stack_t *stack) {
    if (stack == NULL) {
        return 1;
    }

    return vector_empty(stack->vector);
}

int stack_size(stack_t *stack) {
    if (stack == NULL) {
        return 0;
    }

    return vector_size(stack->vector);
}

void stack_push(stack_t *stack, void *data) {
    if (stack == NULL) {
        return;
    }

    vector_push_back(stack->vector, data);
}

void stack_push_range(stack_t *stack, void *data, int n) {
    if (stack == NULL) {
        return;
    }

    vector_push_back_range(stack->vector, data, n);
}

void stack_pop(stack_t *stack) {
    if (stack == NULL) {
        return;
    }

    vector_pop_back(stack->vector);
}

void stack_swap(stack_t *stack, stack_t *other) {
    if (stack == NULL || other == NULL) {
        return;
    }

    vector_swap(stack->vector, other->vector);
}
