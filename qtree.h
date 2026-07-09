#pragma once
#include <stdint.h>
#include "qdata.h"

typedef struct tree_t {
    node_t *root;
    uint64_t count;
} tree_t;

node_t *tree_insert(tree_t *tree, double key, uint64_t hash_sum, uint64_t value, uint8_t tree_height);
void tree_foreach_node(node_t *node, void (*callback)(node_t*, void*, void*, void*), void *arg, void *arg2, void* arg3);
node_t* rb_delete(tree_t *tree, node_t *node);
node_t* tree_get(tree_t *tree, double key);
void tree_print(tree_t *tree);
node_t* tree_quantile(tree_t *tree, double quantile);
