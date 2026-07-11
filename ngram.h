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
ngram_index_t *ngram_index_make_size(uint64_t size);
void ngram_index_delete(ngram_index_t *index);
ngram_list* ngram_get(char *seq, uint8_t n);
void ngram_list_free(ngram_list *list);
void ngram_merge(ngram_index_t *ngram_index, ngram_list *list, char *passdata);
void ngram_index_add_word(ngram_index_t *ngram_index, char *word);
void ngram_index_add_word_trigrams(ngram_index_t *ngram_index, char *word);
uint64_t ngram_index_match(ngram_index_t *ngram_index, char *query);
uint64_t ngram_index_build_from_file(ngram_index_t *ngram_index, const char *words_path,
                                     int trigrams_only, double *load_seconds);
uint64_t ngram_index_search_file(ngram_index_t *ngram_index, const char *words_path,
                                 uint64_t letters, uint64_t max_queries,
                                 uint64_t *matched, double *search_seconds);
