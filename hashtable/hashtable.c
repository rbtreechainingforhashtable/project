#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "rbtree.h"
#include "list.h"
#include "hashtable.h"

hashtable_t *hashtable_new(uint64_t supposed_size, uint64_t (*hash_func)(const char *key)) {
    hashtable_t *ht = calloc(1, sizeof(*ht));
    ht->entries = calloc(1, sizeof(tree_t)*supposed_size);
    ht->allocated = supposed_size;
    ht->hash_func = hash_func;

    return ht;
}

void hashtable_fixup_tree_nodes(node_t *node, void *arg, void *arg2, void *arg3) {
    hashtable_t* ht = arg;
    uint64_t *position = arg2;
    uint64_t right_place = node->sum % ht->allocated;
    list_t* delete_buffer = arg3;
    if (right_place != *position) {
        list_add(delete_buffer, &ht->entries[*position], node);
        tree_insert(&ht->entries[right_place], node->key, node->sum, node->data);
    }
}

void emptying_moved_node(list_node *lnode, void *arg) {
    node_t *node = (node_t*)lnode->arg2;
    tree_t *tree = (tree_t*)lnode->arg1;
    hashtable_t* ht = arg;
    node = rb_delete(tree, node);
    if (node)
        --ht->count;
    free(node);
}

void hashtable_fixup(hashtable_t* ht, uint64_t old_size) {
    list_t *delete_buffer = list_new();

    for (uint64_t i = 0; i < old_size; ++i) {
        tree_t *cur = &ht->entries[i];
        tree_foreach_node(cur->root, hashtable_fixup_tree_nodes, ht, &i, delete_buffer);
    }

    list_foreach_node(delete_buffer, emptying_moved_node, ht);
    list_free(delete_buffer);
}

hashtable_t *hashtable_resize(hashtable_t* ht, uint64_t new_size) {
    if (!new_size)
        new_size = ht->allocated * 2;

    tree_t *new = calloc(1, sizeof(tree_t)*new_size);

    memcpy(new, ht->entries, sizeof(tree_t)*ht->allocated);
    tree_t *old = ht->entries;
    uint64_t old_size = ht->allocated;
    ht->entries = new;
    ht->allocated = new_size;
    free(old);

    hashtable_fixup(ht, old_size);

    return ht;
}

node_t *hashtable_insert_hash(hashtable_t* ht, char *key, uint64_t hash_sum, void *data) {
    uint64_t position = hash_sum % ht->allocated;
    tree_t *node = &ht->entries[position];
    node_t *ret = tree_insert(node, key, hash_sum, data);
    if (ret)
        ++ht->count;
    return ret;
}

node_t *hashtable_insert_auto(hashtable_t* ht, char *key, void *data) {
    uint64_t hash_sum = ht->hash_func(key);
    return hashtable_insert_hash(ht, key, hash_sum, data);
}

node_t *hashtable_get_hash(hashtable_t* ht, char *key, uint64_t hash_sum) {
    if (!ht)
        return NULL;

    uint64_t position = hash_sum % ht->allocated;
    tree_t *node = &ht->entries[position];
    node_t *branch = tree_get(node, key);
    if (!branch) {
        return NULL;
    }

    return branch;
}

node_t *hashtable_get_auto(hashtable_t* ht, char *key) {
    uint64_t hash_sum = ht->hash_func(key);
    return hashtable_get_hash(ht, key, hash_sum);
}

void print_node(node_t *node, void *arg, void *arg2, void *arg3) {
    uint64_t *position = arg2;
    hashtable_t *ht = arg;
    printf("[%p] '%s'[%llu->%llu]: '%s'\n", node, node->key, node->sum, *position, (char*)node->data);
}

void hashtable_foreach(hashtable_t *ht, void (*callback)(node_t*, void*, void*, void*), void *arg) {
    uint64_t max = ht->allocated;
    for (uint64_t i = 0; i < max; ++i) {
        tree_t *node = &ht->entries[i];
        if (node->count)
            tree_foreach_node(node->root, callback, ht, &i, arg);
    }
}

void hashtable_free(hashtable_t *ht) {
    uint64_t max = ht->allocated;
    list_t *delete_buffer = list_new();


    for (uint64_t i = 0; i < max; ++i) {
        tree_t *node = &ht->entries[i];
        if (node->count)
            if (node->root)
                rb_delete(node, node->root);
    }

    free(ht->entries);
    free(ht);
}

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
uint64_t fnv(const char* key) {
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

int __main() {
    hashtable_t *ht = hashtable_new(128, fnv);
    printf("ht->entries %p, size %llu\n", ht->entries, ht->allocated);
    for (uint64_t i = 0; i < 1024; ++i) {
        char newkey[48];
        snprintf(newkey, 47, "%s%llu", "mycity", i);
        node_t *data = hashtable_get_auto(ht, newkey);
        if (!data)
        {
            char *passkey = strdup(newkey);
            char *passdata = strdup("mydata");
            node_t *ret = hashtable_insert_auto(ht, passkey, passdata);
            printf("hashtable_insert_auto %p\n", ret);
            if (!ret) {
                printf("error inserting data %s\n", passkey);
                free(passkey);
                free(passdata);
            }
        }
    }
    hashtable_foreach(ht, print_node, NULL);
    hashtable_resize(ht, 1024);
    hashtable_foreach(ht, print_node, NULL);
    printf("ht->entries %p, size %llu\n", ht->entries, ht->allocated);
    hashtable_free(ht);
    return 0;
}
