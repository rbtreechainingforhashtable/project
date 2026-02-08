#include "ngram.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


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
