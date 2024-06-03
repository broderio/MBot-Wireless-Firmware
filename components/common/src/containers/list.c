#include <string.h>
#include <stdlib.h>

#include "esp_log.h"

#include "common.h"
#include "containers/array.h"

#include "containers/list.h"

typedef struct list_node_t list_node_t;

#pragma pack(1)
struct list_t
{
    list_node_t *start;
    list_node_t *end;
    type_t type;
    int type_size;
    int size;
};
#pragma pack()

#pragma pack(1)
struct list_node_t
{
    list_node_t *next;
    list_node_t *prev;
    void *value;
};
#pragma pack()

list_iterator_t _list_create_iterator(list_t *list, list_node_t *node, int dir)
{
    list_iterator_t it = {
        .list = list,
        .node = node,
        .dir = dir};
    return it;
}

list_node_t *_list_alloc_node(list_t *list, void *data)
{
    list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t));
    if (node == NULL)
    {
        return NULL;
    }
    node->next = NULL;
    node->prev = NULL;

    if (list->type != TYPE_STRING && list->type != TYPE_POINTER)
    {
        node->value = (void *)malloc(list->type_size);
        if (node->value == NULL)
        {
            free(node);
            return NULL;
        }
        memcpy(node->value, data, list->type_size);
    }
    else if (list->type == TYPE_STRING)
    {
        int size = strlen((char *)data);
        node->value = (void *)malloc(size + 1);
        if (node->value == NULL)
        {
            free(node);
            return NULL;
        }
        strcpy(node->value, data);
    }
    else
    {
        node->value = data;
    }
    return node;
}

list_node_t *_list_comp_merge(list_node_t *a, list_node_t *b, compare_func_t compare)
{
    if (a == NULL)
        return b;
    if (b == NULL)
        return a;

    if (compare(a->value, b->value) <= 0)
    {
        a->next = _list_comp_merge(a->next, b, compare);
        if (a->next != NULL)
            a->next->prev = a;
        return a;
    }
    else
    {
        b->next = _list_comp_merge(a, b->next, compare);
        if (b->next != NULL)
            b->next->prev = b;
        return b;
    }
}

list_node_t *_list_merge_sort(list_node_t *head, compare_func_t compare)
{
    if (head == NULL || head->next == NULL)
        return head;

    list_node_t *slow = head;
    list_node_t *fast = head->next;

    while (fast != NULL && fast->next != NULL)
    {
        slow = slow->next;
        fast = fast->next->next;
    }

    list_node_t *temp = slow->next;
    slow->next = NULL;
    if (temp != NULL)
        temp->prev = NULL;

    list_node_t *left = _list_merge_sort(head, compare);
    list_node_t *right = _list_merge_sort(temp, compare);

    return _list_comp_merge(left, right, compare);
}

list_t *list_create(type_t type)
{
    list_t *list = (list_t *)malloc(sizeof(list_t));
    if (list == NULL)
    {
        return NULL;
    }

    list->start = NULL;
    list->end = NULL;
    list->type = type;
    list->size = 0;

    list->type_size = TYPE_SIZE(type);

    return list;
}

void list_destroy(list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    list_node_t *node = list->start;
    if (node == NULL)
    {
        free(list);
        return;
    }

    while (node != NULL)
    {
        list_node_t *next = node->next;
        if (list->type != TYPE_POINTER)
        {
            free(node->value);
        }
        free(node);
        node = next;
    }
    free(list);
    return;
}

type_t list_type(list_t *list)
{
    if (list == NULL)
    {
        return TYPE_ERROR;
    }
    return list->type;
}

int list_type_size(list_t *list)
{
    if (list == NULL)
    {
        return -1;
    }
    return list->type_size;
}

void *list_front(list_t *list)
{
    if (list == NULL || list->size == 0)
    {
        return NULL;
    }

    return list->start->value;
}

void *list_back(list_t *list)
{
    if (list == NULL || list->size == 0)
    {
        return NULL;
    }

    return list->end->value;
}

void *list_at(list_t *list, int index)
{
    if (list == NULL)
    {
        return NULL;
    }
    if (index < 0 || index >= list->size)
    {
        return NULL;
    }
    if (index == 0)
    {
        return list->start->value;
    }
    if (index == list->size - 1)
    {
        return list->end->value;
    }

    list_node_t *node;
    if (index <= list->size / 2)
    {
        node = list->start;
        for (int i = 0; i < index; i++)
        {
            node = node->next;
        }
    }
    else
    {
        node = list->end;
        for (int i = list->size - 1; i > index; i--)
        {
            node = node->prev;
        }
    }
    return node->value;
}

list_iterator_t list_begin(list_t *list)
{
    return _list_create_iterator(list, list->start, 0);
}
list_iterator_t list_rbegin(list_t *list)
{
    return _list_create_iterator(list, list->end, 1);
}
list_iterator_t list_end(list_t *list)
{
    return _list_create_iterator(list, NULL, 0);
}
list_iterator_t list_rend(list_t *list)
{
    return _list_create_iterator(list, NULL, 1);
}

int list_empty(list_t *list)
{
    if (list == NULL)
    {
        return -1;
    }

    return list->size == 0;
}

int list_size(list_t *list)
{
    if (list == NULL)
    {
        return -1;
    }

    return list->size;
}

void list_clear(list_t *list)
{
    if (list == NULL || list->size == 0)
    {
        return;
    }

    list_node_t *node = list->start, *next;
    while (node != NULL)
    {
        if (list->type != TYPE_POINTER)
        {
            free(node->value);
        }
        next = node->next;
        free(node);
        node = next;
    }
    list->start = NULL;
    list->end = NULL;
    list->size = 0;
}

list_iterator_t list_insert(list_t *list, list_iterator_t pos, void *data)
{
    if (list == NULL)
    {
        return list_end(list);
    }

    list_node_t *node = _list_alloc_node(list, data);
    if (node == NULL)
    {
        return list_end(list);
    }

    if (list->size == 0)
    {
        list->start = node;
        list->end = node;
    }
    else if (list_iterator_equal(pos, list_begin(list)))
    {
        node->next = list->start;
        list->start->prev = node;
        list->start = node;
    }
    else if (list_iterator_equal(pos, list_end(list)))
    {
        node->prev = list->end;
        list->end->next = node;
        list->end = node;
    }
    else
    {
        list_node_t *next = pos.node;
        list_node_t *prev = next->prev;
        node->next = next;
        node->prev = prev;
        prev->next = node;
        next->prev = node;
    }

    list->size++;
    return _list_create_iterator(list, node, 0);
}

list_iterator_t list_insert_range(list_t *list, list_iterator_t pos, void *data, int n)
{
    if (list == NULL || n <= 0)
    {
        return list_end(list);
    }

    list_iterator_t it = list_insert(list, pos, data);
    if (list_iterator_equal(it, list_end(list)))
    {
        return it;
    }

    list_node_t *prev = it.node;
    for (int i = 1; i < n; i++)
    {
        list_node_t *node = _list_alloc_node(list, data + i * list->type_size);
        if (node == NULL)
        {
            return list_end(list);
        }
        node->prev = prev;
        node->next = prev->next;
        prev->next = node;
        if (node->next != NULL)
        {
            node->next->prev = node;
        }
        else
        {
            list->end = node;
        }
        list->size++;
        prev = node;
    }
    return it;
}

list_iterator_t list_erase(list_t *list, list_iterator_t start, list_iterator_t end)
{
    if (list == NULL || list->size == 0)
    {
        return list_end(list);
    }

    if (list_iterator_equal(start, end))
    {
        return start;
    }

    list_iterator_t it = start;
    while (!list_iterator_equal(it, end))
    {
        list_node_t *node = it.node;
        it = list_iterator_next(it);

        // If start is the first node
        if (node->prev == NULL)
        {
            list->start = node->next;
        }
        else
        {
            node->prev->next = node->next;
        }

        // If start is the last node
        if (node->next == NULL)
        {
            list->end = node->prev;
        }
        else
        {
            node->next->prev = node->prev;
        }

        if (list->type != TYPE_POINTER)
        {
            free(node->value);
        }
        free(node);

        list->size--;
    }
    return start;
}

void list_push_back(list_t *list, void *data)
{
    if (list == NULL)
    {
        return;
    }

    list_insert(list, list_end(list), data);
}

void list_append_range(list_t *list, void *data, int n)
{
    if (list == NULL || n <= 0)
    {
        return;
    }

    list_insert_range(list, list_end(list), data, n);
}

void list_pop_back(list_t *list)
{
    list_erase(list, list_iterator_prev(list_end(list)), list_end(list));
}

void list_push_front(list_t *list, void *data)
{
    list_insert(list, list_begin(list), data);
}

void list_prepend_range(list_t *list, void *data, int n)
{
    if (list == NULL || n <= 0)
    {
        return;
    }

    list_insert_range(list, list_begin(list), data, n);
}

void list_pop_front(list_t *list)
{
    list_erase(list, list_begin(list), list_iterator_next(list_begin(list)));
}

void list_resize(list_t *list, int n, void *data)
{
    if (n <= 0)
    {
        list_clear(list);
        return;
    }

    if (n < list->size)
    {
        list_erase(list, list_iterator_add(list_begin(list), n), list_end(list));
    }
    else
    {
        for (int i = list->size; i < n; i++)
        {
            list_push_back(list, data);
        }
    }
}

void list_swap(list_t *list, list_t *other)
{
    list_node_t *start = list->start;
    list_node_t *end = list->end;
    int size = list->size;

    list->start = other->start;
    list->end = other->end;
    list->size = other->size;

    other->start = start;
    other->end = end;
    other->size = size;
}

void list_reverse(list_t *list)
{
    list_node_t *node = list->start;
    list_node_t *temp;
    while (node != NULL)
    {
        temp = node->next;
        node->next = node->prev;
        node->prev = temp;
        node = temp;
    }
    temp = list->start;
    list->start = list->end;
    list->end = temp;
}

void list_merge(list_t *list1, list_t *list2)
{
    if (list1 == NULL || list2 == NULL)
    {
        return;
    }

    if (list1->type != list2->type)
    {
        return;
    }

    if (list1->size == 0)
    {
        list1->start = list2->start;
        list1->end = list2->end;
        list1->size = list2->size;
        list2->start = NULL;
        list2->end = NULL;
        list2->size = 0;
        return;
    }

    if (list2->size == 0)
    {
        return;
    }

    list1->end->next = list2->start;
    list2->start->prev = list1->end;
    list1->end = list2->end;
    list1->size += list2->size;
    list2->start = NULL;
    list2->end = NULL;
    list2->size = 0;
}

void list_splice(list_t *list, list_iterator_t pos, list_t *other, list_iterator_t start, list_iterator_t end)
{
    if (list == NULL || other == NULL || pos.list != list || start.list != other || end.list != other)
    {
        return;
    }

    if (start.node == end.node)
    {
        return;
    }

    size_t count = 0;
    for (list_node_t *node = start.node; node != end.node; node = node->next)
    {
        count++;
    }

    list_node_t *p = pos.node;
    list_node_t *s = start.node;
    list_node_t *e = end.node;
    list_node_t *e_prev = NULL;
    if (e == NULL)
    {
        e_prev = list_iterator_prev(end).node;
    }
    else {
        e_prev = e->prev;
    }

    // Remove [start, end) from other list
    if (s->prev == NULL)
    {
        other->start = e;
    }
    else
    {
        s->prev->next = e;
    }

    if (e == NULL)
    {
        other->end = s->prev;
    }
    else
    {
        e->prev = s->prev;
    }

    // Insert [start, end) into list
    if (p == NULL)
    {
        if (list->size == 0)
        {
            list->start = s;
            list->end = e_prev;
            list->size = count;
        }
        else
        {
            list->end->next = s;
            s->prev = list->end;
            list->end = e_prev;
            list->size += count;
        }
        return;
    }

    if (p->prev == NULL)
    {
        list->start = s;
    }
    else
    {
        p->prev->next = s;
    }
    s->prev = p->prev;
    e_prev->next = p;
    p->prev = e_prev;
    list->size += count;
    other->size -= count;
}

void list_remove(list_t *list, void *data)
{
    if (list == NULL)
    {
        return;
    }

    list_node_t *node = list->start;
    list_node_t *prev = NULL;
    list_node_t *next = NULL;

    int size;
    if (list->type == TYPE_STRING)
    {
        size = strlen((char *)data);
    }
    else
    {
        size = list->type_size;
    }

    while (node != NULL)
    {
        next = node->next;
        if (memcmp(node->value, data, size) == 0)
        {
            if (prev == NULL)
            {
                list->start = next;
            }
            else
            {
                prev->next = next;
            }
            if (next == NULL)
            {
                list->end = prev;
            }
            free(node->value);
            free(node);
            list->size--;
        }
        else
        {
            prev = node;
        }
        node = next;
    }
}

void list_remove_if(list_t *list, predicate_func_t predicate)
{
    if (list == NULL)
    {
        return;
    }

    list_node_t *node = list->start;
    list_node_t *prev = NULL;
    list_node_t *next = NULL;

    while (node != NULL)
    {
        next = node->next;
        if (predicate(node->value))
        {
            if (prev == NULL)
            {
                list->start = next;
            }
            else
            {
                prev->next = next;
            }
            if (next == NULL)
            {
                list->end = prev;
            }
            free(node->value);
            free(node);
            list->size--;
        }
        else
        {
            prev = node;
        }
        node = next;
    }
}

void list_unique(list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    list_node_t *node = list->start;

    while (node != NULL && node->next != NULL)
    {
        if (memcmp(node->value, node->next->value, list->type_size) == 0)
        {
            list_node_t *dup = node->next;
            node->next = node->next->next;
            if (node->next != NULL)
            {
                node->next->prev = node;
            }
            if (dup == list->end)
            {
                list->end = node;
            }
            free(dup->value);
            free(dup);
            list->size--;
        }
        else
        {
            node = node->next;
        }
    }
}

void list_sort(list_t *list, compare_func_t compare)
{
    if (list == NULL)
    {
        return;
    }

    list->start = _list_merge_sort(list->start, compare);

    list_node_t *node = list->start;
    while (node->next != NULL)
    {
        node->next->prev = node;
        node = node->next;
    }
    list->end = node;
}

list_iterator_t list_iterator_next(list_iterator_t it)
{
    if (it.node == NULL)
    {
        return it;
    }

    if (it.dir)
    {
        it.node = it.node->prev;
    }
    else
    {
        it.node = it.node->next;
    }

    return it;
}

list_iterator_t list_iterator_prev(list_iterator_t it)
{
    if (it.dir)
    {
        if (it.node == NULL)
        {
            it.node = it.list->start;
        }
        else
        {
            it.node = it.node->next;
        }
    }
    else
    {
        if (it.node == NULL)
        {
            it.node = it.list->end;
        }
        else
        {
            it.node = it.node->prev;
        }
    }
    return it;
}

list_iterator_t list_iterator_add(list_iterator_t it, int n)
{
    if (it.node == NULL || n == 0)
    {
        return it;
    }

    if (n < 0)
    {
        return list_iterator_sub(it, -n);
    }

    int i = 0;
    while (i < n && it.node != NULL)
    {
        it = list_iterator_next(it);
        i++;
    }
    return it;
}

list_iterator_t list_iterator_sub(list_iterator_t it, int n)
{
    if (it.node == NULL || n == 0)
    {
        return it;
    }

    if (n < 0)
    {
        return list_iterator_add(it, -n);
    }

    int i = 0;
    while (i < n && it.node != NULL)
    {
        it = list_iterator_prev(it);
        i++;
    }
    return it;
}

list_iterator_t list_iterator_advance(list_iterator_t iterator, int n)
{
    if (n == 0)
        return iterator;

    if (iterator.dir == 0)
    {
        return list_iterator_add(iterator, n);
    }
    else
    {
        return list_iterator_sub(iterator, n);
    }
}

void *list_iterator_value(list_iterator_t it)
{
    if (it.node == NULL)
    {
        return NULL;
    }

    return it.node->value;
}

int list_iterator_equal(list_iterator_t it, list_iterator_t other)
{
    if (it.dir == 1)
    {
        it = list_iterator_prev(it);
    }
    if (other.dir == 1)
    {
        other = list_iterator_prev(other);
    }
    return it.node == other.node;
}