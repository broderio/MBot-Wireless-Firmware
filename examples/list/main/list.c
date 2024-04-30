#include <stdio.h>

#include "esp_log.h"

#include "common.h"
#include "containers/array.h"
#include "containers/list.h"

typedef struct test_obj_t
{
    int a;
    float b;
    double c;
} test_obj_t;

// Covers: all types, list_create, list_insert, list_at, list_destroy
void list_test_1()
{
    ESP_LOGI("TEST1", "Creating lists ...");
    list_t *list_int = list_create(TYPE_INT);
    list_t *list_float = list_create(TYPE_FLOAT);
    list_t *list_double = list_create(TYPE_DOUBLE);
    list_t *list_char = list_create(TYPE_CHAR);
    list_t *list_string = list_create(TYPE_STRING);
    list_t *list_pointer = list_create(TYPE_POINTER);
    list_t *list_array = list_create(TYPE_POINTER);

    assert(list_int != NULL && list_type(list_int) == TYPE_INT && list_type_size(list_int) == sizeof(int));
    assert(list_float != NULL && list_type(list_float) == TYPE_FLOAT && list_type_size(list_float) == sizeof(float));
    assert(list_double != NULL && list_type(list_double) == TYPE_DOUBLE && list_type_size(list_double) == sizeof(double));
    assert(list_char != NULL && list_type(list_char) == TYPE_CHAR && list_type_size(list_char) == sizeof(char));
    assert(list_string != NULL && list_type(list_string) == TYPE_STRING && list_type_size(list_string) == sizeof(char *));
    assert(list_pointer != NULL && list_type(list_pointer) == TYPE_POINTER && list_type_size(list_pointer) == sizeof(void *));
    assert(list_array != NULL && list_type(list_array) == TYPE_POINTER && list_type_size(list_array) == sizeof(void *));

    int int_data = 10;
    float float_data = 10.0;
    double double_data = 10.0;
    char char_data = 'a';
    char *string_data = "Hello";

    test_obj_t obj_data = {10, 10.0, 10.0};

    array_t *array_data = array_create(TYPE_INT, 3);
    int array_val = 10;
    *(int *)array_at(array_data, 0) = array_val++;
    *(int *)array_at(array_data, 1) = array_val++;
    *(int *)array_at(array_data, 2) = array_val;

    ESP_LOGI("TEST1", "Inserting data ...");
    list_insert(list_int, list_begin(list_int), &int_data);
    list_insert(list_float, list_begin(list_float), &float_data);
    list_insert(list_double, list_begin(list_double), &double_data);
    list_insert(list_char, list_begin(list_char), &char_data);
    list_insert(list_string, list_begin(list_string), string_data);
    list_insert(list_pointer, list_begin(list_pointer), &obj_data);
    list_insert(list_array, list_begin(list_array), array_data);

    ESP_LOGI("TEST1", "Reading data ...");
    assert(*(int *)list_at(list_int, 0) == int_data);
    assert(*(float *)list_at(list_float, 0) == float_data);
    assert(*(double *)list_at(list_double, 0) == double_data);
    assert(*(char *)list_at(list_char, 0) == char_data);
    assert(strcmp((char *)list_at(list_string, 0), string_data) == 0);

    test_obj_t val = *(test_obj_t *)list_at(list_pointer, 0);
    assert(val.a == obj_data.a && val.b == obj_data.b && val.c == obj_data.c);

    int array_val_0 = *(int *)array_at((array_t *)list_at(list_array, 0), 0);
    int array_val_1 = *(int *)array_at((array_t *)list_at(list_array, 0), 1);
    int array_val_2 = *(int *)array_at((array_t *)list_at(list_array, 0), 2);
    assert(array_val_0 == 10 && array_val_1 == 11 && array_val_2 == 12);

    ESP_LOGI("TEST1", "Destroying lists ...");
    list_destroy(list_int);
    list_destroy(list_float);
    list_destroy(list_double);
    list_destroy(list_char);
    list_destroy(list_string);
    list_destroy(list_pointer);
    list_destroy(list_array);
    array_destroy(array_data);
}

// Covers: all types, list_create, list_push_back, list_push_front, list_pop_back, list_pop_front, list_size, list_destroy
void list_test_2()
{
    // Get initial heap size
    uint32_t heap_size = esp_get_free_heap_size();

    ESP_LOGI("TEST2", "Creating lists ...");
    list_t *list_int = list_create(TYPE_INT);
    list_t *list_float = list_create(TYPE_FLOAT);
    list_t *list_double = list_create(TYPE_DOUBLE);
    list_t *list_char = list_create(TYPE_CHAR);
    list_t *list_string = list_create(TYPE_STRING);
    list_t *list_pointer = list_create(TYPE_POINTER);
    list_t *list_array = list_create(TYPE_POINTER);

    ESP_LOGI("TEST2", "Heap size after creating lists: %lu", heap_size - esp_get_free_heap_size());

    ESP_LOGI("TEST2", "Pushing data to back ...");

    int int_data = 10;
    list_push_back(list_int, &int_data);
    int_data += 10;
    list_push_back(list_int, &int_data);
    int_data += 10;
    list_push_back(list_int, &int_data);

    assert(list_size(list_int) == 3);

    float float_data = 10.0;
    list_push_back(list_float, &float_data);
    float_data += 10.0;
    list_push_back(list_float, &float_data);
    float_data += 10.0;
    list_push_back(list_float, &float_data);

    assert(list_size(list_float) == 3);

    double double_data = 10.0;
    list_push_back(list_double, &double_data);
    double_data += 10.0;
    list_push_back(list_double, &double_data);
    double_data += 10.0;
    list_push_back(list_double, &double_data);

    assert(list_size(list_double) == 3);

    char char_data = 'a';
    list_push_back(list_char, &char_data);
    char_data++;
    list_push_back(list_char, &char_data);
    char_data++;
    list_push_back(list_char, &char_data);

    assert(list_size(list_char) == 3);

    char *string_data = "Hello";
    list_push_back(list_string, string_data);
    string_data = "World";
    list_push_back(list_string, string_data);
    string_data = "!";
    list_push_back(list_string, string_data);

    assert(list_size(list_string) == 3);

    test_obj_t obj_data = {10, 10.0, 10.0};
    list_push_back(list_pointer, &obj_data);
    test_obj_t obj_data_2 = {20, 20.0, 20.0};
    list_push_back(list_pointer, &obj_data_2);
    test_obj_t obj_data_3 = {30, 30.0, 30.0};
    list_push_back(list_pointer, &obj_data_3);

    assert(list_size(list_pointer) == 3);

    array_t *array_data = array_create(TYPE_INT, 3);
    int array_val = 0;
    *(int *)array_at(array_data, 0) = array_val++;
    *(int *)array_at(array_data, 1) = array_val++;
    *(int *)array_at(array_data, 2) = array_val++;
    list_push_back(list_array, array_data);
    array_data = array_create(TYPE_INT, 3);
    *(int *)array_at(array_data, 0) = array_val++;
    *(int *)array_at(array_data, 1) = array_val++;
    *(int *)array_at(array_data, 2) = array_val++;
    list_push_back(list_array, array_data);
    array_data = array_create(TYPE_INT, 3);
    *(int *)array_at(array_data, 0) = array_val++;
    *(int *)array_at(array_data, 1) = array_val++;
    *(int *)array_at(array_data, 2) = array_val++;
    list_push_back(list_array, array_data);

    assert(list_size(list_array) == 3);

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == 10 + i * 10);
    }

    for (int i = 0; i < list_size(list_float); i++)
    {
        assert(*(float *)list_at(list_float, i) == 10.0 + i * 10.0);
    }

    for (int i = 0; i < list_size(list_double); i++)
    {
        assert(*(double *)list_at(list_double, i) == 10.0 + i * 10.0);
    }

    for (int i = 0; i < list_size(list_char); i++)
    {
        assert(*(char *)list_at(list_char, i) == 'a' + i);
    }

    for (int i = 0; i < list_size(list_string); i++)
    {
        assert(strcmp((char *)list_at(list_string, i), i == 0 ? "Hello" : (i == 1 ? "World" : "!")) == 0);
    }

    for (int i = 0; i < list_size(list_pointer); i++)
    {
        test_obj_t val = *(test_obj_t *)list_at(list_pointer, i);
        assert(val.a == 10 + i * 10 && val.b == 10.0 + i * 10.0 && val.c == 10.0 + i * 10.0);
    }

    for (int i = 0; i < list_size(list_array); i++)
    {
        int array_val_0 = *(int *)array_at((array_t *)list_at(list_array, i), 0);
        int array_val_1 = *(int *)array_at((array_t *)list_at(list_array, i), 1);
        int array_val_2 = *(int *)array_at((array_t *)list_at(list_array, i), 2);
        assert(array_val_0 == i * 3 && array_val_1 == 1 + i * 3 && array_val_2 == 2 + i * 3);
    }

    ESP_LOGI("TEST2", "Pushing data to front ...");

    int_data = 0;
    list_push_front(list_int, &int_data);
    int_data -= 10;
    list_push_front(list_int, &int_data);
    int_data -= 10;
    list_push_front(list_int, &int_data);

    assert(list_size(list_int) == 6);

    float_data = 0.0;
    list_push_front(list_float, &float_data);
    float_data -= 10.0;
    list_push_front(list_float, &float_data);
    float_data -= 10.0;
    list_push_front(list_float, &float_data);

    assert(list_size(list_float) == 6);

    double_data = 0.0;
    list_push_front(list_double, &double_data);
    double_data -= 10.0;
    list_push_front(list_double, &double_data);
    double_data -= 10.0;
    list_push_front(list_double, &double_data);

    assert(list_size(list_double) == 6);

    char_data = 'a' - 1;
    list_push_front(list_char, &char_data);
    char_data -= 1;
    list_push_front(list_char, &char_data);
    char_data -= 1;
    list_push_front(list_char, &char_data);

    assert(list_size(list_char) == 6);

    string_data = "!";
    list_push_front(list_string, string_data);
    string_data = "push_front";
    list_push_front(list_string, string_data);
    string_data = "Testing";
    list_push_front(list_string, string_data);

    assert(list_size(list_string) == 6);

    test_obj_t obj_data_4 = {0, 0.0, 0.0};
    list_push_front(list_pointer, &obj_data_4);
    test_obj_t obj_data_5 = {-10, -10.0, -10.0};
    list_push_front(list_pointer, &obj_data_5);
    test_obj_t obj_data_6 = {-20, -20.0, -20.0};
    list_push_front(list_pointer, &obj_data_6);

    assert(list_size(list_pointer) == 6);

    array_val = -3;
    array_data = array_create(TYPE_INT, 3);
    *(int *)array_at(array_data, 0) = array_val++;
    *(int *)array_at(array_data, 1) = array_val++;
    *(int *)array_at(array_data, 2) = array_val;
    list_push_front(list_array, array_data);
    array_data = array_create(TYPE_INT, 3);
    array_val = -6;
    *(int *)array_at(array_data, 0) = array_val++;
    *(int *)array_at(array_data, 1) = array_val++;
    *(int *)array_at(array_data, 2) = array_val;
    list_push_front(list_array, array_data);
    array_data = array_create(TYPE_INT, 3);
    array_val = -9;
    *(int *)array_at(array_data, 0) = array_val++;
    *(int *)array_at(array_data, 1) = array_val++;
    *(int *)array_at(array_data, 2) = array_val;
    list_push_front(list_array, array_data);

    assert(list_size(list_array) == 6);    

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == i * 10 - 20);
    }

    for (int i = 0; i < list_size(list_float); i++)
    {
        assert(*(float *)list_at(list_float, i) == i * 10.0 - 20.0);
    }

    for (int i = 0; i < list_size(list_double); i++)
    {
        assert(*(double *)list_at(list_double, i) == i * 10.0 - 20.0);
    }

    for (int i = 0; i < list_size(list_char); i++)
    {
        assert(*(char *)list_at(list_char, i) == 'a' + i - 3);
    }

    assert(strcmp((char *)list_at(list_string, 0), "Testing") == 0);
    assert(strcmp((char *)list_at(list_string, 1), "push_front") == 0);
    assert(strcmp((char *)list_at(list_string, 2), "!") == 0);
    assert(strcmp((char *)list_at(list_string, 3), "Hello") == 0);
    assert(strcmp((char *)list_at(list_string, 4), "World") == 0);
    assert(strcmp((char *)list_at(list_string, 5), "!") == 0);

    for (int i = 0; i < list_size(list_pointer); i++)
    {
        test_obj_t val = *(test_obj_t *)list_at(list_pointer, i);
        assert(val.a == i * 10 - 20 && val.b == i * 10.0 - 20.0 && val.c == i * 10.0 - 20.0);
    }

    for (int i = 0; i < list_size(list_array); i++)
    {
        int array_val_0 = *(int *)array_at((array_t *)list_at(list_array, i), 0);
        int array_val_1 = *(int *)array_at((array_t *)list_at(list_array, i), 1);
        int array_val_2 = *(int *)array_at((array_t *)list_at(list_array, i), 2);
        assert(array_val_0 == i * 3 - 9 && array_val_1 == i * 3 - 8 && array_val_2 == i * 3 - 7);
    }

    ESP_LOGI("TEST2", "Popping data from back ...");

    list_pop_back(list_int);
    list_pop_back(list_int);
    list_pop_back(list_int);

    assert(list_size(list_int) == 3);

    list_pop_back(list_float);
    list_pop_back(list_float);
    list_pop_back(list_float);

    assert(list_size(list_float) == 3);

    list_pop_back(list_double);
    list_pop_back(list_double);
    list_pop_back(list_double);

    assert(list_size(list_double) == 3);

    list_pop_back(list_char);
    list_pop_back(list_char);
    list_pop_back(list_char);

    assert(list_size(list_char) == 3);

    list_pop_back(list_string);
    list_pop_back(list_string);
    list_pop_back(list_string);

    assert(list_size(list_string) == 3);

    list_pop_back(list_pointer);
    list_pop_back(list_pointer);
    list_pop_back(list_pointer);

    assert(list_size(list_pointer) == 3);

    array_t *arr_ptr = (array_t *)list_back(list_array);
    array_destroy(arr_ptr);
    list_pop_back(list_array);
    arr_ptr = (array_t *)list_back(list_array);
    array_destroy(arr_ptr);
    list_pop_back(list_array);
    arr_ptr = (array_t *)list_back(list_array);
    array_destroy(arr_ptr);
    list_pop_back(list_array);

    assert(list_size(list_array) == 3);

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == i * 10 - 20);
    }

    for (int i = 0; i < list_size(list_float); i++)
    {
        assert(*(float *)list_at(list_float, i) == i * 10.0 - 20.0);
    }

    for (int i = 0; i < list_size(list_double); i++)
    {
        assert(*(double *)list_at(list_double, i) == i * 10.0 - 20.0);
    }

    for (int i = 0; i < list_size(list_char); i++)
    {
        assert(*(char *)list_at(list_char, i) == 'a' + i - 3);
    }

    assert(strcmp((char *)list_at(list_string, 0), "Testing") == 0);
    assert(strcmp((char *)list_at(list_string, 1), "push_front") == 0);
    assert(strcmp((char *)list_at(list_string, 2), "!") == 0);

    for (int i = 0; i < list_size(list_pointer); i++)
    {
        test_obj_t val = *(test_obj_t *)list_at(list_pointer, i);
        assert(val.a == i * 10 - 20 && val.b == i * 10.0 - 20.0 && val.c == i * 10.0 - 20.0);
    }

    for (int i = 0; i < list_size(list_array); i++)
    {
        int array_val_0 = *(int *)array_at((array_t *)list_at(list_array, i), 0);
        int array_val_1 = *(int *)array_at((array_t *)list_at(list_array, i), 1);
        int array_val_2 = *(int *)array_at((array_t *)list_at(list_array, i), 2);
        assert(array_val_0 == i * 3 - 9 && array_val_1 == i * 3 - 8 && array_val_2 == i * 3 - 7);
    }

    ESP_LOGI("TEST2", "Popping data from front ...");

    list_pop_front(list_int);
    assert(list_size(list_int) == 2);
    assert(*(int *)list_at(list_int, 0) == -10);
    list_pop_front(list_int);
    assert(list_size(list_int) == 1);
    assert(*(int *)list_at(list_int, 0) == 0);
    list_pop_front(list_int);
    assert(list_size(list_int) == 0);

    list_pop_front(list_float);
    assert(list_size(list_float) == 2);
    assert(*(float *)list_at(list_float, 0) == -10.0);
    list_pop_front(list_float);
    assert(list_size(list_float) == 1);
    assert(*(float *)list_at(list_float, 0) == 0.0);
    list_pop_front(list_float);
    assert(list_size(list_float) == 0);

    list_pop_front(list_double);
    assert(list_size(list_double) == 2);
    assert(*(double *)list_at(list_double, 0) == -10.0);
    list_pop_front(list_double);
    assert(list_size(list_double) == 1);
    assert(*(double *)list_at(list_double, 0) == 0.0);
    list_pop_front(list_double);
    assert(list_size(list_double) == 0);

    list_pop_front(list_char);
    assert(list_size(list_char) == 2);
    assert(*(char *)list_at(list_char, 0) == 'a' - 2);
    list_pop_front(list_char);
    assert(list_size(list_char) == 1);
    assert(*(char *)list_at(list_char, 0) == 'a' - 1);
    list_pop_front(list_char);
    assert(list_size(list_char) == 0);

    list_pop_front(list_string);
    assert(list_size(list_string) == 2);
    assert(strcmp((char *)list_at(list_string, 0), "push_front") == 0);
    list_pop_front(list_string);
    assert(list_size(list_string) == 1);
    assert(strcmp((char *)list_at(list_string, 0), "!") == 0);
    list_pop_front(list_string);
    assert(list_size(list_string) == 0);

    list_pop_front(list_pointer);
    assert(list_size(list_pointer) == 2);
    test_obj_t val = *(test_obj_t *)list_at(list_pointer, 0);
    assert(val.a == -10 && val.b == -10.0 && val.c == -10.0);
    list_pop_front(list_pointer);
    assert(list_size(list_pointer) == 1);
    val = *(test_obj_t *)list_at(list_pointer, 0);
    assert(val.a == 0 && val.b == 0.0 && val.c == 0.0);
    list_pop_front(list_pointer);
    assert(list_size(list_pointer) == 0);

    arr_ptr = (array_t *)list_front(list_array);
    array_destroy(arr_ptr);
    list_pop_front(list_array);
    assert(list_size(list_array) == 2);
    assert(*(int *)array_at((array_t *)list_at(list_array, 0), 0) == -6);
    arr_ptr = (array_t *)list_front(list_array);
    array_destroy(arr_ptr);
    list_pop_front(list_array);
    assert(list_size(list_array) == 1);
    assert(*(int *)array_at((array_t *)list_at(list_array, 0), 0) == -3);
    arr_ptr = (array_t *)list_front(list_array);
    array_destroy(arr_ptr);
    list_pop_front(list_array);
    assert(list_size(list_array) == 0);

    ESP_LOGI("TEST2", "Destroying lists ...");
    list_destroy(list_int);
    list_destroy(list_float);
    list_destroy(list_double);
    list_destroy(list_char);
    list_destroy(list_string);
    list_destroy(list_pointer);
    list_destroy(list_array);

    assert(esp_get_free_heap_size() == heap_size);
}

// Covers: times push and access
void test_list_3()
{
    list_t *list_int = list_create(TYPE_INT);

    uint32_t start_time = get_time_ms();
    for (int i = 0; i < 10000; i++)
    {
        list_push_back(list_int, &i);
    }
    uint32_t end_time = get_time_ms();
    ESP_LOGI("TEST3", "Average push time: %f ms", (float)(end_time - start_time) / 10000);

    start_time = get_time_ms();
    for (int i = 0; i < 10000; i++)
    {
        int val = *(int *)list_at(list_int, i);
    }
    end_time = get_time_ms();
    ESP_LOGI("TEST3", "Average access time: %f ms", (float)(end_time - start_time) / 10000);

    start_time = get_time_ms();

    for (list_iterator_t it = list_begin(list_int); !list_iterator_equal(it, list_end(list_int)); it = list_iterator_next(it))
    {
        int val = *(int *)list_iterator_value(it);
    }
    end_time = get_time_ms();
    ESP_LOGI("TEST3", "Average iterator access time: %f ms", (float)(end_time - start_time) / 10000);

    for (list_iterator_t it = list_rbegin(list_int); !list_iterator_equal(it, list_rend(list_int)); it = list_iterator_next(it))
    {
        int val = *(int *)list_iterator_value(it);
    }
    end_time = get_time_ms();
    ESP_LOGI("TEST3", "Average reverse iterator access time: %f ms", (float)(end_time - start_time) / 10000);

    list_destroy(list_int);
}

// Covers: insert_range, append_range, prepend_range, resizse, erase, clear
void test_list_4() {
    list_t *list_int = list_create(TYPE_INT);

    int data[] = {1, 2, 3, 4, 5};

    ESP_LOGI("TEST4", "Appending data ...");
    list_append_range(list_int, data, 5);

    assert(list_size(list_int) == 5);

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == i + 1);
    }

    int data2[] = {6, 7, 8, 9, 10};

    ESP_LOGI("TEST4", "Prepending data ...");
    list_prepend_range(list_int, data2, 5);

    assert(list_size(list_int) == 10);

    for (int i = 0; i < list_size(list_int); i++)
    {
        if (i < 5)
        {
            assert(*(int *)list_at(list_int, i) == i + 6);
        }
        else
        {
            assert(*(int *)list_at(list_int, i) == i - 4);
        }
    }

    int val = 100;

    ESP_LOGI("TEST4", "Resizing list ...");
    list_resize(list_int, 5, &val);

    assert(list_size(list_int) == 5);

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == i + 6);
    }

    ESP_LOGI("TEST4", "Resizing list ...");
    list_resize(list_int, 20, &val);

    for (int i = 0; i < list_size(list_int); i++)
    {
        if (i < 5)
        {
            assert(*(int *)list_at(list_int, i) == i + 6);
        }
        else
        {
            assert(*(int *)list_at(list_int, i) == 100);
        }
    }

    int data3[] = {11, 12, 13, 14, 15};
    list_iterator_t it = list_begin(list_int);
    for (int i = 0; i < 8; ++i)
    {
        it = list_iterator_next(it);
    }

    ESP_LOGI("TEST4", "Inserting data ...");
    list_insert_range(list_int, it, data3, 5); // This invalidates our iterator

    assert(list_size(list_int) == 25);

    for (int i = 0; i < list_size(list_int); i++)
    {
        if (i < 5)
        {
            assert(*(int *)list_at(list_int, i) == i + 6);
        }
        else if (i < 8)
        {
            assert(*(int *)list_at(list_int, i) == 100);
        }
        else if (i < 13)
        {
            assert(*(int *)list_at(list_int, i) == i + 3);
        }
        else
        {
            assert(*(int *)list_at(list_int, i) == 100);
        }
    }

    it = list_begin(list_int);
    for (int i = 0; i < 8; ++i)
    {
        it = list_iterator_next(it);
    }

    ESP_LOGI("TEST4", "Erasing data ...");
    list_erase(list_int, list_begin(list_int), it);
    assert(list_size(list_int) == 17);

    for (int i = 0; i < list_size(list_int); i++)
    {
        if (i < 5)
        {
            assert(*(int *)list_at(list_int, i) == i + 11);
        }
        else
        {
            assert(*(int *)list_at(list_int, i) == 100);
        }
    }

    ESP_LOGI("TEST4", "Clearing list ...");
    list_clear(list_int);

    assert(list_size(list_int) == 0);

    list_destroy(list_int);
}

int predicate(const void *data) {
    return *(int *)data < 5;
}

// Covers: append_range, remove, remove_if
void list_test_5() {
    list_t *list_int = list_create(TYPE_INT);

    int data[] = {1, 3, 3, 4, 5};

    ESP_LOGI("TEST5", "Appending data ...");
    list_append_range(list_int, data, 5);

    assert(list_size(list_int) == 5);

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == data[i]);
    }

    ESP_LOGI("TEST5", "Removing data ...");
    int val = 3;
    list_remove(list_int, &val);

    assert(list_size(list_int) == 3);
    assert(*(int *)list_at(list_int, 0) == 1);
    assert(*(int *)list_at(list_int, 1) == 4);
    assert(*(int *)list_at(list_int, 2) == 5);

    ESP_LOGI("TEST5", "Removing data ...");
    list_remove_if(list_int, predicate);

    assert(list_size(list_int) == 1);
    assert(*(int *)list_at(list_int, 0) == 5);

    list_destroy(list_int);
}

int compare_less(const void *a, const void *b) {
    if (*(int *)a < *(int *)b)
    {
        return -1;
    }
    else if (*(int *)a > *(int *)b)
    {
        return 1;
    }
    return 0;
}

int compare_greater(const void *a, const void *b) {
    if (*(int *)a > *(int *)b)
    {
        return -1;
    }
    else if (*(int *)a < *(int *)b)
    {
        return 1;
    }
    return 0;
}

// Covers: sort, reverse, unique
void list_test_6() {
    list_t *list_int = list_create(TYPE_INT);

    int data[] = {5, 3, 4, 1, 2, 5, 3, 4, 1, 2};

    ESP_LOGI("TEST6", "Appending data ...");
    list_append_range(list_int, data, 10);

    assert(list_size(list_int) == 10);

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == data[i]);
    }

    ESP_LOGI("TEST6", "Sorting data ...");
    list_sort(list_int, compare_less);

    assert(list_size(list_int) == 10);

    for (int i = 0; i < list_size(list_int) - 1; i++)
    {
        assert(*(int *)list_at(list_int, i) <= *(int *)list_at(list_int, i + 1));
    }

    ESP_LOGI("TEST6", "Reversing data ...");
    list_reverse(list_int);

    assert(list_size(list_int) == 10);

    for (int i = 0; i < list_size(list_int) - 1; i++)
    {
        assert(*(int *)list_at(list_int, i) >= *(int *)list_at(list_int, i + 1));
    }

    ESP_LOGI("TEST6", "Appending data ...");
    list_append_range(list_int, data, 10);

    assert(list_size(list_int) == 20);

    for (int i = 10; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == data[i - 10]);
    }

    ESP_LOGI("TEST6", "Sorting data ...");
    list_sort(list_int, compare_greater);

    assert(list_size(list_int) == 20);

    for (int i = 0; i < list_size(list_int) - 1; i++)
    {
        assert(*(int *)list_at(list_int, i) >= *(int *)list_at(list_int, i + 1));
    }

    ESP_LOGI("TEST6", "Unique data ...");
    list_unique(list_int);

    assert(list_size(list_int) == 5);

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == 5 - i);
    }

    list_destroy(list_int);
}

// Covers: merge, splice, and swap
void list_test_7() {
    list_t *list_int = list_create(TYPE_INT);
    list_t *list_int2 = list_create(TYPE_INT);

    int data[] = {1, 2, 3, 4, 5};
    int data2[] = {6, 7, 8, 9, 10};

    ESP_LOGI("TEST7", "Appending data ...");
    list_append_range(list_int, data, 5);

    assert(list_size(list_int) == 5);

    list_append_range(list_int2, data2, 5);

    assert(list_size(list_int2) == 5);

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == i + 1);
    }

    for (int i = 0; i < list_size(list_int2); i++)
    {
        assert(*(int *)list_at(list_int2, i) == i + 6);
    }

    ESP_LOGI("TEST7", "Merging data ...");
    list_merge(list_int, list_int2);

    assert(list_size(list_int) == 10);
    assert(list_size(list_int2) == 0);

    for (int i = 0; i < list_size(list_int); i++)
    {
        assert(*(int *)list_at(list_int, i) == i + 1);
    }

    ESP_LOGI("TEST7", "Appending data ...");
    list_append_range(list_int2, data2, 5);

    assert(list_size(list_int2) == 5);

    for (int i = 0; i < list_size(list_int2); i++)
    {
        assert(*(int *)list_at(list_int2, i) == i + 6);
    }

    list_iterator_t it = list_begin(list_int);
    assert(*(int *)list_iterator_value(it) == 1);

    list_iterator_t it2 = list_begin(list_int2);
    assert(*(int *)list_iterator_value(it2) == 6);

    it = list_iterator_advance(it, 3);
    assert(*(int *)list_iterator_value(it) == 4);
    it2 = list_iterator_advance(it2, 3);
    assert(*(int *)list_iterator_value(it2) == 9);

    ESP_LOGI("TEST7", "Splicing data ...");
    list_splice(list_int, it, list_int2, it2, list_end(list_int2));
    
    assert(list_size(list_int) == 12);
    assert(list_size(list_int2) == 3);

    for (int i = 0; i < list_size(list_int); i++)
    {
        if (i < 3)
        {
            assert(*(int *)list_at(list_int, i) == i + 1);
        }
        else if (i < 5)
        {
            assert(*(int *)list_at(list_int, i) == i + 6);
        }
        else
        {
            assert(*(int *)list_at(list_int, i) == i - 1);
        }
    }

    assert(*(int *)list_at(list_int2, 0) == 6);
    assert(*(int *)list_at(list_int2, 1) == 7);
    assert(*(int *)list_at(list_int2, 2) == 8);

    ESP_LOGI("TEST7", "Swapping data ...");
    list_swap(list_int, list_int2);

    assert(list_size(list_int) == 3);
    assert(list_size(list_int2) == 12);

    assert(*(int *)list_at(list_int, 0) == 6);
    assert(*(int *)list_at(list_int, 1) == 7);
    assert(*(int *)list_at(list_int, 2) == 8);

    for (int i = 0; i < list_size(list_int2); i++)
    {
        if (i < 3)
        {
            assert(*(int *)list_at(list_int2, i) == i + 1);
        }
        else if (i < 5)
        {
            assert(*(int *)list_at(list_int2, i) == i + 6);
        }
        else
        {
            assert(*(int *)list_at(list_int2, i) == i - 1);
        }
    }

    list_destroy(list_int);
    list_destroy(list_int2);
}

void app_main(void)
{
    ESP_LOGI("list", "Start list test");
    ESP_LOGI("list", "Test 1");
    list_test_1();
    ESP_LOGI("list", "Test 2");
    list_test_2();
    ESP_LOGI("list", "Test 4");
    test_list_4();
    ESP_LOGI("list", "Test 5");
    list_test_5();
    ESP_LOGI("list", "Test 6");
    list_test_6();
    ESP_LOGI("list", "Test 7");
    list_test_7();
    ESP_LOGI("list", "Timed Test");
    test_list_3();
}
