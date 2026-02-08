#pragma once
#include <stdint.h>
#include "data.h"

typedef struct tree_t {
    node_t *root;
    uint64_t count;
} tree_t;

node_t *tree_insert(tree_t *tree, char* key, uint64_t hash_sum, void* value);
void tree_foreach_node(node_t *node, void (*callback)(node_t*, void*, void*, void*), void *arg, void *arg2, void* arg3);
node_t* rb_delete(tree_t *tree, node_t *node);
node_t* tree_get(tree_t *tree, char *key);
void tree_print(tree_t *tree);
