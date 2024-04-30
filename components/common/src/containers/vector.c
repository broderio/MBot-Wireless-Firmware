#include "common.h"

#include "vector.h"

struct vector_t {
    void *data;
    type_t type;
    int size;
    int capacity;
};

struct vector_iterator_t {
    vector_t *vector;
    int index;
    int dir;
};

vector_iterator_t *_vector_iterator_create(vector_t *vector, int index, int dir) {
    vector_iterator_t *iterator = malloc(sizeof(vector_iterator_t));
    if (iterator == NULL) {
        return NULL;
    }

    iterator->vector = vector;
    iterator->index = index;
    iterator->dir = dir;
    return iterator;
}

vector_t *vector_create(type_t type) {
    vector_t *vector = malloc(sizeof(vector_t));
    if (vector == NULL) {
        return NULL;
    }

    vector->data = NULL;
    vector->type = type;
    vector->size = 0;
    vector->capacity = 0;
    return vector;
}

void vector_destroy(vector_t *vector) {
    if (vector == NULL) {
        return;
    }

    free(vector->data);
    free(vector);
}

void *vector_at(vector_t *vector, int index) {
    if (vector == NULL || vector->size == 0 || index < 0 || index >= vector->size) {
        return NULL;
    }

    return vector->data + index * type_size(vector->type);
}

void *vector_front(vector_t *vector) {
    if (vector == NULL || vector->size == 0) {
        return NULL;
    }

    return vector->data;
}

void *vector_back(vector_t *vector) {
    if (vector == NULL || vector->size == 0) {
        return NULL;
    }

    return vector->data + (vector->size - 1) * type_size(vector->type);
}

void *vector_data(vector_t *vector) {
    if (vector == NULL) {
        return NULL;
    }

    return vector->data;
}

vector_iterator_t *vector_begin(vector_t *vector) {
    return _vector_iterator_create(vector, 0, 0);
}

vector_iterator_t *vector_end(vector_t *vector) {
    return _vector_iterator_create(vector, vector->size, 0);
}

vector_iterator_t *vector_rbegin(vector_t *vector) {
    return _vector_iterator_create(vector, vector->size - 1, 1);
}

vector_iterator_t *vector_rend(vector_t *vector) {
    return _vector_iterator_create(vector, -1, 1);
}

int vector_empty(vector_t *vector) {
    if (vector == NULL) {
        return 1;
    }

    return vector->size == 0;
}

int vector_size(vector_t *vector) {
    if (vector == NULL) {
        return 0;
    }

    return vector->size;
}

void vector_reserve(vector_t *vector, int n) {
    if (vector == NULL || n <= vector->capacity) {
        return;
    }

    int type_size;
    if (vector->type != TYPE_STRING && vector->type != TYPE_ARRAY) {
        type_size = GENERIC_TYPE_SIZE(array->type);
    }
    else if (vector->type == TYPE_STRING) {
        type_size = sizeof(char*);
    }
    else {
        type_size = sizeof(array_t*);
    }

    void *data = realloc(vector->data, n * type_size);
    if (data == NULL) {
        return;
    }

    vector->data = data;
    vector->capacity = n;
}

int vector_capacity(vector_t *vector) {
    if (vector == NULL) {
        return 0;
    }

    return vector->capacity;
}

void vector_shrink_to_fit(vector_t *vector) {
    if (vector == NULL || vector->size == 0) {
        return;
    }

    int type_size;
    if (vector->type != TYPE_STRING && vector->type != TYPE_ARRAY) {
        type_size = GENERIC_TYPE_SIZE(array->type);
    }
    else if (vector->type == TYPE_STRING) {
        type_size = sizeof(char*);
    }
    else {
        type_size = sizeof(array_t*);
    }

    void *data = realloc(vector->data, n * type_size);
    if (data == NULL) {
        return;
    }

    vector->data = data;
    vector->capacity = vector->size;
}

void vector_clear(vector_t *vector) {
    if (vector == NULL) {
        return;
    }

    vector->size = 0;
}

vector_iterator_t *vector_insert(vector_t *vector, void *data, int index) {
    if (vector == NULL || index < 0 || index > vector->size) {
        return;
    }

    if (vector->size == vector->capacity) {
        vector_reserve(vector, vector->capacity * 2);
    }

    int type_size;
    if (vector->type != TYPE_STRING && vector->type != TYPE_ARRAY) {
        type_size = GENERIC_TYPE_SIZE(array->type);
    }
    else if (vector->type == TYPE_STRING) {
        type_size = sizeof(char*);
    }
    else {
        type_size = sizeof(array_t*);
    }

    memmove(vector->data + (index + 1) * type_size, vector->data + index * type_size, (vector->size - index) * type_size);
    memcpy(vector->data + index * type_size, data, type_size);
    vector->size++;
    return _vector_iterator_create(vector, index, 0);
}

vector_iterator_t *vector_insert_range(vector_t *vector, void *data, int index, int n) {
    if (vector == NULL || index < 0 || index > vector->size || n <= 0) {
        return;
    }

    if (vector->size + n > vector->capacity) {
        vector_reserve(vector, vector->size + n);
    }

    int type_size;
    if (vector->type != TYPE_STRING && vector->type != TYPE_ARRAY) {
        type_size = GENERIC_TYPE_SIZE(array->type);
    }
    else if (vector->type == TYPE_STRING) {
        type_size = sizeof(char*);
    }
    else {
        type_size = sizeof(array_t*);
    }

    memmove(vector->data + (index + n) * type_size, vector->data + index * type_size, (vector->size - index) * type_size);
    memcpy(vector->data + index * type_size, data, n * type_size);
    vector->size += n;
    return _vector_iterator_create(vector, index, 0);
}

void vector_erase(vector_t *vector, int index) {
    if (vector == NULL || index < 0 || index >= vector->size) {
        return;
    }

    int type_size;
    if (vector->type != TYPE_STRING && vector->type != TYPE_ARRAY) {
        type_size = GENERIC_TYPE_SIZE(array->type);
    }
    else if (vector->type == TYPE_STRING) {
        type_size = sizeof(char*);
    }
    else {
        type_size = sizeof(array_t*);
    }

    memmove(vector->data + index * type_size, vector->data + (index + 1) * type_size, (vector->size - index - 1) * type_size);
    vector->size--;
}

void vector_push_back(vector_t *vector, void *data) {
    if (vector == NULL) {
        return;
    }

    if (vector->size == vector->capacity) {
        vector_reserve(vector, vector->capacity * 2);
    }

    int type_size;
    if (vector->type != TYPE_STRING && vector->type != TYPE_ARRAY) {
        type_size = GENERIC_TYPE_SIZE(array->type);
    }
    else if (vector->type == TYPE_STRING) {
        type_size = sizeof(char*);
    }
    else {
        type_size = sizeof(array_t*);
    }

    memcpy(vector->data + vector->size * type_size, data, type_size);
    vector->size++;
}

void vector_append_range(vector_t *vector, void *data, int n) {
    if (vector == NULL || n <= 0) {
        return;
    }

    if (vector->size + n > vector->capacity) {
        vector_reserve(vector, vector->size + n);
    }

    int type_size;
    if (vector->type != TYPE_STRING && vector->type != TYPE_ARRAY) {
        type_size = GENERIC_TYPE_SIZE(array->type);
    }
    else if (vector->type == TYPE_STRING) {
        type_size = sizeof(char*);
    }
    else {
        type_size = sizeof(array_t*);
    }

    memcpy(vector->data + vector->size * type_size, data, n * type_size);
    vector->size += n;
}

void vector_pop_back(vector_t *vector) {
    if (vector == NULL || vector->size == 0) {
        return;
    }

    vector->size--;
}

void vector_resize(vector_t *vector, int n, void *value) {
    if (vector == NULL || n < 0) {
        return;
    }

    if (n > vector->size) {
        if (n > vector->capacity) {
            vector_reserve(vector, n);
        }

        int type_size;
        if (vector->type != TYPE_STRING && vector->type != TYPE_ARRAY) {
            type_size = GENERIC_TYPE_SIZE(array->type);
        }
        else if (vector->type == TYPE_STRING) {
            type_size = sizeof(char*);
        }
        else {
            type_size = sizeof(array_t*);
        }

        memset(vector->data + vector->size * type_size, value, (n - vector->size) * type_size);
        vector->size = n;
    }
    else {
        vector->size = n;
    }
}

void vector_swap(vector_t *vector, vector_t *other) {
    if (vector == NULL || other == NULL) {
        return;
    }

    vector_t temp = *vector;
    *vector = *other;
    *other = temp;
}

vector_iterator_t *vector_iterator_next(vector_t *vector, vector_iterator_t *iterator) {
    if (vector == NULL || iterator == NULL) {
        return NULL;
    }

    if (iterator->dir == 0) {
        iterator->index++;
    }
    else {
        iterator->index--;
    }

    return iterator;
}

vector_iterator_t *vector_iterator_prev(vector_t *vector, vector_iterator_t *iterator) {
    if (vector == NULL || iterator == NULL) {
        return NULL;
    }

    if (iterator->dir == 0) {
        iterator->index--;
    }
    else {
        iterator->index++;
    }

    return iterator;
}

int vector_iterator_equal(vector_iterator_t *iterator1, vector_iterator_t *iterator2) {
    if (iterator1 == NULL || iterator2 == NULL) {
        return 0;
    }

    return iterator1->vector == iterator2->vector && iterator1->index == iterator2->index;
}

void *vector_iterator_data(vector_t *vector, vector_iterator_t *iterator) {
    if (vector == NULL || iterator == NULL) {
        return NULL;
    }

    if (iterator->index < 0 || iterator->index >= vector->size) {
        return NULL;
    }

    int type_size;
    if (vector->type != TYPE_STRING && vector->type != TYPE_ARRAY) {
        type_size = GENERIC_TYPE_SIZE(array->type);
    }
    else if (vector->type == TYPE_STRING) {
        type_size = sizeof(char*);
    }
    else {
        type_size = sizeof(array_t*);
    }

    return vector->data + iterator->index * type_size;
}
