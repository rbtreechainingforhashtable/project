#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common/lineio.h"
#include "common/timing.h"

#define ALPHABET_SIZE 26
typedef struct trie_t {
	struct trie_t *children[ALPHABET_SIZE];
	uint8_t terminate;
} trie_t;

static void usage(const char *prog) {
	fprintf(stderr,
		"Usage: %s [--words PATH] [--letters N] [--queries N] [--all]\n"
		"\n"
		"Experiment: trie\n"
		"Load a prefix trie and benchmark prefix lookups for words of a fixed length.\n"
		"\n"
		"Defaults: --letters 3 --queries 4000\n"
		"Use --all to run the benchmark for letters 3 through 7.\n",
		prog);
}

static void trie_insert(trie_t *trie, char *key, unsigned key_len) {
	if (!key_len) {
		trie->terminate = 1;
		return;
	}

	const unsigned int index = key[0] - 'a';
	if (ALPHABET_SIZE <= index)
		return;

	if (NULL == trie->children[index])
		trie->children[index] = calloc(1, sizeof(trie_t));

	trie_insert(trie->children[index], key + 1, key_len - 1);
}

static void trie_search(trie_t *trie, char *key, unsigned key_len, trie_t **result) {
	if (!key_len) {
		*result = trie;
		return;
	}

	const unsigned int index = key[0] - 'a';
	if (ALPHABET_SIZE <= index)
		return;

	if (!trie->children[index])
		return;

	trie_search(trie->children[index], key + 1, key_len - 1, result);
}

static void trie_print(trie_t *trie, char prefix[], unsigned prefix_len, uint64_t *matched) {
	if (!trie)
		return;

	if (trie->terminate)
		++*matched;

	for (int i = 0; i < ALPHABET_SIZE; i++) {
		if (NULL == trie->children[i])
			continue;

		prefix[prefix_len] = (char)(i + 'a');
		trie_print(trie->children[i], prefix, prefix_len + 1, matched);
	}
}

static int check_trie(const char *words_path, trie_t *root, uint64_t letters, uint64_t maxnum) {
	struct timespec checking;
	struct timespec checked;
	char key[256];
	uint64_t matched = 0;
	uint64_t resulted = 0;
	double search_seconds;
	FILE *fd = fopen(words_path, "r");

	if (!fd) {
		perror(words_path);
		return 1;
	}

	timespec_now(&checking);
	while (fgets(key, sizeof(key), fd) && maxnum) {
		size_t key_len = strip_line_ending(key);
		if (key_len != letters)
			continue;

		--maxnum;
		++resulted;
		trie_t *trie = NULL;
		trie_search(root, key, (unsigned)key_len, &trie);
		trie_print(trie, key, (unsigned)key_len, &matched);
	}
	fclose(fd);
	timespec_now(&checked);
	search_seconds = timespec_elapsed_sec(&checking, &checked);

	fprintf(stderr,
		"experiment=trie letters=%llu checked=%llu matched=%llu search_seconds=%.3f results_per_query=%.3f\n",
		(unsigned long long)letters,
		(unsigned long long)resulted,
		(unsigned long long)matched,
		search_seconds,
		resulted ? (matched * 1.0) / resulted : 0.0);
	return 0;
}

static int run_trie_benchmark(const char *words_path, uint64_t letters, uint64_t maxnum) {
	trie_t *root = calloc(1, sizeof(trie_t));
	char key[256];
	struct timespec pre;
	struct timespec loaded;
	double load_seconds;
	FILE *fd;

	if (!root)
		return 1;

	timespec_now(&pre);
	fd = fopen(words_path, "r");
	if (!fd) {
		perror(words_path);
		free(root);
		return 1;
	}

	while (fgets(key, sizeof(key), fd)) {
		size_t key_len = strip_line_ending(key);
		trie_insert(root, key, (unsigned)key_len);
	}
	fclose(fd);
	timespec_now(&loaded);
	load_seconds = timespec_elapsed_sec(&pre, &loaded);
	fprintf(stderr, "experiment=trie load_seconds=%.3f\n", load_seconds);

	return check_trie(words_path, root, letters, maxnum);
}

int main(int argc, char **argv) {
	char *words_path = "vendor/english-words/words_alpha.txt";
	uint64_t letters = 3;
	uint64_t maxnum = 4000;
	bool run_all = false;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			usage(argv[0]);
			return 0;
		}
		if (!strcmp(argv[i], "--words") && i + 1 < argc) {
			words_path = argv[++i];
			continue;
		}
		if (!strcmp(argv[i], "--letters") && i + 1 < argc) {
			letters = strtoull(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--queries") && i + 1 < argc) {
			maxnum = strtoull(argv[++i], NULL, 10);
			continue;
		}
		if (!strcmp(argv[i], "--all")) {
			run_all = true;
			continue;
		}
		fprintf(stderr, "unknown argument: %s\n", argv[i]);
		usage(argv[0]);
		return 1;
	}

	if (run_all) {
		int rc = 0;
		for (uint64_t n = 3; n <= 7; ++n) {
			if (run_trie_benchmark(words_path, n, maxnum) != 0)
				rc = 1;
		}
		return rc;
	}

	return run_trie_benchmark(words_path, letters, maxnum);
}
