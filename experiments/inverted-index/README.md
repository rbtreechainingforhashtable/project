# Inverted index search experiment

## Article context

Section **Chaining methods testing** in `draft.tex`, especially the comparison table between the inverted index and trie.

## Methodology

See [docs/benchmark-methodology.md](../../docs/benchmark-methodology.md) (platform, `-O2` build, wall-clock phases, dictionary size 370,105 words).

## What it does

1. Builds an inverted index from the dictionary using `2/3/4/5-grams`.
2. Selects words of a fixed length from the same dictionary.
3. For each query word $w$, unions posting lists for all $n$-grams of $w$ and counts distinct matching words ($n$-gram overlap retrieval).
4. Reports load time, search time, emitted match count, and matches per query.

**Query model:** this is *not* prefix matching. A 3-letter query uses bigrams, so posting lists are large and `matched` counts are orders of magnitude higher than a trie prefix completion on the same query strings. See Section *Query models and comparison scope* in `draft.tex` and [../trie/README.md](../trie/README.md).

## Build

```bash
make inverted_index
```

## Run

```bash
./inverted_index
./inverted_index --letters 3 --queries 4000
./inverted_index --words vendor/english-words/words_alpha.txt --letters 6 --queries 4000
make run-inverted-index LETTERS=4 QUERIES=4000
```

Run the article comparison set for word lengths `3..6`:

```bash
make run-inverted-index-all
```

## CLI options

| Option | Default | Description |
|--------|---------|-------------|
| `--words PATH` | `vendor/english-words/words_alpha.txt` | Dictionary file |
| `--letters N` | `3` | Word length to benchmark |
| `--queries N` | `4000` | Number of lookup queries |

## Output

Example stderr summary:

```text
experiment=inverted-index words=... unique_ngrams=... load_seconds=... letters=3 checked=4000 matched=... search_seconds=... results_per_query=...
```

## Article values

The article reports inverted-index load time around `3.262 s` and search timings for `4000` queries at lengths `3..6`.

## Related experiment

Compare with the trie benchmark in [../trie/README.md](../trie/README.md).
