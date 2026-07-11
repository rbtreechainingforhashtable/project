# RB-tree Chained Hash Table Experiments

Code and benchmarks for the article *Using RB-tree chaining within a hash table*

## Experiments

| Experiment | Binary | Article section | README |
|------------|--------|-----------------|--------|
| N-gram growth | `ngram_calculation` | Chaining methods testing / inverted index | [experiments/ngram-growth/README.md](experiments/ngram-growth/README.md) |
| Inverted index search | `inverted_index` | Trie vs inverted index comparison | [experiments/inverted-index/README.md](experiments/inverted-index/README.md) |
| Trie prefix search | `trie` | Trie vs inverted index comparison | [experiments/trie/README.md](experiments/trie/README.md) |
| Hashtable libraries | `hashtable_benchmark` | Comparing speed with other hashtable libraries | [experiments/hashtable-benchmark/README.md](experiments/hashtable-benchmark/README.md) |
| Quantiles | `quantiles` | Quantile calculating with ordered hash table | [experiments/quantiles/README.md](experiments/quantiles/README.md) |

## Quick start

```bash
make
make help
```

Build and run a single experiment:

```bash
make ngram_calculation
make run-ngram-growth

make inverted_index
make run-inverted-index LETTERS=4 QUERIES=4000

make trie
make run-trie LETTERS=3

make hashtable_benchmark
make run-hashtable-benchmark IMPL=custom KEYS=100000

make quantiles
make run-quantiles SIZE=1024 TREE_HEIGHT=5
```

Run the benchmark suites used in the article tables:

```bash
make run-inverted-index-all
make run-trie-all
make run-hashtable-all
make run-quantiles-all
```

## Dictionary data

Most NLP experiments use `vendor/english-words/words_alpha.txt`.

## TommyDS support

TommyDS is vendored under `other-ht/tommyds`. See [other-ht/README.md](other-ht/README.md).

```bash
make hashtable_benchmark
make run-hashtable-benchmark IMPL=tommy KEYS=100000
make run-hashtable-all KEYS=33554432
```

Standalone Tommy benchmark:

```bash
cd other-ht && make && ./ht_tommy
```
