#pragma once
#include "rbtree.h"


typedef struct hashtable_t {
    tree_t *entries;
    uint64_t count;
    uint64_t allocated;
    uint64_t hash_collisions;
    //uint64_t divide_collisions;
    uint64_t (*hash_func)(const char *key);
} hashtable_t;


hashtable_t *hashtable_new(uint64_t supposed_size, uint64_t (*hash_func)(const char *key));
void hashtable_free(hashtable_t *ht);
node_t *hashtable_insert_auto(hashtable_t* ht, char *key, void *data);
uint64_t fnv(const char* key);
node_t *hashtable_get_auto(hashtable_t* ht, char *key);
void hashtable_foreach(hashtable_t *ht, void (*callback)(node_t*, void*, void*, void*), void *arg);
