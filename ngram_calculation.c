#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <mach/clock.h>
#include <mach/mach.h>
#include <time.h>
#include "hashtable.h"
#include "list.h"

typedef struct ngram_list {
	char **ngrams;
	uint64_t size;
} ngram_list;

typedef struct ngram_index_t {
	hashtable_t *ht;
	uint64_t size;
} ngram_index_t;


ngram_index_t *ngram_index_make() {
	ngram_index_t *new = calloc(1, sizeof(ngram_index_t));
	new->ht = hashtable_new(128, fnv);
	return new;
}

void ngram_index_delete(ngram_index_t *index) {
	hashtable_free(index->ht);
	free(index);
}

ngram_list* ngram_get(char *seq, uint8_t n) {
	uint64_t i;
	ngram_list *list = calloc(1, sizeof(*list));

	if (!seq)
		return list;

	uint64_t len = strlen(seq);
	if (len < n)
		return list;

	for (i = 0; seq[i+n-1]; ++i);
	list->ngrams = calloc(1, sizeof(char*)*i);

	for (i = 0; seq[i+n-1]; ++i) {
		list->ngrams[i] = strndup(seq+i, n);
	}
	list->size = i;

	return list;
}

void ngram_merge(ngram_index_t *ngram_index, ngram_list *list, char *passdata) {
	hashtable_t *ht = ngram_index->ht;
	for(uint64_t i = 0; i < list->size; ++i) {
		char *passkey = list->ngrams[i];
		node_t *data = hashtable_get_auto(ht, passkey);
		if (!data)
		{
			++ngram_index->size;
			list_t *list = list_new();
			list_add(list, passdata, NULL);
			data = hashtable_insert_auto(ht, passkey, list);
			if (!data) {
				printf("error inserting passkey %s\n", passkey);
			}
		}
		else {
			list_add(data->data, passdata, NULL);
		}
	}
}

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

	uint64_t maxnum = 1000 * 4;

	ngram_index_delete(ngram_index);
}

int main(int argc, char **argv) {
	suggests_test(argc, argv);
}
