#include <stdio.h>

#include "esp_log.h"

#include "common.h"
#include "containers/array.h"
#include "containers/vector.h"

/*
vector_t *vector_create(type_t type);                                                           x
void vector_destroy(vector_t *vector);                                                          x

void *vector_at(vector_t *vector, int index);                                                   x
void *vector_front(vector_t *vector);                                                           x
void *vector_back(vector_t *vector);                                                            x
void *vector_data(vector_t *vector);                                                            x

vector_iterator_t *vector_begin(vector_t *vector);
vector_iterator_t *vector_end(vector_t *vector);
vector_iterator_t *vector_rbegin(vector_t *vector);
vector_iterator_t *vector_rend(vector_t *vector);

int vector_empty(vector_t *vector);                                                             x
int vector_size(vector_t *vector);                                                              x
void vector_reserve(vector_t *vector, int n);                                                   x
int vector_capacity(vector_t *vector);                                                          x                      
void vector_shrink_to_fit(vector_t *vector);                                                    x                       

void vector_clear(vector_t *vector);
vector_iterator_t *vector_insert(vector_t *vector, void *data, int index);
vector_iterator_t *vector_insert_range(vector_t *vector, void *data, int index, int n);
void vector_erase(vector_t *vector, int index);
void vector_push_back(vector_t *vector, void *data);                                            x
void vector_append_range(vector_t *vector, void *data, int n);
void vector_pop_back(vector_t *vector);                                                         x
void vector_resize(vector_t *vector, int n, void *value);                                       x
void vector_swap(vector_t *vector, vector_t *other);

vector_iterator_t *vector_iterator_next(vector_t *vector, vector_iterator_t *iterator);
vector_iterator_t *vector_iterator_prev(vector_t *vector, vector_iterator_t *iterator);
int vector_iterator_equal(vector_iterator_t *iterator1, vector_iterator_t *iterator2);
void *vector_iterator_data(vector_t *vector, vector_iterator_t *iterator);
*/

typedef struct test_obj_t
{
    int a;
    float b;
    double c;
} test_obj_t;

// Test: all types, create, destroy, at, front, back, data, push_back, empty, size
void vector_test_1() {
    vector_t *v_char = vector_create(TYPE_CHAR);
    vector_t *v_int = vector_create(TYPE_INT);
    vector_t *v_float = vector_create(TYPE_FLOAT);
    vector_t *v_double = vector_create(TYPE_DOUBLE);
    vector_t *v_ptr = vector_create(TYPE_POINTER);
    vector_t *v_str = vector_create(TYPE_STRING);

    assert(v_char != NULL);
    assert(v_int != NULL);
    assert(v_float != NULL);
    assert(v_double != NULL);
    assert(v_ptr != NULL);
    assert(v_str != NULL);

    assert(vector_empty(v_char) == 1);
    assert(vector_empty(v_int) == 1);
    assert(vector_empty(v_float) == 1);
    assert(vector_empty(v_double) == 1);
    assert(vector_empty(v_ptr) == 1);
    assert(vector_empty(v_str) == 1);

    char char_data = 'a';
    int int_data = 123;
    float float_data = 123.456;
    double double_data = 123.456789;
    test_obj_t *obj_data = malloc(sizeof(test_obj_t));
    obj_data->a = 123;
    obj_data->b = 123.456;
    obj_data->c = 123.456789;
    char *str_data = "Hello, World!";

    vector_push_back(v_char, &char_data);
    vector_push_back(v_int, &int_data);
    vector_push_back(v_float, &float_data);
    vector_push_back(v_double, &double_data);
    vector_push_back(v_ptr, &obj_data);
    vector_push_back(v_str, &str_data);

    assert(vector_empty(v_char) == 0);
    assert(vector_empty(v_int) == 0);
    assert(vector_empty(v_float) == 0);
    assert(vector_empty(v_double) == 0);
    assert(vector_empty(v_ptr) == 0);
    assert(vector_empty(v_str) == 0);

    assert(vector_size(v_char) == 1);
    assert(vector_size(v_int) == 1);
    assert(vector_size(v_float) == 1);
    assert(vector_size(v_double) == 1);
    assert(vector_size(v_ptr) == 1);
    assert(vector_size(v_str) == 1);

    assert(*(char *)vector_at(v_char, 0) == char_data);
    assert(*(int *)vector_at(v_int, 0) == int_data);
    assert(*(float *)vector_at(v_float, 0) == float_data);
    assert(*(double *)vector_at(v_double, 0) == double_data);
    test_obj_t **obj = vector_at(v_ptr, 0);
    ESP_LOGW("vector", "obj->a: %d, obj->b: %f, obj->c: %f", (*obj)->a, (*obj)->b, (*obj)->c);
    assert(obj->a == obj_data->a);
    assert(obj->b == obj_data->b);
    assert(obj->c == obj_data->c);
    assert(*(char **)vector_at(v_str, 0) == str_data);

    assert(*(char *)vector_front(v_char) == char_data);
    assert(*(int *)vector_front(v_int) == int_data);
    assert(*(float *)vector_front(v_float) == float_data);
    assert(*(double *)vector_front(v_double) == double_data);
    obj = vector_front(v_ptr);
    assert(obj->a == obj_data->a);
    assert(obj->b == obj_data->b);
    assert(obj->c == obj_data->c);
    assert(*(char **)vector_front(v_str) == str_data);

    assert(*(char *)vector_back(v_char) == char_data);
    assert(*(int *)vector_back(v_int) == int_data);
    assert(*(float *)vector_back(v_float) == float_data);
    assert(*(double *)vector_back(v_double) == double_data);
    obj = vector_back(v_ptr);
    assert(obj->a == obj_data->a);
    assert(obj->b == obj_data->b);
    assert(obj->c == obj_data->c);
    assert(*(char **)vector_back(v_str) == str_data);

    assert(*(char *)vector_data(v_char) == char_data);
    assert(*(int *)vector_data(v_int) == int_data);
    assert(*(float *)vector_data(v_float) == float_data);
    assert(*(double *)vector_data(v_double) == double_data);
    obj = vector_data(v_ptr);
    assert(obj->a == obj_data->a);
    assert(obj->b == obj_data->b);
    assert(obj->c == obj_data->c);
    assert(*(char **)vector_data(v_str) == str_data);

    vector_destroy(v_char);
    vector_destroy(v_int);
    vector_destroy(v_float);
    vector_destroy(v_double);
    vector_destroy(v_ptr);
    vector_destroy(v_str);
}

// Test: create, destroy, at, front, back, data, push_back, pop_back, empty, size
void vector_test_2() {
    vector_t *v = vector_create(TYPE_INT);

    assert(v != NULL);

    assert(vector_empty(v) == 1);

    int data[] = {1, 2, 3, 4, 5};

    for (int i = 0; i < 5; i++) {
        vector_push_back(v, &data[i]);
    }

    assert(vector_empty(v) == 0);
    assert(vector_size(v) == 5);

    for (int i = 0; i < 5; i++) {
        assert(*(int *)vector_at(v, i) == data[i]);
    }

    assert(*(int *)vector_front(v) == data[0]);
    assert(*(int *)vector_back(v) == data[4]);
    assert(*(int *)vector_data(v) == data[0]);

    vector_pop_back(v);
    
    assert(vector_size(v) == 4);
    assert(*(int *)vector_back(v) == data[3]);

    vector_destroy(v);
}

// Test: create, destroy, at, reserve, capacity, shrink_to_fit, resize, empty, size
void vector_test_3() {
    vector_t *v = vector_create(TYPE_INT);

    assert(v != NULL);

    assert(vector_empty(v) == 1);

    vector_reserve(v, 10);

    assert(vector_capacity(v) == 10);

    vector_shrink_to_fit(v);

    assert(vector_capacity(v) == 0);

    int data[] = {1, 2, 3, 4, 5};

    for (int i = 0; i < 5; i++) {
        vector_push_back(v, &data[i]);
    }

    assert(vector_empty(v) == 0);
    assert(vector_size(v) == 5);

    vector_resize(v, 10, &data[0]);

    assert(vector_size(v) == 10);

    for (int i = 0; i < 10; i++) {
        assert(*(int *)vector_at(v, i) == (i < 5 ? data[i] : data[0]));
    }

    vector_resize(v, 7, &data[0]);

    assert(vector_size(v) == 7);

    for (int i = 0; i < 7; i++) {
        assert(*(int *)vector_at(v, i) == (i < 5 ? data[i] : data[0]));
    }

    int cap = vector_capacity(v);
    vector_shrink_to_fit(v);

    assert(vector_capacity(v) < cap);

    vector_destroy(v);
}

// Test: create, destroy, insert, insert_range, erase, empty, size

void app_main() {
    ESP_LOGI("vector", "Running vector tests...");
    ESP_LOGI("vector", "Test 1");
    vector_test_1();
    ESP_LOGI("vector", "Test 2");
    vector_test_2();
    ESP_LOGI("vector", "Test 3");
    vector_test_3();
    ESP_LOGI("vector", "All tests passed!");
}