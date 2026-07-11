#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common/lineio.h"
#include "common/timing.h"
#include "ngram.h"

static void usage(const char *prog) {
	fprintf(stderr,
		"Usage: %s [--words PATH]\n"
		"\n"
		"Experiment: ngram-growth\n"
		"Build a trigram inverted index and print the growth of unique trigrams\n"
		"while loading the dictionary.\n",
		prog);
}

int main(int argc, char **argv) {
	char *words_path = "vendor/english-words/words_alpha.txt";
	ngram_index_t *ngram_index;
	FILE *fd;
	struct timespec pre;
	struct timespec loaded;
	char buf[256];
	uint64_t words = 0;
	uint64_t step = 1;
	uint64_t transition_point = 10;
	double load_seconds;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			usage(argv[0]);
			return 0;
		}
		if (!strcmp(argv[i], "--words") && i + 1 < argc) {
			words_path = argv[++i];
			continue;
		}
		fprintf(stderr, "unknown argument: %s\n", argv[i]);
		usage(argv[0]);
		return 1;
	}

	ngram_index = ngram_index_make();
	fd = fopen(words_path, "r");
	if (!fd) {
		perror(words_path);
		return 1;
	}

	timespec_now(&pre);
	while (fgets(buf, sizeof(buf), fd)) {
		++words;
		strip_line_ending(buf);
		ngram_index_add_word_trigrams(ngram_index, buf);
		if (words <= transition_point && (words % step) == 0)
			printf("words: %llu, ngrams: %llu: %s\n",
				(unsigned long long)words,
				(unsigned long long)ngram_index->size,
				buf);
		if (words == transition_point && (step = transition_point, transition_point *= 10)) {}
	}
	fclose(fd);
	timespec_now(&loaded);
	load_seconds = timespec_elapsed_sec(&pre, &loaded);

	fprintf(stderr,
		"experiment=ngram-growth words=%llu unique_trigrams=%llu load_seconds=%.3f\n",
		(unsigned long long)words,
		(unsigned long long)ngram_index->size,
		load_seconds);

	ngram_index_delete(ngram_index);
	return 0;
}
