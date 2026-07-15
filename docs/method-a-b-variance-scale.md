# Methodology A+B: uniform variance and same-API scale

## A — Uniform-only 5-run variance

```bash
make run-chaining-uniform-repeated BENCH_RUNS=5
```

Results (2026-07-15, $5\times10^5$ keys, 4096 buckets):

| Policy | Insert | Lookup |
|--------|--------|--------|
| list | 1.073 ± 0.065 | 1.003 ± 0.043 |
| hybrid-batch | 1.152 ± 0.035 | 0.059 ± 0.003 |
| hybrid-incremental | 0.166 ± 0.003 | 0.055 ± 0.002 |
| tree | 0.160 ± 0.005 | 0.053 ± 0.001 |

Stddevs are ~10× tighter than the old full-suite-interleaved repeats.

## B — Fair in-memory scale baseline

```bash
make run-chaining-scale SCALE_KEYS=1048576 SCALE_TABLE=65536
# or
./chaining_benchmark --suite scale --keys 1048576 --table-size 65536 --seed 1 --treeify 8
```

| Policy | Insert | Lookup | Avg cmp | Max chain |
|--------|--------|--------|---------|-----------|
| list | 0.890 | 0.651 | 9.0 | 36 |
| hybrid-batch | 1.082 | 0.158 | 3.7 | 36 |
| hybrid-incremental | 0.612 | 0.161 | 3.6 | 36 |
| tree | 0.620 | 0.162 | 3.6 | 36 |

Same API, RAM only, no disk. At α=16 lists stay OK; trees still win lookup; incremental wins insert vs batch.
