# Finding A: Batch vs incremental hybrid treeification

## Claim

Copying Java’s `TREEIFY_THRESHOLD=8` into C is incomplete without choosing *when* conversion happens:

| Policy | When lists become trees |
|--------|-------------------------|
| **Batch** (`finalize` after load) | Once, after all inserts |
| **Incremental** (Java-like) | As soon as a bin reaches threshold during insert |

## Why it matters

Under **forced-bucket chaining stress** (few overloaded bins; hash held fixed), batch hybrid pays full linked-list insert cost until the end, then converts. Incremental hybrid converts early and subsequent inserts are $O(\log N)$ into trees—close to always-tree. This stresses *chaining internals*, not Zipf hash quality over the full table; empirical long lengths come from the posting-list CDF/replay.

Under **interleaved lookups**, mid-load probes still walk long lists with batch hybrid; incremental already has trees.

## Reproduce

```bash
make chaining_benchmark

# Skew insert / search (hero comparison)
./chaining_benchmark --suite compare --mode hybrid --workload skew \
  --keys 500000 --seed 1 --treeify 8 --policy batch
./chaining_benchmark --suite compare --mode hybrid --workload skew \
  --keys 500000 --seed 1 --treeify 8 --policy incremental

# Mid-load probes during skew insert
./chaining_benchmark --suite compare --mode hybrid --workload skew \
  --keys 500000 --seed 1 --treeify 8 --policy batch \
  --mix-every 10000 --mix-probes 64
./chaining_benchmark --suite compare --mode hybrid --workload skew \
  --keys 500000 --seed 1 --treeify 8 --policy incremental \
  --mix-every 10000 --mix-probes 64

# Full suite
make run-chaining-policy
```

## Smoke numbers (50k keys, skew, 2026-07-15)

| Mode | Insert (s) | Mid comparisons (`mix-every=5000`) |
|------|------------|--------------------------------------|
| hybrid-8 batch | 0.383 | 1,166,720 |
| hybrid-8 incremental | 0.015 | 7,017 |
| always-tree | 0.016 | — |

Final search comparisons are nearly identical once conversion completes; the gap is **load-phase** behaviour.

## Full numbers (500k keys, skew + uniform, 2026-07-15)

| Workload | Mode | Insert (s) | Lookup (s) | Mid cmp |
|----------|------|------------|------------|---------|
| Uniform | list | 0.836 | 0.808 | — |
| Uniform | hybrid-8 batch | 0.913 | 0.060 | — |
| Uniform | hybrid-8 incremental | 0.143 | 0.055 | — |
| Uniform | tree | 0.142 | 0.051 | — |
| Skew | hybrid-8 batch | **121.156** | 0.040 | — |
| Skew | hybrid-8 incremental | **0.201** | 0.044 | — |
| Skew | tree | 0.196 | 0.043 | — |
| Skew + mix | hybrid-8 batch | 93.1 | 0.040 | **36,898,600** |
| Skew + mix | hybrid-8 incremental | 0.187 | 0.042 | **46,275** |

Hero ratio (skew insert): batch / incremental ≈ **603×**.
