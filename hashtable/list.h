#pragma once

typedef struct list_node {
    void *arg1;
    void *arg2;
    struct list_node *next;
} list_node;

typedef struct list_t {
    struct list_node *head;
} list_t;

void list_foreach_node(list_t *list, void (*callback)(list_node*, void*), void* arg1);
void list_free(list_t *list);
void list_add(list_t *list, void *arg1, void *arg2);
list_t* list_new();
