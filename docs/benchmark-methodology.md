# Benchmark methodology

This document defines how experiments in the repository are built, timed, and reported. The same definitions appear in Appendix~A of `draft.tex`.

## Hardware and software platform

| Item | Value |
|------|-------|
| CPU | Apple M4 Pro |
| RAM | 48 GB |
| OS | macOS 26.5.1 (build 25F80) |
| Compiler | Apple Clang 16.0.0 (`cc` from Xcode command-line tools) |
| C standard library | Apple libc / macOS system libraries |

Measurements in the paper were collected on this machine unless noted otherwise.

## Build configuration

All experiment binaries are built via the root `Makefile`:

```bash
make clean && make
```

Default flags (override with `make CFLAGS=...`):

| Flag | Value |
|------|-------|
| `CC` | `cc` (Apple Clang) |
| `CFLAGS` | `-O2 -Wall -Wextra` |
| LTO | not enabled |
| PGO | not used |

`hashtable_benchmark` additionally compiles vendored TommyDS (`other-ht/tommyds`) with `-DWITH_TOMMY` and `-I$(TOMMYDIR)`.

Record the exact compiler build when reproducing:

```bash
cc --version
make -n chaining_benchmark | head -1
```

## Timing methodology

All drivers use **`clock_gettime(CLOCK_MONOTONIC, ...)`** via `common/timing.h`.

This records **elapsed wall time** between two timestamps on a monotonic clock:

- Includes time when the process is runnable but not on-CPU (scheduler, I/O, memory allocation).
- **Not** `CLOCK_THREAD_CPUTIME_ID` and not user/system CPU time from `getrusage`.
- Immune to discontinuous jumps of the real-time clock (NTP adjustments).

Each experiment splits work into phases (e.g. **load/insert/init** vs **search/lookup**). A phase timer starts immediately before the timed loop and stops immediately after it returns. Setup outside the loop (argument parsing, opening files, allocating empty tables) is excluded unless stated in the driver README.

## Number of runs and variance

Article tables report **single-run** timings for long workloads (e.g. 500k-key skew chaining, 33M-key library comparison) where one run already takes minutes to hours.

For shorter configurations we report **mean ± sample standard deviation** over **5 independent runs**. Runs are sequential on an otherwise idle machine; the process is restarted each time so allocator and ASLR state differ between runs.

Use the helper script:

```bash
chmod +x scripts/bench_repeat.sh
./scripts/bench_repeat.sh 5 ./chaining_benchmark --suite compare \
    --mode tree --workload uniform --keys 500000 --seed 1
```

Sample standard deviation uses Bessel's correction (denominator \(n-1\)). Deterministic counters (key comparisons, `max_bucket`, match counts) are identical across runs with the same `--seed` and are reported without variance.

## Datasets and workload sizes

| Workload | Source | Size / parameters |
|----------|--------|-------------------|
| English dictionary | `vendor/english-words/words_alpha.txt` (dwyl/english-words) | 370,105 lowercase alphabetic words (~3.7 MB text) |
| N-gram growth | same dictionary | full-file scan |
| Inverted index / trie | same dictionary | default 4,000 query words per letter length; letters 3–6 in article tables |
| Hashtable libraries | PRNG-generated keys (`common/randstring.c`, `--seed`) | default \(2^{25}\) keys, 10-char suffix; written to `--data-file` then re-read for lookup phase |
| Chaining microbenchmark | PRNG suffix or synthetic skew keys | default 500,000 keys, table size 4096, 8 hot buckets (skew) |
| Quantiles | synthetic Gaussian-mixture doubles in `quantiles.c` | 111,110,000 samples total (100k + 100M + 10M + 1M + 100k per `--seed`) |

Skew chaining keys have the form `BBBB:seq:suffix` where `BBBB` is the bucket id (0–7); the skew hash function returns that id so keys cluster in `hot_buckets` bins.

## Trie versus inverted index: query models

Both drivers use the **same query strings** (first 4,000 dictionary words of length *L*), but they answer **different retrieval questions**. Do not compare raw `search_seconds` or `matched` without this context.

| Aspect | Trie (`trie.c`) | Inverted index (`inverted_index.c`) |
|--------|-----------------|-------------------------------------|
| Retrieval rule | Prefix completion: enumerate all words **starting with** query *w* | *n*-gram overlap: union posting lists for all *n*-grams of *w* |
| Typical `matched` | Low (few prefix extensions) | High (short *n*-grams collide across the dictionary) |
| Why compare | Same corpus, same query count; isolates index build cost and bucket skew on posting lists vs trie depth |

Normalize by output when judging efficiency: `search_seconds × 10⁶ / matched` (microseconds per emitted answer) is comparable across structures; aggregate matches/second is dominated by output cardinality.

## Quantiles: exact vs approximate

Global quantiles from `quantiles` are **approximate** relative to the sorted-array baseline. Per-bucket `tree_quantile` is exact among keys stored in that bucket's tree; `tree_height` may merge distinct keys. See `draft.tex` §Precision model and `experiments/quantiles/README.md`.

## Metric definitions

| Metric | Definition |
|--------|------------|
| `load_seconds` / `init_seconds` / `insert_seconds` | Monotonic elapsed seconds for the build/insert phase |
| `search_seconds` / `lookup` | Monotonic elapsed seconds for the query phase |
| `comparisons` | Count of `strcmp` calls during chained-hash lookup (`chain_ht`) |
| `avg_comparisons` | `comparisons / keys` (successful lookups, one per inserted key) |
| `max_bucket` | Maximum number of entries in any single hash bucket after inserts |
| `matched` | Total posting-list nodes visited during inverted-index or trie prefix expansion |
| `checked` | Number of dictionary words used as queries |
| `results_per_query` | `matched / checked` |
| `memory` | TommyDS `tommy_hashdyn_memory_usage` (bytes), library benchmark only |
| `q0.5`, `q0.9`, `q0.99` | Global quantiles from ordered-bucket hash table (approximate); `baseline_q*` from full sort (exact reference) when `--baseline` is set |

## Experiment drivers

| Binary | Makefile target | README |
|--------|-----------------|--------|
| `ngram_calculation` | `run-ngram-growth` | `experiments/ngram-growth/README.md` |
| `inverted_index` | `run-inverted-index` | `experiments/inverted-index/README.md` |
| `trie` | `run-trie` | `experiments/trie/README.md` |
| `hashtable_benchmark` | `run-hashtable-benchmark` | `experiments/hashtable-benchmark/README.md` |
| `chaining_benchmark` | `run-chaining-compare` | `experiments/chaining-benchmark/README.md` |
| `quantiles` | `run-quantiles` | `experiments/quantiles/README.md` |

Each driver prints one machine-parseable `experiment=...` line to **stderr** per run.
