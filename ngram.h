#pragma once
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

ngram_index_t *ngram_index_make();
void ngram_index_delete(ngram_index_t *index);
ngram_list* ngram_get(char *seq, uint8_t n);
void ngram_merge(ngram_index_t *ngram_index, ngram_list *list, char *passdata);
