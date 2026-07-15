#pragma once

#include <stdint.h>

typedef struct chain_ht_t chain_ht_t;

typedef enum {
	CHAIN_MODE_LIST = 0,
	CHAIN_MODE_TREE = 1,
	CHAIN_MODE_HYBRID = 2,
} chain_mode_t;

chain_ht_t *chain_ht_new(uint64_t size, uint64_t (*hash_func)(const char *key),
                         chain_mode_t mode, uint8_t treeify_threshold);
void chain_ht_free(chain_ht_t *ht);
int chain_ht_insert(chain_ht_t *ht, char *key, void *data);
void *chain_ht_get(chain_ht_t *ht, const char *key);
void chain_ht_reset_stats(chain_ht_t *ht);
uint64_t chain_ht_lookup_comparisons(const chain_ht_t *ht);
uint64_t chain_ht_max_bucket_size(const chain_ht_t *ht);
uint64_t chain_ht_count(const chain_ht_t *ht);
void chain_ht_finalize(chain_ht_t *ht);
uint64_t chain_ht_heap_bytes(const chain_ht_t *ht);
