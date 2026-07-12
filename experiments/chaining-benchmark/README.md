# Chaining benchmark (list vs RB-tree buckets)

Microbenchmark comparing three bucket policies in the **same** C API (`hashtable/chain_ht.c`):

| Mode | Behavior |
|------|----------|
| `list` | Separate chaining with singly linked lists |
| `hybrid-k` | List inserts, then treeify buckets with `count ≥ k` (Java-style threshold; default `k=8`) |
| `tree` | RB tree in every bucket from the first insertion |

Workloads:

- **uniform** — FNV hash over random string keys (well spread across buckets)
- **skew** — keys `"BBBB:seq:suffix"` where `BBBB` is the bucket id; hash function returns the bucket id so ~all keys land in `hot-buckets` bins

## Methodology

Platform, compiler flags, wall-clock timing, datasets, metrics, and multi-run variance: [docs/benchmark-methodology.md](../../docs/benchmark-methodology.md).

```bash
./scripts/bench_repeat.sh 5 ./chaining_benchmark --suite compare \
    --mode tree --workload uniform --keys 500000 --seed 1
```

## Build and run

```bash
make chaining_benchmark

# Full article suite (500k keys, table size 4096, 8 hot buckets)
make run-chaining-compare
make run-chaining-treeify

# Single configuration
./chaining_benchmark --suite compare --mode hybrid --workload skew --treeify 8 --keys 500000
./chaining_benchmark --suite model   # theoretical list vs tree comparison counts
```

## Parameters

| Flag | Default | Meaning |
|------|---------|---------|
| `--keys` | 500000 | Number of insert+lookup operations |
| `--table-size` | 4096 | Bucket count |
| `--hot-buckets` | 8 | Skew workload: number of overloaded bins |
| `--treeify` | 8 | Hybrid treeification threshold |
| `--seed` | 1 | PRNG seed for key suffixes |

## Output

Each run prints one line to stderr:

```
experiment=chaining-benchmark workload=... mode=... insert_seconds=... search_seconds=...
  comparisons=... avg_comparisons=... max_bucket=...
```

`comparisons` counts `strcmp` calls during lookup (same metric for list and tree paths).

## Notes

- Hybrid mode builds lists during insert and calls `chain_ht_finalize()` before the lookup phase (batch treeification), analogous to Java's per-bin conversion after overload.
- The skew workload is intentionally extreme (`max_bucket ≈ keys / hot_buckets`) to show crossover versus list chaining; n-gram posting lists in the article are a real-world skewed case.
