#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common/timing.h"
#include "qtree/qtree.h"

uint64_t nsec_tree;

typedef struct list_node {
	void *arg1;
	void *arg2;
	struct list_node *next;
} list_node;

typedef struct list_t {
	struct list_node *head;
} list_t;

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


typedef struct hashtable_t {
	tree_t *entries;
	uint64_t count;
	uint8_t tree_height;
	double step;
	uint64_t allocated;
	uint64_t hash_collisions;
	uint64_t (*hash_func)(double, double);
} hashtable_t;

hashtable_t *hashtable_new(uint64_t supposed_size, uint64_t (*hash_func)(double, double)) {
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
		tree_insert(&ht->entries[right_place], node->key, node->sum, node->data, ht->tree_height);
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

node_t *hashtable_insert_hash(hashtable_t* ht, double key, uint64_t hash_sum, uint64_t data) {
	uint64_t position = hash_sum % ht->allocated;
	tree_t *node = &ht->entries[position];
	struct timespec start;
	struct timespec end;
	node_t *ret;

	clock_gettime(CLOCK_MONOTONIC, &start);
	ret = tree_insert(node, key, hash_sum, data, ht->tree_height);
	clock_gettime(CLOCK_MONOTONIC, &end);
	nsec_tree += timespec_elapsed_nsec(&start, &end);

	if (ret)
		++ht->count;
	return ret;
}

node_t *hashtable_insert_auto(hashtable_t* ht, double key, uint64_t data) {
	uint64_t hash_sum = ht->hash_func(key, ht->step);
	return hashtable_insert_hash(ht, key, hash_sum, data);
}

node_t *hashtable_get_hash(hashtable_t* ht, double key, uint64_t hash_sum) {
	if (!ht)
		return NULL;

	uint64_t position = hash_sum % ht->allocated;
	tree_t *node = &ht->entries[position];
	node_t *branch = tree_get(node, key);
	if (!branch)
		return NULL;

	return branch;
}

node_t *hashtable_get_auto(hashtable_t* ht, double key) {
	uint64_t hash_sum = ht->hash_func(key, ht->step);
	return hashtable_get_hash(ht, key, hash_sum);
}

void print_node(node_t *node, void *arg, void *arg2, void *arg3) {
	uint64_t *position = arg2;
	(void)arg;
	(void)arg3;
	printf("[%p] '%lf'[%llu->%llu]: '%llu'\n", node, node->key, node->sum, *position, node->data);
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

	for (uint64_t i = 0; i < max; ++i) {
		tree_t *node = &ht->entries[i];
		if (node->count)
			if (node->root)
				rb_delete(node, node->root);
	}

	free(ht->entries);
	free(ht);
}

uint64_t ordered_hash(double key, double step) {
	return (uint64_t)(key / step);
}

typedef struct true_t {
	double *arr;
	uint64_t size;
	uint64_t cur;
} true_t;

int compare_dbl(const void *a, const void *b) {
	double fa = *(const double*)a;
	double fb = *(const double*)b;
	return (fa > fb) - (fa < fb);
}

true_t *true_init(uint64_t size) {
	true_t *t = calloc(1, sizeof(*t));
	t->arr = calloc(size, sizeof(double));
	t->size = size;

	return t;
}

void true_push(true_t *t, double value, uint64_t count) {
	for (uint64_t i = t->cur, j = 0; j < count; ++i, ++j)
		t->arr[i] = value;
	t->cur += count;
}

void true_sort(true_t *t) {
	qsort(t->arr, t->cur, sizeof(double), compare_dbl);
}

double true_quantile(true_t *t, double quantile) {
	uint64_t index = (uint64_t)round(t->cur * quantile);
	return t->arr[index];
}

typedef struct quantiles_index_t {
	hashtable_t *ht;
	uint64_t count;
	double from;
	double to;
	double sum;
	int64_t highest;
	int64_t lowest;
} quantiles_index_t;

quantiles_index_t* quantiles_index_make(double from, double to, uint64_t size, uint8_t tree_height) {
	quantiles_index_t *qi = calloc(1, sizeof(*qi));
	qi->ht = hashtable_new(size, ordered_hash);
	qi->ht->step = fabs(to - from) / size;
	qi->from = from;
	qi->to = to;
	qi->lowest = -1;
	qi->ht->tree_height = tree_height;

	return qi;
}

void quantiles_insert(quantiles_index_t *qi, double data, uint64_t count) {
	uint64_t hash_index = qi->ht->hash_func(data, qi->ht->step);
	double cell = (hash_index + 1.00) / qi->ht->allocated;
	double addnumber = (cell * count);

	hashtable_insert_auto(qi->ht, data, count);
	qi->sum += addnumber;
	qi->count += count;

	if (qi->lowest == -1)
		qi->lowest = (int64_t)hash_index;
	if ((int64_t)hash_index > qi->highest)
		qi->highest = (int64_t)hash_index;
	if ((int64_t)hash_index < qi->lowest)
		qi->lowest = (int64_t)hash_index;
}

double quantile_index_dev(quantiles_index_t *qi) {
	return (qi->sum / qi->count);
}

double quantile_calculate(quantiles_index_t *qi, double quantile) {
	uint64_t cursize = (uint64_t)(qi->highest - qi->lowest);
	double qi_dev = quantile_index_dev(qi);
	uint64_t index = (uint64_t)round(qi_dev * 2 * quantile * cursize);
	index += (uint64_t)qi->lowest;
	if (index > (uint64_t)qi->highest)
		index = (uint64_t)qi->highest;

	node_t *qn = NULL;
	while (!qn) {
		qn = tree_quantile(&qi->ht->entries[index], quantile);
		if (!qn && (index <= (uint64_t)qi->highest)) {
			if (quantile >= 0.5)
				--index;
			else
				++index;
		} else {
			break;
		}
	}

	return qn ? qn->key : 0;
}

double randfrom(double min, double max) {
	double range = (max - min);
	double div = RAND_MAX / range;
	return min + (rand() / div);
}

static void usage(const char *prog) {
	fprintf(stderr,
		"Usage: %s [--size N] [--tree-height N] [--baseline] [--seed N]\n"
		"\n"
		"Experiment: quantiles\n"
		"Compare ordered RB-tree chained hash table quantiles against a sorted array baseline.\n"
		"\n"
		"Defaults: --size 1024 --tree-height 5\n",
		prog);
}

static void gen_data(quantiles_index_t *qi, true_t *t, double from, double to, uint64_t cnt,
                     uint64_t *qnanoseconds, uint64_t *tnanoseconds) {
	for (uint64_t i = 0; i < cnt; ++i) {
		double value = randfrom(from, to);
		struct timespec s_qinsert;
		struct timespec e_qinsert;
		struct timespec s_tinsert;
		struct timespec e_tinsert;

		clock_gettime(CLOCK_MONOTONIC, &s_qinsert);
		quantiles_insert(qi, value, 1);
		clock_gettime(CLOCK_MONOTONIC, &e_qinsert);
		*qnanoseconds += timespec_elapsed_nsec(&s_qinsert, &e_qinsert);

		clock_gettime(CLOCK_MONOTONIC, &s_tinsert);
		true_push(t, value, 1);
		clock_gettime(CLOCK_MONOTONIC, &e_tinsert);
		*tnanoseconds += timespec_elapsed_nsec(&s_tinsert, &e_tinsert);
	}
}

static int run_quantiles(uint64_t size, uint8_t tree_height, int with_baseline, unsigned seed) {
	quantiles_index_t *qi = quantiles_index_make(0, 1, size, tree_height);
	true_t *t = with_baseline ? true_init(111100000) : NULL;
	uint64_t qnanoseconds = 0;
	uint64_t tnanoseconds = 0;
	struct timespec s_qcalc;
	struct timespec e_qcalc;
	struct timespec s_tcalc;
	struct timespec e_tcalc;
	double q05;
	double q09;
	double q099;
	double t05 = 0;
	double t09 = 0;
	double t099 = 0;
	double insert_seconds;
	double search_seconds = 0;
	double baseline_insert_seconds = 0;
	double baseline_search_seconds = 0;

	srand(seed);
	nsec_tree = 0;

	gen_data(qi, t, 0.0, 0.1, 100000, &qnanoseconds, &tnanoseconds);
	gen_data(qi, t, 0.1, 0.2, 100000000, &qnanoseconds, &tnanoseconds);
	gen_data(qi, t, 0.3, 0.5, 10000000, &qnanoseconds, &tnanoseconds);
	gen_data(qi, t, 0.5, 0.7, 1000000, &qnanoseconds, &tnanoseconds);
	gen_data(qi, t, 0.7, 0.9, 100000, &qnanoseconds, &tnanoseconds);

	clock_gettime(CLOCK_MONOTONIC, &s_qcalc);
	q05 = quantile_calculate(qi, 0.5);
	q09 = quantile_calculate(qi, 0.9);
	q099 = quantile_calculate(qi, 0.99);
	clock_gettime(CLOCK_MONOTONIC, &e_qcalc);
	search_seconds = timespec_elapsed_sec(&s_qcalc, &e_qcalc);
	insert_seconds = qnanoseconds / 1000000000.0;
	baseline_insert_seconds = tnanoseconds / 1000000000.0;

	if (with_baseline && t) {
		clock_gettime(CLOCK_MONOTONIC, &s_tcalc);
		true_sort(t);
		t05 = true_quantile(t, 0.5);
		t09 = true_quantile(t, 0.9);
		t099 = true_quantile(t, 0.99);
		clock_gettime(CLOCK_MONOTONIC, &e_tcalc);
		baseline_search_seconds = timespec_elapsed_sec(&s_tcalc, &e_tcalc);
	}

	fprintf(stderr,
		"experiment=quantiles size=%llu tree_height=%u insert_seconds=%.3f search_seconds=%.3f "
		"tree_insert_seconds=%.3f q0.5=%.6f q0.9=%.6f q0.99=%.6f",
		(unsigned long long)size,
		tree_height,
		insert_seconds,
		search_seconds,
		nsec_tree / 1000000000.0,
		q05,
		q09,
		q099);

	if (with_baseline && t) {
		fprintf(stderr,
			" baseline_insert_seconds=%.3f baseline_search_seconds=%.3f "
			"baseline_q0.5=%.6f baseline_q0.9=%.6f baseline_q0.99=%.6f",
			baseline_insert_seconds,
			baseline_search_seconds,
			t05,
			t09,
			t099);
	}

	fprintf(stderr, "\n");
	return 0;
}

int main(int argc, char **argv) {
	uint64_t size = 1024;
	uint8_t tree_height = 5;
	int with_baseline = 0;
	unsigned seed = 1;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			usage(argv[0]);
			return 0;
		}
		if (!strcmp(argv[i], "--size") && i + 1 < argc) {
			size = strtoull(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--tree-height") && i + 1 < argc) {
			tree_height = (uint8_t)strtoul(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--baseline")) {
			with_baseline = 1;
			continue;
		}
		if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
			seed = (unsigned)strtoul(argv[++i], NULL, 10);
			continue;
		}
		fprintf(stderr, "unknown argument: %s\n", argv[i]);
		usage(argv[0]);
		return 1;
	}

	return run_quantiles(size, tree_height, with_baseline, seed);
}
