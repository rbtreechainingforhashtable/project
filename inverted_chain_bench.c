/*
 * Transfer experiment: real trigram posting-list lengths from words_alpha.txt
 * replayed into chain_ht as structural skew (one chain per n-gram, length =
 * posting-list size). Compares list / hybrid-batch / hybrid-incremental / tree.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/timing.h"
#include "hashtable/chain_ht.h"
#include "hashtable/list.h"
#include "ngram.h"

typedef struct {
	uint64_t *lens;
	uint64_t n;
	uint64_t cap;
} len_vec_t;

static uint64_t list_length(const list_t *list) {
	uint64_t n = 0;

	for (list_node *node = list ? list->head : NULL; node; node = node->next)
		++n;
	return n;
}

static void len_push(len_vec_t *v, uint64_t x) {
	if (v->n == v->cap) {
		v->cap = v->cap ? v->cap * 2 : 4096;
		v->lens = realloc(v->lens, v->cap * sizeof(*v->lens));
		if (!v->lens) {
			perror("realloc");
			exit(1);
		}
	}
	v->lens[v->n++] = x;
}

static void collect_posting_len(node_t *node, void *arg, void *arg2, void *arg3) {
	(void)arg;
	(void)arg2;
	len_push(arg3, list_length(node->data));
}

static int cmp_u64(const void *a, const void *b) {
	uint64_t x = *(const uint64_t *)a;
	uint64_t y = *(const uint64_t *)b;

	return (x > y) - (x < y);
}

static uint64_t pct(const uint64_t *a, uint64_t n, unsigned p) {
	uint64_t idx;

	if (!n)
		return 0;
	idx = (n * p) / 100;
	if (idx >= n)
		idx = n - 1;
	return a[idx];
}

/* Hash uses the leading ngram id so all L entries for one posting collide. */
static uint64_t g_bucket_id;

static uint64_t posting_bucket_hash(const char *key) {
	(void)key;
	return g_bucket_id;
}

static int run_replay(const char *label, chain_mode_t mode, treeify_policy_t policy,
                      uint8_t treeify, const uint64_t *lens, uint64_t ngrams,
                      uint64_t table_size) {
	chain_ht_t *ht;
	struct timespec t0, t1, s0, s1;
	uint64_t total_keys = 0;
	uint64_t i, j;
	char **keys;
	uint64_t kidx = 0;
	char buf[64];

	for (i = 0; i < ngrams; ++i)
		total_keys += lens[i];

	keys = calloc(total_keys, sizeof(*keys));
	if (!keys)
		return 1;

	ht = chain_ht_new(table_size, posting_bucket_hash, mode, treeify, policy);
	if (!ht) {
		free(keys);
		return 1;
	}

	timespec_now(&t0);
	for (i = 0; i < ngrams; ++i) {
		g_bucket_id = i;
		for (j = 0; j < lens[i]; ++j) {
			snprintf(buf, sizeof(buf), "%08" PRIx64 ":%08" PRIx64, i, j);
			keys[kidx] = strdup(buf);
			if (!keys[kidx] || !chain_ht_insert(ht, keys[kidx], keys[kidx])) {
				fprintf(stderr, "insert failed i=%llu j=%llu\n",
					(unsigned long long)i, (unsigned long long)j);
				return 1;
			}
			++kidx;
		}
	}
	if (mode == CHAIN_MODE_HYBRID && policy == TREEIFY_POLICY_BATCH)
		chain_ht_finalize(ht);
	timespec_now(&t1);

	chain_ht_reset_stats(ht);
	timespec_now(&s0);
	for (i = 0; i < total_keys; ++i) {
		g_bucket_id = strtoull(keys[i], NULL, 16);
		if (!chain_ht_get(ht, keys[i])) {
			fprintf(stderr, "lookup miss %s\n", keys[i]);
			return 1;
		}
	}
	timespec_now(&s1);

	fprintf(stderr,
		"experiment=inverted-chain-replay mode=%s policy=%s treeify=%u ngrams=%llu keys=%llu "
		"table=%llu insert_seconds=%.3f search_seconds=%.3f comparisons=%llu avg_comparisons=%.3f "
		"max_bucket=%llu heap_bytes=%llu treeify_events=%llu\n",
		label,
		mode == CHAIN_MODE_HYBRID
			? (policy == TREEIFY_POLICY_INCREMENTAL ? "incremental" : "batch")
			: "n/a",
		treeify,
		(unsigned long long)ngrams,
		(unsigned long long)total_keys,
		(unsigned long long)table_size,
		timespec_elapsed_sec(&t0, &t1),
		timespec_elapsed_sec(&s0, &s1),
		(unsigned long long)chain_ht_lookup_comparisons(ht),
		total_keys ? (double)chain_ht_lookup_comparisons(ht) / total_keys : 0.0,
		(unsigned long long)chain_ht_max_bucket_size(ht),
		(unsigned long long)chain_ht_heap_bytes(ht),
		(unsigned long long)chain_ht_treeify_events(ht));

	chain_ht_free(ht);
	for (i = 0; i < total_keys; ++i)
		free(keys[i]);
	free(keys);
	return 0;
}

static void usage(const char *prog) {
	fprintf(stderr,
		"Usage: %s [--words PATH] [--max-ngrams N] [--min-len N]\n"
		"\n"
		"Build a trigram inverted index, report posting-list CDF, then replay the\n"
		"hottest posting lengths into chain_ht (list / hybrid / tree).\n"
		"Default --max-ngrams 2048 (hottest). Use a large N for more coverage.\n",
		prog);
}

int main(int argc, char **argv) {
	const char *words_path = "vendor/english-words/words_alpha.txt";
	uint64_t max_ngrams = 2048;
	uint64_t min_len = 1;
	ngram_index_t *index;
	len_vec_t all = {0};
	uint64_t *lens = NULL;
	uint64_t ngrams = 0;
	uint64_t sum = 0;
	uint64_t i;
	double load_seconds = 0;
	uint64_t words;
	uint64_t table_size;

	for (int a = 1; a < argc; ++a) {
		if (!strcmp(argv[a], "--help") || !strcmp(argv[a], "-h")) {
			usage(argv[0]);
			return 0;
		}
		if (!strcmp(argv[a], "--words") && a + 1 < argc) {
			words_path = argv[++a];
			continue;
		}
		if (!strcmp(argv[a], "--max-ngrams") && a + 1 < argc) {
			max_ngrams = strtoull(argv[++a], NULL, 10);
			continue;
		}
		if (!strcmp(argv[a], "--min-len") && a + 1 < argc) {
			min_len = strtoull(argv[++a], NULL, 10);
			continue;
		}
		fprintf(stderr, "unknown argument: %s\n", argv[a]);
		usage(argv[0]);
		return 1;
	}

	index = ngram_index_make_size(4096);
	words = ngram_index_build_from_file(index, words_path, 1, &load_seconds);
	if (!words) {
		fprintf(stderr, "failed to load %s\n", words_path);
		return 1;
	}

	hashtable_foreach(index->ht, collect_posting_len, &all);
	qsort(all.lens, all.n, sizeof(*all.lens), cmp_u64);
	for (i = 0; i < all.n; ++i)
		sum += all.lens[i];

	fprintf(stderr,
		"experiment=inverted-posting-cdf words=%llu unique_trigrams=%llu load_seconds=%.3f "
		"total_postings=%llu min=%llu p50=%llu p90=%llu p99=%llu max=%llu mean=%.1f\n",
		(unsigned long long)words,
		(unsigned long long)all.n,
		load_seconds,
		(unsigned long long)sum,
		(unsigned long long)(all.n ? all.lens[0] : 0),
		(unsigned long long)pct(all.lens, all.n, 50),
		(unsigned long long)pct(all.lens, all.n, 90),
		(unsigned long long)pct(all.lens, all.n, 99),
		(unsigned long long)(all.n ? all.lens[all.n - 1] : 0),
		all.n ? sum * 1.0 / all.n : 0.0);

	/* Keep postings >= min_len; then take the hottest max_ngrams. */
	for (i = 0; i < all.n; ++i) {
		if (all.lens[i] >= min_len)
			++ngrams;
	}
	if (max_ngrams && ngrams > max_ngrams)
		ngrams = max_ngrams;

	lens = malloc(ngrams * sizeof(*lens));
	if (!lens)
		return 1;

	{
		uint64_t written = 0;
		/* Walk descending (hottest first) among those >= min_len. */
		for (i = all.n; i-- > 0 && written < ngrams; ) {
			if (all.lens[i] >= min_len)
				lens[written++] = all.lens[i];
		}
		ngrams = written;
	}
	free(all.lens);

	sum = 0;
	for (i = 0; i < ngrams; ++i)
		sum += lens[i];

	/* One bucket per n-gram id. */
	table_size = ngrams ? ngrams : 1;

	fprintf(stderr,
		"experiment=inverted-chain-replay-plan ngrams=%llu keys=%llu table=%llu "
		"max_bucket_expected=%llu\n",
		(unsigned long long)ngrams,
		(unsigned long long)sum,
		(unsigned long long)table_size,
		(unsigned long long)(ngrams ? lens[0] : 0));

	if (run_replay("list", CHAIN_MODE_LIST, TREEIFY_POLICY_BATCH, 255,
	                lens, ngrams, table_size) != 0)
		return 1;
	if (run_replay("hybrid-8-batch", CHAIN_MODE_HYBRID, TREEIFY_POLICY_BATCH, 8,
	                lens, ngrams, table_size) != 0)
		return 1;
	if (run_replay("hybrid-8-incremental", CHAIN_MODE_HYBRID, TREEIFY_POLICY_INCREMENTAL, 8,
	                lens, ngrams, table_size) != 0)
		return 1;
	if (run_replay("tree", CHAIN_MODE_TREE, TREEIFY_POLICY_BATCH, 0,
	                lens, ngrams, table_size) != 0)
		return 1;

	free(lens);
	ngram_index_delete(index);
	return 0;
}
