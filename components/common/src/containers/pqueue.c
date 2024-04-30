#include "common.h"

#include "vector.h"
#include "pqueue.h"

struct pqueue_t {
    vector_t *vector;
    compare_func_t compare;
};

pqueue_t *pqueue_create(type_t type, compare_func_t compare) {
    pqueue_t *pqueue = (pqueue_t*)malloc(sizeof(pqueue_t));
    if (pqueue == NULL) {
        return NULL;
    }

    pqueue->vector = vector_create(type);
    if (pqueue->vector == NULL) {
        free(pqueue);
        return NULL;
    }

    pqueue->compare = compare;
    return pqueue;
}

void pqueue_destroy(pqueue_t *pqueue) {
    if (pqueue == NULL) {
        return;
    }

    vector_destroy(pqueue->vector);
    free(pqueue);
}

void *pqueue_top(pqueue_t *pqueue) {
    if (pqueue == NULL) {
        return NULL;
    }

    return vector_front(pqueue->vector);
}

int pqueue_empty(pqueue_t *pqueue) {
    if (pqueue == NULL) {
        return 1;
    }

    return vector_empty(pqueue->vector);
}

int pqueue_size(pqueue_t *pqueue) {
    if (pqueue == NULL) {
        return 0;
    }

    return vector_size(pqueue->vector);
}

void pqueue_push(pqueue_t *pqueue, void *data) {
    if (pqueue == NULL) {
        return;
    }

    vector_push_back(pqueue->vector, data);

    int i = vector_size(pqueue->vector) - 1;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pqueue->compare(vector_at(pqueue->vector, i), vector_at(pqueue->vector, parent)) >= 0) {
            break;
        }

        // Manually swap elements
        void* temp = vector_at(pqueue->vector, i);
        vector_at(pqueue->vector, i) = vector_at(pqueue->vector, parent);
        vector_at(pqueue->vector, parent) = temp;

        i = parent;
    }
}

void pqueue_push_range(pqueue_t *pqueue, void *data, int n) {
    if (pqueue == NULL || data == NULL) {
        return;
    }

    for (int i = 0; i < n; i++) {
        pqueue_push(pqueue, (char*)data + i * pqueue->vector->type_size);
    }
}

void pqueue_pop(pqueue_t *pqueue) {
    if (pqueue == NULL || vector_empty(pqueue->vector)) {
        return;
    }

    vector_at(pqueue->vector, 0) = vector_back(pqueue->vector);
    vector_pop_back(pqueue->vector);

    int i = 0;
    int size = vector_size(pqueue->vector);
    while (true) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < size && pqueue->compare(vector_at(pqueue->vector, left), vector_at(pqueue->vector, smallest)) < 0) {
            smallest = left;
        }

        if (right < size && pqueue->compare(vector_at(pqueue->vector, right), vector_at(pqueue->vector, smallest)) < 0) {
            smallest = right;
        }

        if (smallest == i) {
            break;
        }

        void* temp = vector_at(pqueue->vector, i);
        vector_at(pqueue->vector, i) = vector_at(pqueue->vector, smallest);
        vector_at(pqueue->vector, smallest) = temp;

        i = smallest;
    }
}

void pqueue_swap(pqueue_t *pqueue, pqueue_t *other) {
    if (pqueue == NULL || other == NULL) {
        return;
    }

    pqueue_t temp = *pqueue;
    *pqueue = *other;
    *other = temp;
}
