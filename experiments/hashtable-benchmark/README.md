# Hashtable benchmark experiment

## Article context

Section **Comparing speed with other hashtable libraries** in `draft.tex`.

## Methodology

Hardware (Apple M4 Pro, 48 GB, macOS 26.5.1), compiler flags (`-O2`, no LTO), monotonic wall-clock timing, dataset sizes, and metric definitions: [docs/benchmark-methodology.md](../../docs/benchmark-methodology.md).

## What it does

1. Generates randomized string keys.
2. Inserts them into a hash table implementation.
3. Writes the keys to a data file.
4. Reads the file back and looks up every key.
5. Reports initialization and search timings.

Supported implementations:

| `--impl` | Description |
|----------|-------------|
| `gnu` | GNU libc `hsearch_r` |
| `custom` | RB-tree chained hash table from this repository |
| `tommy` | TommyDS `tommy_hashdyn` (optional build) |

## Build

```bash
make hashtable_benchmark
```

TommyDS sources are expected in `other-ht/tommyds` (see [other-ht/README.md](../../other-ht/README.md)).

## Run

```bash
./hashtable_benchmark --impl gnu
./hashtable_benchmark --impl custom
./hashtable_benchmark --impl tommy
make run-hashtable-benchmark IMPL=custom KEYS=100000
```

Run GNU, custom, and Tommy benchmarks with the same defaults:

```bash
make run-hashtable-all KEYS=33554432
```

## CLI options

| Option | Default | Description |
|--------|---------|-------------|
| `--impl NAME` | required | `gnu`, `custom`, or `tommy` |
| `--keys N` | `33554432` | Number of keys to insert |
| `--key-len N` | `10` | Random key length |
| `--data-file PATH` | `/tmp/hashtable-benchmark-keys.txt` | Key file used for lookups |
| `--seed N` | `1` | PRNG seed |

## Output

Example stderr summary:

```text
experiment=hashtable-benchmark impl=custom keys=33554432 init_seconds=... search_seconds=... size=...
```

## Article values

The article compares:

| Implementation | Initialization (s) | Search (s) |
|----------------|-------------------:|-----------:|
| GNU hsearch | 16.186 | 12.091 |
| tommy_hashdyn | 13.044 | 8.12 |
| custom implementation | 12.091 | 9.047 |

## Notes

- The full `33,554,432` key run needs substantial memory and time. Use a smaller `--keys` value for smoke tests.
- TommyDS lives in `other-ht/tommyds`; see [other-ht/README.md](../../other-ht/README.md) for the standalone `ht_tommy` benchmark.
