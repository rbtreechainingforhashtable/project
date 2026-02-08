#include <stdlib.h>
#include "list.h"

list_t* list_new() {
    return calloc(1, sizeof(list_t));
}

void list_add(list_t *list, void *arg1, void *arg2) {
    list_node *new = malloc(sizeof(*new));

    new->arg1 = arg1;
    new->arg2 = arg2;
    new->next = list->head;

    list->head = new;
}

void list_free(list_t *list) {
    list_node *lnode = list->head;
    while (lnode) {
        list_node *next = lnode->next;
        free(lnode);
        lnode = next;
    }

    free(list);
}

void list_foreach_node(list_t *list, void (*callback)(list_node*, void*), void* arg1) {
    list_node *lnode = list->head;
    while (lnode) {
        list_node *next = lnode->next;
        callback(lnode, arg1);
        lnode = next;
    }
}
