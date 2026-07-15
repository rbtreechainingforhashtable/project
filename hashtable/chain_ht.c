#include "chain_ht.h"

#include <stdlib.h>
#include <string.h>

#include "rbtree.h"
#include "data.h"

typedef struct chain_node {
	char *key;
	void *data;
	struct chain_node *next;
} chain_node_t;

typedef struct bucket_t {
	uint8_t as_tree;
	uint64_t count;
	union {
		chain_node_t *head;
		tree_t tree;
	} u;
} bucket_t;

struct chain_ht_t {
	bucket_t *buckets;
	uint64_t count;
	uint64_t allocated;
	uint64_t (*hash_func)(const char *key);
	chain_mode_t mode;
	uint8_t treeify_threshold;
	uint64_t lookup_comparisons;
	uint64_t max_bucket_size;
};

uint64_t chain_ht_lookup_comparisons(const chain_ht_t *ht) {
	return ht ? ht->lookup_comparisons : 0;
}

uint64_t chain_ht_max_bucket_size(const chain_ht_t *ht) {
	return ht ? ht->max_bucket_size : 0;
}

uint64_t chain_ht_count(const chain_ht_t *ht) {
	return ht ? ht->count : 0;
}

static uint64_t tree_node_heap_bytes(const node_t *node) {
	uint64_t bytes;

	if (!node)
		return 0;

	bytes = sizeof(*node);
	if (node->key)
		bytes += strlen(node->key) + 1;

	return bytes + tree_node_heap_bytes(node->left) + tree_node_heap_bytes(node->right);
}

static uint64_t bucket_heap_bytes(const bucket_t *bucket) {
	uint64_t bytes = 0;

	if (!bucket || !bucket->count)
		return 0;

	if (bucket->as_tree)
		return tree_node_heap_bytes(bucket->u.tree.root);

	for (chain_node_t *node = bucket->u.head; node; node = node->next) {
		bytes += sizeof(*node);
		if (node->key)
			bytes += strlen(node->key) + 1;
	}

	return bytes;
}

uint64_t chain_ht_heap_bytes(const chain_ht_t *ht) {
	uint64_t bytes;
	uint64_t i;

	if (!ht)
		return 0;

	bytes = sizeof(*ht) + ht->allocated * sizeof(*ht->buckets);
	for (i = 0; i < ht->allocated; ++i)
		bytes += bucket_heap_bytes(&ht->buckets[i]);

	return bytes;
}

static void bucket_treeify(bucket_t *bucket, uint64_t (*hash_func)(const char *key)) {
	chain_node_t *node;
	tree_t tree = {0};

	if (bucket->as_tree)
		return;

	for (node = bucket->u.head; node; ) {
		chain_node_t *next = node->next;
		if (!tree_get(&tree, node->key))
			tree_insert(&tree, node->key, hash_func(node->key), node->data);
		free(node);
		node = next;
	}

	bucket->u.tree = tree;
	bucket->as_tree = 1;
}

static int bucket_insert_list(bucket_t *bucket, char *key, void *data) {
	chain_node_t *node;

	for (node = bucket->u.head; node; node = node->next) {
		if (!strcmp(node->key, key))
			return 2;
	}

	node = malloc(sizeof(*node));

	if (!node)
		return 0;

	node->key = key;
	node->data = data;
	node->next = bucket->u.head;
	bucket->u.head = node;
	++bucket->count;
	return 1;
}

static void bucket_free_nodes(bucket_t *bucket) {
	if (bucket->as_tree) {
		if (bucket->u.tree.root)
			rb_delete(&bucket->u.tree, bucket->u.tree.root);
		return;
	}

	chain_node_t *node = bucket->u.head;
	while (node) {
		chain_node_t *next = node->next;
		free(node);
		node = next;
	}
}

chain_ht_t *chain_ht_new(uint64_t size, uint64_t (*hash_func)(const char *key),
                         chain_mode_t mode, uint8_t treeify_threshold) {
	chain_ht_t *ht = calloc(1, sizeof(*ht));

	if (!ht)
		return NULL;

	ht->buckets = calloc(size, sizeof(*ht->buckets));
	if (!ht->buckets) {
		free(ht);
		return NULL;
	}

	ht->allocated = size;
	ht->hash_func = hash_func;
	ht->mode = mode;
	ht->treeify_threshold = treeify_threshold;
	return ht;
}

void chain_ht_free(chain_ht_t *ht) {
	uint64_t i;

	if (!ht)
		return;

	for (i = 0; i < ht->allocated; ++i)
		bucket_free_nodes(&ht->buckets[i]);

	free(ht->buckets);
	free(ht);
}

static int bucket_insert(chain_ht_t *ht, bucket_t *bucket, char *key, void *data,
                         uint64_t hash_sum) {
	int rc;

	if (ht->mode == CHAIN_MODE_TREE) {
		if (tree_insert(&bucket->u.tree, key, hash_sum, data))
			return 1;
		return 0;
	}

	rc = bucket_insert_list(bucket, key, data);
	if (rc != 1)
		return rc;

	return 1;
}

void chain_ht_finalize(chain_ht_t *ht) {
	uint64_t i;

	if (!ht || ht->mode != CHAIN_MODE_HYBRID)
		return;

	for (i = 0; i < ht->allocated; ++i) {
		bucket_t *bucket = &ht->buckets[i];
		if (!bucket->as_tree && bucket->count >= ht->treeify_threshold)
			bucket_treeify(bucket, ht->hash_func);
	}
}

int chain_ht_insert(chain_ht_t *ht, char *key, void *data) {
	uint64_t hash_sum = ht->hash_func(key);
	uint64_t position = hash_sum % ht->allocated;
	bucket_t *bucket = &ht->buckets[position];
	int inserted;

	if (bucket->as_tree || ht->mode == CHAIN_MODE_TREE) {
		bucket->as_tree = 1;
		if (tree_get(&bucket->u.tree, key))
			return 1;
		if (!tree_insert(&bucket->u.tree, key, hash_sum, data))
			return 0;
		++bucket->count;
	} else {
		inserted = bucket_insert(ht, bucket, key, data, hash_sum);
		if (inserted == 2)
			return 1;
		if (!inserted)
			return 0;
	}

	++ht->count;
	if (bucket->count > ht->max_bucket_size)
		ht->max_bucket_size = bucket->count;

	return 1;
}

void chain_ht_reset_stats(chain_ht_t *ht) {
	ht->lookup_comparisons = 0;
}

void *chain_ht_get(chain_ht_t *ht, const char *key) {
	uint64_t hash_sum = ht->hash_func(key);
	uint64_t position = hash_sum % ht->allocated;
	bucket_t *bucket = &ht->buckets[position];

	if (bucket->as_tree || ht->mode == CHAIN_MODE_TREE) {
		node_t *node = bucket->u.tree.root;

		while (node) {
			++ht->lookup_comparisons;
			int rc = strcmp(key, node->key);
			if (rc > 0)
				node = node->right;
			else if (rc < 0)
				node = node->left;
			else
				return node->data;
		}
		return NULL;
	}

	for (chain_node_t *node = bucket->u.head; node; node = node->next) {
		++ht->lookup_comparisons;
		if (!strcmp(node->key, key))
			return node->data;
	}

	return NULL;
}
