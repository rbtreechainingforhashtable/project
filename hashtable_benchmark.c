#include <inttypes.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__GLIBC__)
#define GNU_HSEARCH_REENTRANT 1
#endif

#include "common/randstring.h"
#include "common/timing.h"
#include "hashtable.h"

#ifdef WITH_TOMMY
#include "tommy.h"

typedef struct tnode {
	char *key;
	tommy_node node;
} tnode;

static int tommy_check(const void *arg, const void *obj) {
	return strcmp((const char *)arg, ((const tnode *)obj)->key);
}
#endif

typedef struct table_t {
#ifdef GNU_HSEARCH_REENTRANT
	struct hsearch_data htab;
#endif
	size_t size;
} table_t;

static void usage(const char *prog) {
	fprintf(stderr,
		"Usage: %s --impl {gnu,custom,tommy} [--keys N] [--key-len N] [--data-file PATH] [--seed N]\n"
		"\n"
		"Experiment: hashtable-benchmark\n"
		"Insert randomized string keys into a hash table, write them to a file,\n"
		"then look up every key from that file and report init/search timings.\n"
		"\n"
		"Defaults: --keys 33554432 --key-len 10 --data-file /tmp/hashtable-benchmark-keys.txt\n",
		prog);
}

static table_t *gnu_table_create(size_t size) {
	table_t *table = calloc(1, sizeof(*table));
	if (!table)
		return NULL;
#ifdef GNU_HSEARCH_REENTRANT
	if (!hcreate_r(size, &table->htab)) {
#else
	if (!hcreate(size)) {
#endif
		free(table);
		return NULL;
	}
	return table;
}

static void gnu_table_destroy(table_t *table) {
	if (!table)
		return;
#ifdef GNU_HSEARCH_REENTRANT
	hdestroy_r(&table->htab);
#else
	hdestroy();
#endif
	free(table);
}

static int gnu_table_add(table_t *table, char *key) {
	ENTRY e = {.key = key, .data = key};
#ifdef GNU_HSEARCH_REENTRANT
	ENTRY *ep = NULL;
	if (hsearch_r(e, FIND, &ep, &table->htab))
		return 0;
	if (!hsearch_r(e, ENTER, &ep, &table->htab))
		return 0;
#else
	ENTRY *ep = hsearch(e, FIND);
	if (ep)
		return 0;
	ep = hsearch(e, ENTER);
	if (!ep)
		return 0;
#endif
	++table->size;
	return 1;
}

static char *gnu_table_get(table_t *table, char *key) {
	ENTRY e = {.key = key};
#ifdef GNU_HSEARCH_REENTRANT
	ENTRY *ep = NULL;
	if (!hsearch_r(e, FIND, &ep, &table->htab) || !ep)
		return NULL;
#else
	ENTRY *ep = hsearch(e, FIND);
	if (!ep)
		return NULL;
#endif
	return ep->data;
}

static int benchmark_gnu(uint64_t key_count, size_t key_len, const char *data_file, unsigned seed) {
	table_t *table;
	FILE *fd;
	struct timespec init_start;
	struct timespec init_end;
	struct timespec search_start;
	struct timespec search_end;
	char buf[256];
	double init_seconds;
	double search_seconds;

	srand(seed);
	table = gnu_table_create((size_t)key_count);
	if (!table) {
		fprintf(stderr, "failed to create GNU hsearch table\n");
		return 1;
	}

	timespec_now(&init_start);
	fd = fopen(data_file, "w");
	if (!fd) {
		perror(data_file);
		gnu_table_destroy(table);
		return 1;
	}

	for (uint64_t i = 0; i < key_count; ++i) {
		char *new_key = get_rand_string(key_len);
		if (!new_key) {
			fclose(fd);
			gnu_table_destroy(table);
			return 1;
		}
		gnu_table_add(table, new_key);
		fputs(new_key, fd);
		fputs("\n", fd);
	}
	fclose(fd);
	timespec_now(&init_end);
	init_seconds = timespec_elapsed_sec(&init_start, &init_end);

	timespec_now(&search_start);
	fd = fopen(data_file, "r");
	if (!fd) {
		perror(data_file);
		gnu_table_destroy(table);
		return 1;
	}
	while (fgets(buf, sizeof(buf), fd)) {
		size_t len = strlen(buf) - 1;
		buf[len] = 0;
		gnu_table_get(table, buf);
	}
	fclose(fd);
	timespec_now(&search_end);
	search_seconds = timespec_elapsed_sec(&search_start, &search_end);

	fprintf(stderr,
		"experiment=hashtable-benchmark impl=gnu keys=%llu init_seconds=%.3f search_seconds=%.3f size=%zu\n",
		(unsigned long long)key_count,
		init_seconds,
		search_seconds,
		table->size);

	gnu_table_destroy(table);
	return 0;
}

static int benchmark_custom(uint64_t key_count, size_t key_len, const char *data_file, unsigned seed) {
	hashtable_t *ht;
	FILE *fd;
	struct timespec init_start;
	struct timespec init_end;
	struct timespec search_start;
	struct timespec search_end;
	char buf[256];
	double init_seconds;
	double search_seconds;

	srand(seed);
	ht = hashtable_new(key_count, fnv);
	if (!ht)
		return 1;

	timespec_now(&init_start);
	fd = fopen(data_file, "w");
	if (!fd) {
		perror(data_file);
		hashtable_free(ht);
		return 1;
	}

	for (uint64_t i = 0; i < key_count; ++i) {
		char *new_key = get_rand_string(key_len);
		if (!new_key) {
			fclose(fd);
			hashtable_free(ht);
			return 1;
		}
		hashtable_insert_auto(ht, new_key, new_key);
		fputs(new_key, fd);
		fputs("\n", fd);
	}
	fclose(fd);
	timespec_now(&init_end);
	init_seconds = timespec_elapsed_sec(&init_start, &init_end);

	timespec_now(&search_start);
	fd = fopen(data_file, "r");
	if (!fd) {
		perror(data_file);
		hashtable_free(ht);
		return 1;
	}
	while (fgets(buf, sizeof(buf), fd)) {
		size_t len = strlen(buf) - 1;
		buf[len] = 0;
		hashtable_get_auto(ht, buf);
	}
	fclose(fd);
	timespec_now(&search_end);
	search_seconds = timespec_elapsed_sec(&search_start, &search_end);

	fprintf(stderr,
		"experiment=hashtable-benchmark impl=custom keys=%llu init_seconds=%.3f search_seconds=%.3f size=%llu\n",
		(unsigned long long)key_count,
		init_seconds,
		search_seconds,
		(unsigned long long)ht->count);

	hashtable_free(ht);
	return 0;
}

#ifdef WITH_TOMMY
static int benchmark_tommy(uint64_t key_count, size_t key_len, const char *data_file, unsigned seed) {
	tommy_hashdyn hashdyn;
	FILE *fd;
	struct timespec init_start;
	struct timespec init_end;
	struct timespec search_start;
	struct timespec search_end;
	char buf[256];
	double init_seconds;
	double search_seconds;

	srand(seed);
	tommy_hashdyn_init(&hashdyn);

	timespec_now(&init_start);
	fd = fopen(data_file, "w");
	if (!fd) {
		perror(data_file);
		tommy_hashdyn_done(&hashdyn);
		return 1;
	}

	for (uint64_t i = 0; i < key_count; ++i) {
		char *new_key = get_rand_string(key_len);
		tnode *td;
		uint32_t hash_sum;
		tnode *node;

		if (!new_key) {
			fclose(fd);
			tommy_hashdyn_done(&hashdyn);
			return 1;
		}

		td = malloc(sizeof(*td));
		if (!td) {
			free(new_key);
			fclose(fd);
			tommy_hashdyn_done(&hashdyn);
			return 1;
		}
		td->key = new_key;
		hash_sum = tommy_strhash_u32(0, new_key);
		node = tommy_hashdyn_search(&hashdyn, tommy_check, new_key, hash_sum);
		if (!node)
			tommy_hashdyn_insert(&hashdyn, &td->node, td, hash_sum);
		fputs(new_key, fd);
		fputs("\n", fd);
	}
	fclose(fd);
	timespec_now(&init_end);
	init_seconds = timespec_elapsed_sec(&init_start, &init_end);

	timespec_now(&search_start);
	fd = fopen(data_file, "r");
	if (!fd) {
		perror(data_file);
		tommy_hashdyn_done(&hashdyn);
		return 1;
	}
	while (fgets(buf, sizeof(buf), fd)) {
		size_t len = strlen(buf) - 1;
		uint32_t hash_sum;

		buf[len] = 0;
		hash_sum = tommy_strhash_u32(0, buf);
		tommy_hashdyn_search(&hashdyn, tommy_check, buf, hash_sum);
	}
	fclose(fd);
	timespec_now(&search_end);
	search_seconds = timespec_elapsed_sec(&search_start, &search_end);

	fprintf(stderr,
		"experiment=hashtable-benchmark impl=tommy keys=%llu init_seconds=%.3f search_seconds=%.3f size=%llu memory=%llu\n",
		(unsigned long long)key_count,
		init_seconds,
		search_seconds,
		(unsigned long long)tommy_hashdyn_count(&hashdyn),
		(unsigned long long)tommy_hashdyn_memory_usage(&hashdyn));

	tommy_hashdyn_done(&hashdyn);
	return 0;
}
#endif

int main(int argc, char **argv) {
	const char *impl = NULL;
	const char *data_file = "/tmp/hashtable-benchmark-keys.txt";
	uint64_t key_count = 33554432;
	size_t key_len = 10;
	unsigned seed = 1;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			usage(argv[0]);
			return 0;
		}
		if (!strcmp(argv[i], "--impl") && i + 1 < argc) {
			impl = argv[++i];
			continue;
		}
		if (!strcmp(argv[i], "--keys") && i + 1 < argc) {
			key_count = strtoull(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--key-len") && i + 1 < argc) {
			key_len = (size_t)strtoull(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--data-file") && i + 1 < argc) {
			data_file = argv[++i];
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

	if (!impl) {
		fprintf(stderr, "--impl is required\n");
		usage(argv[0]);
		return 1;
	}

	if (!strcmp(impl, "gnu"))
		return benchmark_gnu(key_count, key_len, data_file, seed);
	if (!strcmp(impl, "custom"))
		return benchmark_custom(key_count, key_len, data_file, seed);
#ifdef WITH_TOMMY
	if (!strcmp(impl, "tommy"))
		return benchmark_tommy(key_count, key_len, data_file, seed);
#endif

	fprintf(stderr, "unknown implementation: %s\n", impl);
#ifdef WITH_TOMMY
	fprintf(stderr, "supported values: gnu, custom, tommy\n");
#else
	fprintf(stderr, "supported values: gnu, custom\n");
	fprintf(stderr, "rebuild with TOMMYDIR set to enable tommy\n");
#endif
	return 1;
}
