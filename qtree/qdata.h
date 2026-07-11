#pragma once
#include <inttypes.h>

typedef struct node_t {
    struct node_t *left;
    struct node_t *right;
    struct node_t *parent;
    int color;

    uint64_t sum;
    uint64_t count;
    double key;
    uint64_t data;
} node_t;
