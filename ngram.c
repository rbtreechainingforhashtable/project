#include "common/lineio.h"
#include "ngram.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

ngram_index_t *ngram_index_make() {
	return ngram_index_make_size(128);
}

ngram_index_t *ngram_index_make_size(uint64_t size) {
	ngram_index_t *new = calloc(1, sizeof(ngram_index_t));
	new->ht = hashtable_new(size, fnv);
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

void ngram_list_free(ngram_list *list) {
	if (!list)
		return;

	for (uint64_t i = 0; i < list->size; ++i)
		free(list->ngrams[i]);
	free(list->ngrams);
	free(list);
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

static uint8_t ngram_size_for_word(size_t len) {
	if (len > 5)
		return 5;
	if (len > 4)
		return 4;
	if (len > 3)
		return 3;
	if (len > 2)
		return 2;
	return 0;
}

void ngram_index_add_word(ngram_index_t *ngram_index, char *word) {
	static const uint8_t sizes[] = {2, 3, 4, 5};
	size_t len = strlen(word);
	char *stored = strdup(word);

	for (size_t i = 0; i < sizeof(sizes); ++i) {
		if (len >= sizes[i])
			ngram_merge(ngram_index, ngram_get(word, sizes[i]), stored);
	}
}

void ngram_index_add_word_trigrams(ngram_index_t *ngram_index, char *word) {
	ngram_merge(ngram_index, ngram_get(word, 3), strdup(word));
}

static void fill_the_query(list_node *lnode, void *arg) {
	char *data = (char*)lnode->arg1;
	hashtable_t *routes_match = arg;

	node_t *routes = hashtable_get_auto(routes_match, data);
	if (!routes) {
		routes = hashtable_insert_auto(routes_match, data, NULL);
		if (!routes)
			printf("error inserting routes match %s\n", data);
	}
}

static void matched_data(node_t *node, void *arg, void *arg2, void *arg3) {
	(void)node;
	(void)arg;
	(void)arg2;
	uint64_t *matched = arg3;
	++*matched;
}

uint64_t ngram_index_match(ngram_index_t *ngram_index, char *query) {
	hashtable_t *routes_match = hashtable_new(32, fnv);
	ngram_list *list = NULL;
	uint64_t matched = 0;
	uint64_t len = strlen(query);

	uint8_t n = ngram_size_for_word(len);
	if (!n)
		return 0;

	list = ngram_get(query, n);
	for(uint64_t i = 0; i < list->size; ++i) {
		char *passkey = list->ngrams[i];
		node_t *data = hashtable_get_auto(ngram_index->ht, passkey);
		if (!data)
			continue;

		list_t *chain = data->data;
		list_foreach_node(chain, fill_the_query, routes_match);
	}

	if (routes_match->count)
		hashtable_foreach(routes_match, matched_data, &matched);

	hashtable_free(routes_match);
	ngram_list_free(list);
	return matched;
}

uint64_t ngram_index_build_from_file(ngram_index_t *ngram_index, const char *words_path,
                                     int trigrams_only, double *load_seconds) {
	FILE *fd = fopen(words_path, "r");
	uint64_t words = 0;
	struct timespec start;
	struct timespec end;
	char buf[256];

	if (!fd)
		return 0;

	clock_gettime(CLOCK_MONOTONIC, &start);
	while (fgets(buf, sizeof(buf), fd)) {
		++words;
		strip_line_ending(buf);
		if (trigrams_only)
			ngram_index_add_word_trigrams(ngram_index, buf);
		else
			ngram_index_add_word(ngram_index, buf);
	}
	fclose(fd);
	clock_gettime(CLOCK_MONOTONIC, &end);

	if (load_seconds)
		*load_seconds = (double)(end.tv_sec - start.tv_sec)
			+ (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;

	return words;
}

uint64_t ngram_index_search_file(ngram_index_t *ngram_index, const char *words_path,
                                 uint64_t letters, uint64_t max_queries,
                                 uint64_t *matched, double *search_seconds) {
	FILE *fd = fopen(words_path, "r");
	uint64_t checked = 0;
	uint64_t total_matched = 0;
	struct timespec start;
	struct timespec end;
	char buf[256];

	if (!fd)
		return 0;

	clock_gettime(CLOCK_MONOTONIC, &start);
	while (fgets(buf, sizeof(buf), fd) && max_queries) {
		size_t len = strip_line_ending(buf);
		if (len != letters)
			continue;

		buf[len] = 0;
		--max_queries;
		++checked;
		total_matched += ngram_index_match(ngram_index, buf);
	}
	fclose(fd);
	clock_gettime(CLOCK_MONOTONIC, &end);

	if (matched)
		*matched = total_matched;
	if (search_seconds)
		*search_seconds = (double)(end.tv_sec - start.tv_sec)
			+ (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;

	return checked;
}
