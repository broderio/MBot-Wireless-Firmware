#include <string.h>
#include <stdlib.h>

#include "common.h"

#include "containers/array.h"

struct array_t {
    void *data;
    type_t type;
    int type_size;
    int size;
};

array_iterator_t _array_iterator_create(array_t *array, int index, int dir) {
    array_iterator_t iterator = {
        .array = array,
        .index = index,
        .dir = dir
    };
    return iterator;
}

array_t *array_create(type_t type, int size) {
    array_t *array = (array_t*)malloc(sizeof(array_t));
    if (array == NULL) {
        return NULL;
    }
    array->type = type;

    if (array->type != TYPE_STRING) {
        array->type_size = TYPE_SIZE(array->type);
    }
    else {
        array->type_size = sizeof(char*);
    }

    array->data = (void*)malloc(array->type_size * size);
    array->size = size;

    return array;
}

void array_destroy(array_t *array) {
    if (array == NULL) {
        return;
    }

    free(array->data);
    free(array);
}

type_t array_type(array_t *array) {
    if (array == NULL) {
        return TYPE_ERROR;
    }

    return array->type;
}

void *array_at(array_t *array, int index) {
    if (array == NULL || index < 0 || index >= array->size) {
        return NULL;
    }

    return array->data + index * array->type_size;
}

void *array_front(array_t *array) {
    if (array == NULL) {
        return NULL;
    }

    return array->data;
}

void *array_back(array_t *array) {
    if (array == NULL) {
        return NULL;
    }

    return array->data + (array->size - 1) * array->type_size;
}

array_iterator_t array_begin(array_t *array) {
    if (array == NULL) {
        return _array_iterator_create(NULL, 0, 0);
    }

    return _array_iterator_create(array, 0, 0);
}

array_iterator_t array_rbegin(array_t *array) {
    if (array == NULL) {
        return _array_iterator_create(NULL, 0, 1);
    }

    return _array_iterator_create(array, array->size - 1, 1);
}

array_iterator_t array_end(array_t *array) {
    if (array == NULL) {
        return _array_iterator_create(NULL, 0, 0);
    }

    return _array_iterator_create(array, array->size, 0);
}

array_iterator_t array_rend(array_t *array) {
    if (array == NULL) {
        return _array_iterator_create(NULL, 0, 1);
    }

    return _array_iterator_create(array, -1, 1);
}

int array_empty(array_t *array) {
    if (array == NULL) {
        return 1;
    }

    return array->size == 0;
}

int array_size(array_t *array) {
    if (array == NULL) {
        return 0;
    }

    return array->size;
}

void array_fill(array_t *array, void *value) {
    if (array == NULL || value == NULL) {
        return;
    }

    for (int i = 0; i < array->size; i++) {
        memcpy(array->data + i * array->type_size, value, array->type_size);
    }
}

void array_swap(array_t *array, array_t *other) {
    if (array == NULL || other == NULL) {
        return;
    }

    void *data = array->data;
    type_t type = array->type;
    int size = array->size;

    array->data = other->data;
    array->type = other->type;
    array->size = other->size;

    other->data = data;
    other->type = type;
    other->size = size;
}

void array_iterator_next(array_iterator_t *iterator) {
    if (iterator == NULL) {
        return;
    }

    iterator->index += iterator->dir == 0 ? 1 : -1;
}

void array_iterator_prev(array_iterator_t *iterator) {
    if (iterator == NULL) {
        return;
    }

    iterator->index -= iterator->dir == 0 ? 1 : -1;
}

void *array_iterator_value(array_iterator_t *iterator) {
    if (iterator == NULL) {
        return NULL;
    }

    if (iterator->index < 0 || iterator->index >= iterator->array->size) {
        return NULL;
    }

    return iterator->array->data + iterator->index * iterator->array->type_size;
}

int array_iterator_equal(array_iterator_t *iterator, array_iterator_t *other) {
    if (iterator == NULL || other == NULL) {
        return 0;
    }
    return iterator->array == other->array && iterator->index == other->index;
}
