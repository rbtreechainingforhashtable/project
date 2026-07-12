# Trie prefix search experiment

## Article context

Section **Chaining methods testing** in `draft.tex`, especially the comparison table between trie and inverted index.

## Methodology

See [docs/benchmark-methodology.md](../../docs/benchmark-methodology.md).

## What it does

1. Loads all dictionary words into a simple prefix trie.
2. Selects words of a fixed length from the dictionary (same 4,000-query workload as `inverted_index`).
3. For each query word $w$, navigates to prefix $w$ and enumerates every dictionary word that **starts with** $w$ (prefix completion).
4. Reports trie load time, search time, emitted match count, and matches per query.

**Query model:** prefix completion, not $n$-gram overlap. Match counts are much smaller than the inverted index on the same query strings because the trie returns extensions of $w$, not every word sharing a bigram with $w$. See `draft.tex` Section *Query models and comparison scope*.

## Build

```bash
make trie
```

## Run

```bash
./trie
./trie --letters 3 --queries 4000
./trie --words vendor/english-words/words_alpha.txt --letters 6 --queries 4000
make run-trie LETTERS=5 QUERIES=4000
```

Run the article comparison set for word lengths `3..7`:

```bash
./trie --all --queries 4000
make run-trie-all
```

## CLI options

| Option | Default | Description |
|--------|---------|-------------|
| `--words PATH` | `vendor/english-words/words_alpha.txt` | Dictionary file |
| `--letters N` | `3` | Word length to benchmark |
| `--queries N` | `4000` | Number of lookup queries |
| `--all` | off | Run benchmarks for letters `3..7` |

## Output

Example stderr summary:

```text
experiment=trie load_seconds=...
experiment=trie letters=3 checked=4000 matched=... search_seconds=... results_per_query=...
```

## Article values

The article reports trie load time around `0.077 s` and much lower search times than the inverted index for the same query set.

## Related experiment

Compare with the inverted-index benchmark in [../inverted-index/README.md](../inverted-index/README.md).
