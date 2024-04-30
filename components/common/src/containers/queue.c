#include "common.h"
#include "list.h"

#include "queue.h"

struct queue_t {
    list_t *list;
};

queue_t *queue_create(type_t type) {
    queue_t *queue = (queue_t*)malloc(sizeof(queue_t));
    if (queue == NULL) {
        return NULL;
    }

    queue->list = list_create(type);
    if (queue->list == NULL) {
        free(queue);
        return NULL;
    }

    return queue;
}

void queue_destroy(queue_t *queue) {
    if (queue == NULL) {
        return;
    }

    list_destroy(queue->list);
    free(queue);
}

void *queue_front(queue_t *queue) {
    if (queue == NULL) {
        return NULL;
    }

    return list_front(queue->list);
}

void *queue_back(queue_t *queue) {
    if (queue == NULL) {
        return NULL;
    }

    return list_back(queue->list);
}

int queue_empty(queue_t *queue) {
    if (queue == NULL) {
        return 1;
    }

    return list_empty(queue->list);
}

int queue_size(queue_t *queue) {
    if (queue == NULL) {
        return 0;
    }

    return list_size(queue->list);
}

void queue_push(queue_t *queue, void *data) {
    if (queue == NULL) {
        return;
    }

    list_push_back(queue->list, data);
}

void queue_push_range(queue_t *queue, void *data, int n) {
    if (queue == NULL) {
        return;
    }

    list_append_range(queue->list, data, n);
}

void queue_pop(queue_t *queue) {
    if (queue == NULL) {
        return;
    }

    list_pop_front(queue->list);
}

void queue_swap(queue_t *queue, queue_t *other) {
    if (queue == NULL || other == NULL) {
        return;
    }

    list_t *list = queue->list;
    queue->list = other->list;
    other->list = list;
}
