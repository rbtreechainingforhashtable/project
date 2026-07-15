# Inverted-index → chain_ht transfer experiment

## What it does

1. Builds a **trigram** inverted index on `words_alpha.txt`.
2. Reports posting-list length CDF.
3. Replays the hottest `N` posting lengths into `chain_ht` as chaining-internal
   chains (one bucket per trigram; chain length = posting length)—transfers
   *empirical lengths*, not an FNV Zipf hash mix.
4. Compares **list / hybrid-batch / hybrid-incremental / tree**.

## Reproduce

```bash
make inverted_chain_bench
./inverted_chain_bench --max-ngrams 256
# or
make run-inverted-chain-bench MAX_NGRAMS=2048
```

## Numbers (2026-07-15, Apple M4 Pro)

### CDF (all trigrams)

| Unique | Total postings | p50 | p90 | p99 | max | mean |
|--------|----------------|-----|-----|-----|-----|------|
| 9165 | 2,754,516 | 29 | 793 | 4002 | 23501 | 300.5 |

### Hottest 256 replay (1,065,810 keys) — in paper Table `tab:posting-replay`

| Mode | Insert (s) | Lookup (s) | Avg cmp |
|------|------------|------------|---------|
| list | 6.617 | 6.573 | 2880.7 |
| hybrid-8 batch | 7.347 | 0.082 | 11.7 |
| hybrid-8 incremental | 0.223 | 0.080 | 11.7 |
| tree | 0.224 | 0.077 | 11.7 |

### Hottest 2048 replay (2,462,176 keys) — confirms same pattern

| Mode | Insert (s) | Lookup (s) | Avg cmp |
|------|------------|------------|---------|
| list | 8.261 | 8.170 | 1549.6 |
| hybrid-8 batch | 9.226 | 0.176 | 10.4 |
| hybrid-8 incremental | 0.473 | 0.167 | 10.4 |
| tree | 0.467 | 0.161 | 10.4 |

Finding A transfers: batch insert ≫ incremental ≈ tree; final lookup comparisons converge once treeified.
