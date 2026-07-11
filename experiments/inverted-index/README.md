# Inverted index search experiment

## Article context

Section **Chaining methods testing** in `draft.tex`, especially the comparison table between the inverted index and trie.

## What it does

1. Builds an inverted index from the dictionary using `2/3/4/5-grams`.
2. Selects words of a fixed length from the same dictionary.
3. Runs substring-style lookups through the inverted index.
4. Reports load time, search time, matched node count, and results per query.

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
