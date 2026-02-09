all: ngram_calculation trie

ngram_calculation:
	cc -I hashtable ngram.c hashtable/hashtable.c hashtable/rbtree.c hashtable/list.c ngram_calculation.c -o ngram_calculation

trie:
	cc -I hashtable trie.c -o trie

clean:
	rm -f trie ngram_calculation
