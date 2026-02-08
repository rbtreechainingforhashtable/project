trigram-calculation:
	cc -I hashtable hashtable/hashtable.c hashtable/rbtree.c hashtable/list.c ngram_calculation.c -o ngram_calculation

all:
	trigram-calculation
