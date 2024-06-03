#include <string.h>
#include <stdlib.h>

#include "common.h"

#include "containers/vector.h"

struct vector_t {
    void *data;
    type_t type;
    int type_size;
    int size;
    int capacity;
};

void _vector_store_value(vector_t *vector, int index, void *data) {
    if (vector->type != TYPE_STRING && vector->type != TYPE_POINTER) {
        memcpy(vector->data + index * vector->type_size, data, vector->type_size);
    }
    else if (vector->type == TYPE_STRING) {

    }
    else {
        memcpy(vector->data + index * vector->type_size, &data, vector->type_size);
    }
}

vector_iterator_t _vector_iterator_create(vector_t *vector, int index, int dir) {
    vector_iterator_t it = {vector, index, dir};
    return it;
}

vector_t *vector_create(type_t type) {
    vector_t *vector = malloc(sizeof(vector_t));
    if (vector == NULL) {
        return NULL;
    }

    vector->data = NULL;
    vector->type = type;
    vector->type_size = TYPE_SIZE(type);
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

    return vector->data + index * vector->type_size;
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

    return vector->data + (vector->size - 1) * vector->type_size;
}

void *vector_data(vector_t *vector) {
    if (vector == NULL) {
        return NULL;
    }

    return vector->data;
}

vector_iterator_t vector_begin(vector_t *vector) {
    return _vector_iterator_create(vector, 0, 0);
}

vector_iterator_t vector_end(vector_t *vector) {
    return _vector_iterator_create(vector, vector->size, 0);
}

vector_iterator_t vector_rbegin(vector_t *vector) {
    return _vector_iterator_create(vector, vector->size - 1, 1);
}

vector_iterator_t vector_rend(vector_t *vector) {
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

    void *data = realloc(vector->data, n * vector->type_size);
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
    if (vector == NULL || vector->size == 0 || vector->size == vector->capacity) {
        return;
    }

    void *data = realloc(vector->data, vector->size * vector->type_size);
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

vector_iterator_t vector_insert(vector_t *vector, vector_iterator_t pos, void *data) {
    if (vector == NULL) {
        return _vector_iterator_create(NULL, 0, 0);
    }

    if (vector->size == vector->capacity) {
        vector_reserve(vector, vector->capacity * 2);
    }

    if (vector_iterator_equal(pos, vector_end(vector))) {
        vector_push_back(vector, data);
        return _vector_iterator_create(vector, vector->size - 1, 0);
    }

    memmove(vector->data + (pos.index + 1) * vector->type_size, vector->data + pos.index * vector->type_size, (vector->size - pos.index) * vector->type_size);
    memcpy(vector->data + pos.index * vector->type_size, data, vector->type_size);
    vector->size++;
    return _vector_iterator_create(vector, pos.index, 0);
}

vector_iterator_t vector_insert_range(vector_t *vector, vector_iterator_t pos, void *data, int n) {
    if (vector == NULL || pos.index < 0 || pos.index > vector->size || n <= 0) {
        return _vector_iterator_create(NULL, 0, 0);
    }

    if (vector->size + n > vector->capacity) {
        vector_reserve(vector, vector->size + n);
    }

    memmove(vector->data + (pos.index + n) * vector->type_size, vector->data + pos.index * vector->type_size, (vector->size - pos.index) * vector->type_size);
    memcpy(vector->data + pos.index * vector->type_size, data, n * vector->type_size);
    vector->size += n;
    return _vector_iterator_create(vector, pos.index, 0);
}

vector_iterator_t vector_erase(vector_t *vector, vector_iterator_t start, vector_iterator_t end) {
    if (vector == NULL || vector->size == 0 || vector_iterator_equal(start, end)) {
        return vector_end(vector);
    }

    if (vector_iterator_equal(start, vector_begin(vector)) && vector_iterator_equal(end, vector_end(vector))) {
        vector->size = 0;
        return vector_end(vector);
    }

    if (vector_iterator_equal(start, vector_begin(vector))) {
        vector->size -= end.index;
        memmove(vector->data, vector->data + end.index * vector->type_size, vector->size * vector->type_size);
        return vector_begin(vector);
    }

    if (vector_iterator_equal(end, vector_end(vector))) {
        vector->size = start.index;
        return vector_end(vector);
    }

    memmove(vector->data + start.index * vector->type_size, vector->data + end.index * vector->type_size, (vector->size - end.index) * vector->type_size);
    vector->size -= end.index - start.index;
    return _vector_iterator_create(vector, start.index, 0);
}

void vector_push_back(vector_t *vector, void *data) {
    if (vector == NULL) {
        return;
    }

    if (vector->size == vector->capacity) {
        if (vector->capacity == 0) {
            vector_reserve(vector, 1);
        }
        else {
            vector_reserve(vector, vector->capacity * 2);
        }
    }
    memcpy(vector->data + vector->size * vector->type_size, data, vector->type_size);
    vector->size++;
}

void vector_append_range(vector_t *vector, void *data, int n) {
    if (vector == NULL || n <= 0) {
        return;
    }

    if (vector->size + n > vector->capacity) {
        vector_reserve(vector, vector->size + n);
    }

    memcpy(vector->data + vector->size * vector->type_size, data, n * vector->type_size);
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

        for (int i = vector->size; i < n; i++) {
            memcpy(vector->data + i * vector->type_size, value, vector->type_size);
        }
        vector->size = n;
    }
    else {
        vector->size = n;
    }
}

void vector_swap(vector_t *vector, vector_t *other) {
    if (vector == NULL || other == NULL || vector->type != other->type) {
        return;
    }

    vector_t temp = *vector;
    vector->capacity = other->capacity;
    vector->size = other->size;
    vector->data = other->data;

    other->capacity = temp.capacity;
    other->size = temp.size;
    other->data = temp.data;
}

vector_iterator_t vector_iterator_next(vector_iterator_t it) {
    if (it.vector == NULL) {
        return _vector_iterator_create(NULL, 0, 0);
    }

    if (it.dir == 0) {
        it.index++;
    }
    else {
        it.index--;
    }

    return it;
}

vector_iterator_t vector_iterator_prev(vector_iterator_t it) {
    if (it.vector == NULL) {
        return _vector_iterator_create(NULL, 0, 0);
    }

    if (it.dir == 0) {
        it.index--;
    }
    else {
        it.index++;
    }

    return it;
}

int vector_iterator_equal(vector_iterator_t it1, vector_iterator_t it2) {
    return it1.vector == it2.vector && it1.index == it2.index;
}

void *vector_iterator_data(vector_iterator_t it) {
    if (it.vector == NULL) {
        return NULL;
    }

    if (it.index < 0 || it.index >= it.vector->size) {
        return NULL;
    }

    return it.vector->data + it.index * it.vector->type_size;
}
