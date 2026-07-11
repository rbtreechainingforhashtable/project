# N-gram growth experiment

## Article context

Section **Chaining methods testing / Natural language processing with hash table in inverted index** in `draft.tex`.

This experiment measures how the number of unique trigrams grows while loading an English dictionary into an inverted index backed by an RB-tree chained hash table.

## What it does

1. Reads words from `vendor/english-words/words_alpha.txt`.
2. Splits each word into trigrams (`n = 3`).
3. Stores each trigram in the hash table and chains matching words.
4. Prints `(words, unique_trigrams)` checkpoints while loading.
5. Reports total load time on stderr.

## Build

```bash
make ngram_calculation
```

## Run

```bash
./ngram_calculation
./ngram_calculation --words vendor/english-words/words_alpha.txt
make run-ngram-growth
```

## Output

Stdout contains growth checkpoints such as:

```text
words: 10, ngrams: 28: ...
```

Stderr contains a summary line:

```text
experiment=ngram-growth words=... unique_trigrams=... load_seconds=...
```

## Notes

- This experiment indexes only trigrams. It is used for the growth charts in the article.
- The inverted-index search experiment uses multi-gram indexing (`2/3/4/5-grams`) and is implemented separately in `inverted_index`.
