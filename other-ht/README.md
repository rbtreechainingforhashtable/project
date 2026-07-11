# Other hash table implementations

This directory contains third-party hash table code and a standalone TommyDS benchmark used in the article section **Comparing speed with other hashtable libraries**.

## Layout

| Path | Description |
|------|-------------|
| `tommyds/` | Cloned [TommyDS](https://www.tommyds.it) repository |
| `ht_tommy.c` | Standalone benchmark for `tommy_hashdyn` |
| `Makefile` | Build helper for `ht_tommy` |

The main integrated benchmark lives in the project root as `hashtable_benchmark` and can use the same TommyDS sources from here.

## TommyDS

TommyDS is an external C library with several hash table variants. This project uses `tommy_hashdyn`, a dynamic chained hash table.

Upstream documentation:

- `tommyds/README`
- `tommyds/doc/index.html`
- https://www.tommyds.it

If the clone is missing, initialize it from the project root:

```bash
git clone https://github.com/mht37/tommyds.git other-ht/tommyds
```

## Standalone Tommy benchmark

`ht_tommy.c` is the original article benchmark for TommyDS. It:

1. Inserts randomized 10-character keys into `tommy_hashdyn`
2. Writes keys to `newdata.txt`
3. Reads the file back and looks up every key
4. Prints initialization and search timings

### Build

```bash
cd other-ht
make
```

### Run

```bash
cd other-ht
./ht_tommy
```

By default the program uses `33,554,431` keys, same order of magnitude as the article table. For a quick smoke test, edit `sz` in `ht_tommy.c` or use the integrated benchmark below.

### Output

```text
init 13.044 seconds
hash table size is ..., memory ...
read 8.120 seconds
```

## Integrated benchmark (recommended)

The root `hashtable_benchmark` binary runs GNU, custom, and Tommy implementations with the same CLI:

```bash
# from project root
make hashtable_benchmark
make run-hashtable-benchmark IMPL=tommy KEYS=100000

# full article-sized run
make run-hashtable-benchmark IMPL=tommy KEYS=33554432
```

With the local clone in place, `make` picks up `other-ht/tommyds/tommyds` automatically.

Override the TommyDS path if needed:

```bash
make hashtable_benchmark TOMMYDIR=/path/to/tommyds/tommyds
```

## Article reference

From `draft.tex`:

| Implementation | Initialization (s) | Search (s) |
|----------------|-------------------:|-----------:|
| GNU hsearch | 16.186 | 12.091 |
| tommy_hashdyn | 13.044 | 8.12 |
| custom RB-tree chained hash table | 12.091 | 9.047 |

See also: [experiments/hashtable-benchmark/README.md](../experiments/hashtable-benchmark/README.md)

## Notes

- `newdata.txt` is created in the current working directory when `ht_tommy` runs.
- TommyDS is maintained separately from this repository; treat `other-ht/tommyds` as a vendored dependency.
- The root `hashtable_benchmark --impl tommy` is the preferred way to compare all three implementations with the same options.
