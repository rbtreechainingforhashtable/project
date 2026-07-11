# Quantiles experiment

## Article context

Section **Quantile calculating with ordered hash table** in `draft.tex`.

## What it does

1. Generates a skewed synthetic workload of floating-point samples.
2. Inserts samples into an ordered RB-tree chained hash table.
3. Calculates quantiles `q(0.5)`, `q(0.9)`, and `q(0.99)`.
4. Optionally compares against a sorted-array baseline with `--baseline`.

## Build

```bash
make quantiles
```

## Run

```bash
./quantiles
./quantiles --size 1024 --tree-height 5
./quantiles --size 4096 --tree-height 5 --baseline
make run-quantiles SIZE=1024 TREE_HEIGHT=4
```

Run the article configuration set:

```bash
make run-quantiles-all
```

## CLI options

| Option | Default | Description |
|--------|---------|-------------|
| `--size N` | `1024` | Number of hash-table buckets |
| `--tree-height N` | `5` | Maximum RB-tree height per bucket |
| `--baseline` | off | Also run sorted-array baseline |
| `--seed N` | `1` | PRNG seed |

## Output

Example stderr summary:

```text
experiment=quantiles size=1024 tree_height=5 insert_seconds=... search_seconds=... tree_insert_seconds=... q0.5=... q0.9=... q0.99=...
```

With `--baseline`, the output also includes:

```text
baseline_insert_seconds=... baseline_search_seconds=... baseline_q0.5=... baseline_q0.9=... baseline_q0.99=...
```

## Article values

The article reports several hash-table sizes and tree heights:

| Hashtable size | Tree height | Insert (s) | Search (s) |
|----------------:|------------:|-----------:|-----------:|
| 1024 | 4 | 8.079 | ~0 |
| 4096 | 4 | 8.571 | ~0 |
| 4096 | 5 | 10.337 | ~0 |
| 16384 | 5 | 13.347 | ~0 |
| 16384 | 6 | 17.156 | ~0 |

The sorted-array baseline in the article takes about `1.758 s` to insert and `22.18 s` to search.

## Notes

- The default workload inserts about `111,100,000` samples. A full run is CPU- and memory-intensive.
- Use `make run-quantiles-all` to reproduce the article's parameter sweep with baseline comparison enabled.
