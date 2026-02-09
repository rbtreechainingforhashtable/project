#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include <time.h>
#include "hashtable.h"
#include "list.h"
#include "ngram.h"

void suggests_test(int argc, char **argv) {
	char *words_path = "vendor/english-words/words_alpha.txt";
	if (argc > 1)
		words_path = argv[1];

	ngram_index_t *ngram_index = ngram_index_make();

	FILE *fd = fopen(words_path, "r");
	if (!fd)
		return;

	struct timespec pre;
	clock_gettime(CLOCK_REALTIME, &pre);
	char buf[256];
	uint64_t words = 0;
	uint64_t step = 1;
	uint64_t transition_point = 10;
	while (fgets(buf, 256, fd)) {
		++words;
		buf[strlen(buf)-1] = 0;
		ngram_merge(ngram_index, ngram_get(buf, 3), strdup(buf));
		if (words <= transition_point && (words % step) == 0)
			printf("words: %llu, ngrams: %llu: %s\n", words, ngram_index->size, buf);
		if (words == transition_point && (step = transition_point, transition_point *= 10)) {}
	}
	fclose(fd);
	struct timespec loaded;
	clock_gettime(CLOCK_REALTIME, &loaded);

	ngram_index_delete(ngram_index);
}

int main(int argc, char **argv) {
	suggests_test(argc, argv);
}
