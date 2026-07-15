#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common/randstring.h"
#include "common/timing.h"
#include "hashtable/chain_ht.h"
#include "hashtable/hashtable.h"

typedef enum {
	WORKLOAD_UNIFORM = 0,
	WORKLOAD_SKEW = 1,
} workload_t;

typedef struct bench_result {
	const char *label;
	double insert_seconds;
	double search_seconds;
	uint64_t comparisons;
	uint64_t max_bucket;
	uint64_t count;
	uint64_t heap_bytes;
} bench_result;

static uint64_t skew_bucket_hash(const char *key) {
	return strtoull(key, NULL, 10);
}

static void gen_uniform_key(char *buf, size_t buflen, uint64_t seq, size_t key_len) {
	char *suffix = get_rand_string(key_len + 1);

	if (!suffix) {
		snprintf(buf, buflen, "%012" PRIu64, seq);
		return;
	}

	snprintf(buf, buflen, "%012" PRIu64 ":%s", seq, suffix);
	free(suffix);
}

static void gen_skew_key(char *buf, size_t buflen, uint64_t bucket_id, uint64_t seq,
                         size_t suffix_len) {
	char suffix[32];
	size_t i;

	for (i = 0; i < suffix_len; ++i)
		suffix[i] = 'a' + (char)((seq + i * 17 + bucket_id) % 26);
	suffix[suffix_len] = '\0';

	snprintf(buf, buflen, "%04" PRIu64 ":%016" PRIu64 ":%s", bucket_id, seq, suffix);
}

static int run_chain_benchmark(workload_t workload, chain_mode_t mode, uint8_t treeify_threshold,
                               uint64_t key_count, uint64_t table_size, uint64_t hot_buckets,
                               size_t key_len, unsigned seed, bench_result *out) {
	chain_ht_t *ht;
	uint64_t (*hash_func)(const char *key) = fnv;
	char **keys;
	struct timespec insert_start;
	struct timespec insert_end;
	struct timespec search_start;
	struct timespec search_end;
	uint64_t i;
	const char *mode_label = mode == CHAIN_MODE_LIST ? "list"
		: mode == CHAIN_MODE_TREE ? "tree" : "hybrid";

	srand(seed);
	keys = calloc(key_count, sizeof(*keys));
	if (!keys)
		return 1;

	if (workload == WORKLOAD_SKEW)
		hash_func = skew_bucket_hash;

	ht = chain_ht_new(table_size, hash_func, mode, treeify_threshold);
	if (!ht) {
		free(keys);
		return 1;
	}

	timespec_now(&insert_start);
	for (i = 0; i < key_count; ++i) {
		char buf[128];
		char *stored;

		if (workload == WORKLOAD_UNIFORM) {
			gen_uniform_key(buf, sizeof(buf), i, key_len);
		} else {
			gen_skew_key(buf, sizeof(buf), i % hot_buckets, i / hot_buckets + 1, key_len);
		}

		stored = strdup(buf);
		keys[i] = stored;
		if (!stored || !chain_ht_insert(ht, stored, stored)) {
			fprintf(stderr, "insert failed at i=%llu key=%s\n",
				(unsigned long long)i, stored ? stored : "(null)");
			chain_ht_free(ht);
			for (uint64_t j = 0; j <= i; ++j)
				free(keys[j]);
			free(keys);
			return 1;
		}
	}
	if (chain_ht_count(ht) != key_count) {
		fprintf(stderr, "inserted count mismatch: got %llu expected %llu\n",
			(unsigned long long)chain_ht_count(ht),
			(unsigned long long)key_count);
	}
	if (mode == CHAIN_MODE_HYBRID)
		chain_ht_finalize(ht);
	timespec_now(&insert_end);

	chain_ht_reset_stats(ht);
	timespec_now(&search_start);
	for (i = 0; i < key_count; ++i) {
		if (!chain_ht_get(ht, keys[i])) {
			fprintf(stderr, "lookup miss at i=%llu key=%s\n",
				(unsigned long long)i, keys[i]);
			chain_ht_free(ht);
			for (uint64_t j = 0; j < key_count; ++j)
				free(keys[j]);
			free(keys);
			return 1;
		}
	}
	timespec_now(&search_end);

	if (mode == CHAIN_MODE_HYBRID) {
		static char hybrid_label[32];
		snprintf(hybrid_label, sizeof(hybrid_label), "hybrid-%u", treeify_threshold);
		out->label = hybrid_label;
	} else {
		out->label = mode_label;
	}
	out->insert_seconds = timespec_elapsed_sec(&insert_start, &insert_end);
	out->search_seconds = timespec_elapsed_sec(&search_start, &search_end);
	out->comparisons = chain_ht_lookup_comparisons(ht);
	out->max_bucket = chain_ht_max_bucket_size(ht);
	out->count = chain_ht_count(ht);
	out->heap_bytes = chain_ht_heap_bytes(ht);

	fprintf(stderr,
		"experiment=chaining-benchmark workload=%s mode=%s treeify=%u keys=%llu table=%llu hot_buckets=%llu "
		"insert_seconds=%.3f search_seconds=%.3f comparisons=%llu avg_comparisons=%.3f max_bucket=%llu heap_bytes=%llu\n",
		workload == WORKLOAD_UNIFORM ? "uniform" : "skew",
		out->label,
		treeify_threshold,
		(unsigned long long)key_count,
		(unsigned long long)table_size,
		(unsigned long long)hot_buckets,
		out->insert_seconds,
		out->search_seconds,
		(unsigned long long)out->comparisons,
		key_count ? (double)out->comparisons / key_count : 0.0,
		(unsigned long long)out->max_bucket,
		(unsigned long long)out->heap_bytes);

	chain_ht_free(ht);
	for (i = 0; i < key_count; ++i)
		free(keys[i]);
	free(keys);
	return 0;
}

static void print_lookup_model(void) {
	static const struct {
		uint64_t chain_len;
		uint64_t list_cmp;
		uint64_t tree_cmp;
	} model[] = {
		{8, 8, 3}, {7, 7, 3}, {6, 6, 3}, {5, 5, 3},
		{4, 4, 2}, {3, 3, 2}, {2, 2, 1}, {1, 1, 1},
	};
	size_t i;

	printf("chain_len\tlist_comparisons\ttree_comparisons\n");
	for (i = 0; i < sizeof(model) / sizeof(model[0]); ++i) {
		printf("%llu\t%llu\t%llu\n",
			(unsigned long long)model[i].chain_len,
			(unsigned long long)model[i].list_cmp,
			(unsigned long long)model[i].tree_cmp);
	}
}

static void usage(const char *prog) {
	fprintf(stderr,
		"Usage: %s --suite {compare,treeify,model} [options]\n"
		"\n"
		"Options:\n"
		"  --keys N           Number of keys (default 500000)\n"
		"  --table-size N     Hash table size (default 4096)\n"
		"  --hot-buckets N    Skew buckets (default 8)\n"
		"  --key-len N        Random suffix length (default 8)\n"
		"  --mode NAME        list, tree, or hybrid (compare suite only)\n"
		"  --workload NAME    uniform or skew (compare suite only)\n"
		"  --treeify N        Hybrid threshold (default 8)\n",
		prog);
}

int main(int argc, char **argv) {
	const char *suite = NULL;
	const char *mode_name = NULL;
	const char *workload_name = NULL;
	uint8_t treeify_threshold = 8;
	uint64_t key_count = 500000;
	uint64_t table_size = 4096;
	uint64_t hot_buckets = 8;
	size_t key_len = 8;
	unsigned seed = 1;
	uint8_t thresholds[] = {255, 8, 4, 2, 1, 0};
	size_t ti;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			usage(argv[0]);
			return 0;
		}
		if (!strcmp(argv[i], "--suite") && i + 1 < argc) {
			suite = argv[++i];
			continue;
		}
		if (!strcmp(argv[i], "--keys") && i + 1 < argc) {
			key_count = strtoull(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--table-size") && i + 1 < argc) {
			table_size = strtoull(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--hot-buckets") && i + 1 < argc) {
			hot_buckets = strtoull(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--key-len") && i + 1 < argc) {
			key_len = (size_t)strtoull(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--mode") && i + 1 < argc) {
			mode_name = argv[++i];
			continue;
		}
		if (!strcmp(argv[i], "--workload") && i + 1 < argc) {
			workload_name = argv[++i];
			continue;
		}
		if (!strcmp(argv[i], "--treeify") && i + 1 < argc) {
			treeify_threshold = (uint8_t)strtoul(argv[++i], NULL, 10);
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

	if (!suite) {
		fprintf(stderr, "--suite is required\n");
		usage(argv[0]);
		return 1;
	}

	if (!strcmp(suite, "model")) {
		print_lookup_model();
		return 0;
	}

	if (!strcmp(suite, "compare")) {
		bench_result result;

		if (mode_name && workload_name) {
			chain_mode_t mode = CHAIN_MODE_HYBRID;
			workload_t workload = WORKLOAD_UNIFORM;

			if (!strcmp(mode_name, "list"))
				mode = CHAIN_MODE_LIST;
			else if (!strcmp(mode_name, "tree"))
				mode = CHAIN_MODE_TREE;
			else if (!strcmp(mode_name, "hybrid"))
				mode = CHAIN_MODE_HYBRID;
			else {
				fprintf(stderr, "unknown mode: %s\n", mode_name);
				return 1;
			}

			if (!strcmp(workload_name, "uniform"))
				workload = WORKLOAD_UNIFORM;
			else if (!strcmp(workload_name, "skew"))
				workload = WORKLOAD_SKEW;
			else {
				fprintf(stderr, "unknown workload: %s\n", workload_name);
				return 1;
			}

			return run_chain_benchmark(workload, mode, treeify_threshold, key_count,
			                           table_size, hot_buckets, key_len, seed, &result);
		}

		chain_mode_t modes[] = {CHAIN_MODE_LIST, CHAIN_MODE_HYBRID, CHAIN_MODE_TREE};
		uint8_t treeify[] = {255, 8, 0};
		workload_t workloads[] = {WORKLOAD_UNIFORM, WORKLOAD_SKEW};

		for (size_t w = 0; w < 2; ++w) {
			for (size_t m = 0; m < 3; ++m) {
				if (run_chain_benchmark(workloads[w], modes[m], treeify[m], key_count,
				                        table_size, hot_buckets, key_len, seed, &result) != 0)
					return 1;
			}
		}
		return 0;
	}

	if (!strcmp(suite, "treeify")) {
		bench_result result;

		for (ti = 0; ti < sizeof(thresholds); ++ti) {
			chain_mode_t mode = thresholds[ti] == 255 ? CHAIN_MODE_LIST
				: thresholds[ti] == 0 ? CHAIN_MODE_TREE : CHAIN_MODE_HYBRID;
			uint8_t threshold = thresholds[ti] == 255 ? 255 : thresholds[ti];

			if (run_chain_benchmark(WORKLOAD_SKEW, mode, threshold, key_count,
			                        table_size, hot_buckets, key_len, seed, &result) != 0)
				return 1;
		}
		return 0;
	}

	fprintf(stderr, "unknown suite: %s\n", suite);
	return 1;
}
