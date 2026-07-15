CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra
TOMMYDIR ?= other-ht/tommyds/tommyds
TOMMY_CFLAGS = -I$(TOMMYDIR) -include string.h -include $(TOMMYDIR)/tommylist.h

HASHTABLE_SRC = hashtable/hashtable.c hashtable/rbtree.c hashtable/list.c
NGRAM_SRC = ngram.c $(HASHTABLE_SRC)
COMMON_SRC = common/randstring.c

BINARIES = ngram_calculation inverted_index trie hashtable_benchmark quantiles chaining_benchmark inverted_chain_bench

CHAINING_SRC = hashtable/chain_ht.c hashtable/hashtable.c hashtable/rbtree.c hashtable/list.c

.PHONY: all clean help spe-unified spe-split \
	run-ngram-growth run-inverted-index run-trie run-hashtable-benchmark run-quantiles \
	run-chaining-compare run-chaining-treeify run-chaining-compare-repeated \
	run-chaining-policy run-inverted-chain-bench run-chaining-uniform-repeated \
	run-chaining-scale \
	run-inverted-index-all run-trie-all run-hashtable-all run-quantiles-all

all: $(BINARIES)

spe-unified:
	python3 scripts/build_spe_unified.py merge

spe-split:
	python3 scripts/build_spe_unified.py split

help:
	@echo "Build all experiments:"
	@echo "  make"
	@echo ""
	@echo "Build one experiment:"
	@echo "  make ngram_calculation"
	@echo "  make inverted_index"
	@echo "  make trie"
	@echo "  make hashtable_benchmark"
	@echo "  make chaining_benchmark"
	@echo ""
	@echo "Run one experiment:"
	@echo "  make run-ngram-growth"
	@echo "  make run-inverted-index LETTERS=3 QUERIES=4000"
	@echo "  make run-trie LETTERS=3 QUERIES=4000"
	@echo "  make run-hashtable-benchmark IMPL=custom KEYS=100000"
	@echo "  make run-quantiles SIZE=1024 TREE_HEIGHT=5"
	@echo ""
	@echo "Run article benchmark suites:"
	@echo "  make run-inverted-index-all"
	@echo "  make run-trie-all"
	@echo "  make run-hashtable-all"
	@echo "  make run-quantiles-all"
	@echo ""
	@echo "TommyDS support (local clone in other-ht/tommyds):"
	@echo "  make hashtable_benchmark"
	@echo "  make run-hashtable-benchmark IMPL=tommy KEYS=100000"
	@echo "  make run-chaining-compare CHAIN_KEYS=500000"
	@echo "  make run-chaining-compare-repeated BENCH_RUNS=5 CHAIN_KEYS=500000"
	@echo "  make run-chaining-treeify CHAIN_KEYS=500000"
	@echo "  make run-chaining-policy CHAIN_KEYS=500000   # batch vs incremental hybrid"
	@echo "  make run-chaining-uniform-repeated BENCH_RUNS=5  # uniform-only variance"
	@echo "  make run-chaining-scale SCALE_KEYS=1048576     # fair in-memory same-API baseline"
	@echo "  make run-inverted-chain-bench MAX_NGRAMS=256   # real posting-list replay"
	@echo "  make spe-unified   # merge split .tex -> spe-manuscript.tex"
	@echo "  make spe-split     # split spe-manuscript.tex -> draft-spe.tex + spe-body.tex + ..."

ngram_calculation: ngram_calculation.c common/lineio.c $(NGRAM_SRC)
	$(CC) $(CFLAGS) -Ihashtable -Icommon ngram_calculation.c common/lineio.c $(NGRAM_SRC) -o $@

inverted_index: inverted_index.c common/lineio.c $(NGRAM_SRC)
	$(CC) $(CFLAGS) -Ihashtable -Icommon inverted_index.c common/lineio.c $(NGRAM_SRC) -o $@

trie: trie.c common/lineio.c
	$(CC) $(CFLAGS) -Icommon trie.c common/lineio.c -o $@

hashtable_benchmark: hashtable_benchmark.c $(HASHTABLE_SRC) $(COMMON_SRC)
	$(CC) $(CFLAGS) -Ihashtable -Icommon hashtable_benchmark.c $(HASHTABLE_SRC) $(COMMON_SRC) \
		$(TOMMY_CFLAGS) -DWITH_TOMMY $(TOMMYDIR)/tommy.c \
		-o $@

quantiles: quantiles.c qtree/qtree.c
	$(CC) $(CFLAGS) -lm quantiles.c qtree/qtree.c -o $@

chaining_benchmark: chaining_benchmark.c $(CHAINING_SRC) $(COMMON_SRC)
	$(CC) $(CFLAGS) -Ihashtable -Icommon chaining_benchmark.c $(CHAINING_SRC) $(COMMON_SRC) -o $@

inverted_chain_bench: inverted_chain_bench.c ngram.c common/lineio.c $(CHAINING_SRC)
	$(CC) $(CFLAGS) -Ihashtable -Icommon inverted_chain_bench.c ngram.c common/lineio.c \
		$(CHAINING_SRC) -o $@

WORDS ?= vendor/english-words/words_alpha.txt
LETTERS ?= 3
QUERIES ?= 4000
IMPL ?= custom
KEYS ?= 33554432
KEY_LEN ?= 10
DATA_FILE ?= /tmp/hashtable-benchmark-keys.txt
SIZE ?= 1024
TREE_HEIGHT ?= 5
SEED ?= 1
CHAIN_KEYS ?= 500000
CHAIN_TABLE ?= 4096
CHAIN_HOT ?= 8
BENCH_RUNS ?= 5

run-chaining-compare: chaining_benchmark
	./chaining_benchmark --suite compare --keys $(CHAIN_KEYS) --table-size $(CHAIN_TABLE) --hot-buckets $(CHAIN_HOT) --seed $(SEED)

run-chaining-compare-repeated: chaining_benchmark
	./scripts/bench_repeat.sh $(BENCH_RUNS) ./chaining_benchmark --suite compare \
		--keys $(CHAIN_KEYS) --table-size $(CHAIN_TABLE) --hot-buckets $(CHAIN_HOT) --seed $(SEED)

# Uniform-only variance: no skew between repetitions (avoids confounding).
run-chaining-uniform-repeated: chaining_benchmark
	@echo "=== uniform list ==="
	./scripts/bench_repeat.sh $(BENCH_RUNS) ./chaining_benchmark --suite compare \
		--mode list --workload uniform --keys $(CHAIN_KEYS) --table-size $(CHAIN_TABLE) \
		--hot-buckets $(CHAIN_HOT) --seed $(SEED)
	@echo "=== uniform hybrid-batch ==="
	./scripts/bench_repeat.sh $(BENCH_RUNS) ./chaining_benchmark --suite compare \
		--mode hybrid --workload uniform --policy batch --treeify 8 \
		--keys $(CHAIN_KEYS) --table-size $(CHAIN_TABLE) --hot-buckets $(CHAIN_HOT) --seed $(SEED)
	@echo "=== uniform hybrid-incremental ==="
	./scripts/bench_repeat.sh $(BENCH_RUNS) ./chaining_benchmark --suite compare \
		--mode hybrid --workload uniform --policy incremental --treeify 8 \
		--keys $(CHAIN_KEYS) --table-size $(CHAIN_TABLE) --hot-buckets $(CHAIN_HOT) --seed $(SEED)
	@echo "=== uniform tree ==="
	./scripts/bench_repeat.sh $(BENCH_RUNS) ./chaining_benchmark --suite compare \
		--mode tree --workload uniform --keys $(CHAIN_KEYS) --table-size $(CHAIN_TABLE) \
		--hot-buckets $(CHAIN_HOT) --seed $(SEED)

run-chaining-scale: chaining_benchmark
	./chaining_benchmark --suite scale --keys $(SCALE_KEYS) --table-size $(SCALE_TABLE) --seed $(SEED)

SCALE_KEYS ?= 1048576
SCALE_TABLE ?= 65536

run-chaining-treeify: chaining_benchmark
	./chaining_benchmark --suite treeify --keys $(CHAIN_KEYS) --table-size $(CHAIN_TABLE) --hot-buckets $(CHAIN_HOT) --seed $(SEED)

run-chaining-policy: chaining_benchmark
	./chaining_benchmark --suite policy --keys $(CHAIN_KEYS) --table-size $(CHAIN_TABLE) \
		--hot-buckets $(CHAIN_HOT) --seed $(SEED) --treeify 8

run-inverted-chain-bench: inverted_chain_bench
	./inverted_chain_bench --words $(WORDS) --max-ngrams $(MAX_NGRAMS)

MAX_NGRAMS ?= 2048

run-ngram-growth: ngram_calculation
	./ngram_calculation --words $(WORDS)

run-inverted-index: inverted_index
	./inverted_index --words $(WORDS) --letters $(LETTERS) --queries $(QUERIES)

run-inverted-index-all: inverted_index
	@for letters in 3 4 5 6; do \
		./inverted_index --words $(WORDS) --letters $$letters --queries $(QUERIES); \
	done

run-trie: trie
	./trie --words $(WORDS) --letters $(LETTERS) --queries $(QUERIES)

run-trie-all: trie
	./trie --words $(WORDS) --all --queries $(QUERIES)

run-hashtable-benchmark: hashtable_benchmark
	./hashtable_benchmark --impl $(IMPL) --keys $(KEYS) --key-len $(KEY_LEN) --data-file $(DATA_FILE) --seed $(SEED)

run-hashtable-all: hashtable_benchmark
	@for impl in gnu custom tommy; do \
		./hashtable_benchmark --impl $$impl --keys $(KEYS) --key-len $(KEY_LEN) --data-file $(DATA_FILE) --seed $(SEED); \
	done

run-quantiles: quantiles
	./quantiles --size $(SIZE) --tree-height $(TREE_HEIGHT) --seed $(SEED)

run-quantiles-all: quantiles
	@for cfg in "1024 4" "4096 4" "4096 5" "16384 5" "16384 6"; do \
		set -- $$cfg; \
		./quantiles --size $$1 --tree-height $$2 --baseline --seed $(SEED); \
	done

clean:
	rm -f $(BINARIES)
