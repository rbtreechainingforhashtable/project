#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ngram.h"

static void usage(const char *prog) {
	fprintf(stderr,
		"Usage: %s [--words PATH] [--letters N] [--queries N]\n"
		"\n"
		"Experiment: inverted-index\n"
		"Build a multi-gram inverted index (2/3/4/5-grams) and benchmark substring\n"
		"lookups for words of a fixed length.\n"
		"\n"
		"Defaults: --letters 3 --queries 4000\n",
		prog);
}

int main(int argc, char **argv) {
	char *words_path = "vendor/english-words/words_alpha.txt";
	uint64_t letters = 3;
	uint64_t max_queries = 4000;
	ngram_index_t *ngram_index;
	uint64_t words;
	uint64_t checked;
	uint64_t matched = 0;
	double load_seconds = 0;
	double search_seconds = 0;

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
			max_queries = strtoull(argv[++i], NULL, 10);
			continue;
		}
		fprintf(stderr, "unknown argument: %s\n", argv[i]);
		usage(argv[0]);
		return 1;
	}

	ngram_index = ngram_index_make();
	words = ngram_index_build_from_file(ngram_index, words_path, 0, &load_seconds);
	checked = ngram_index_search_file(ngram_index, words_path, letters, max_queries,
		&matched, &search_seconds);

	fprintf(stderr,
		"experiment=inverted-index words=%llu unique_ngrams=%llu load_seconds=%.3f "
		"letters=%llu checked=%llu matched=%llu search_seconds=%.3f results_per_query=%.3f\n",
		(unsigned long long)words,
		(unsigned long long)ngram_index->size,
		load_seconds,
		(unsigned long long)letters,
		(unsigned long long)checked,
		(unsigned long long)matched,
		search_seconds,
		checked ? (matched * 1.0) / checked : 0.0);

	ngram_index_delete(ngram_index);
	return 0;
}
