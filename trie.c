#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <mach/clock.h>
#include <mach/mach.h>
#include <time.h>

#define ALPHABET_SIZE 26
typedef struct trie_t {
    struct trie_t *children[ALPHABET_SIZE];
    uint8_t terminate;
} trie_t;

void trie_insert(trie_t * trie, char *key, unsigned key_len) {
    if (!key_len) {
        trie->terminate = 1;
        return;
    }

    const unsigned int index = key[0] - 'a';
    if (ALPHABET_SIZE <= index) {
        return;
    }

    if (NULL == trie->children[index]) {
        trie->children[index] = calloc(1, sizeof(trie_t));
    }
    
    trie_insert(trie->children[index], key + 1, key_len - 1);
}


void trie_search(trie_t * trie, char *key, unsigned key_len, trie_t ** result) {
    if (!key_len) {
        *result = trie;
        return;
    }

    const unsigned int index = key[0] - 'a';
    if (ALPHABET_SIZE <= index) {
        return;
    }

    if (!trie->children[index]) {
        return;
    }

    trie_search(trie->children[index], key + 1, key_len - 1, result);
}

void trie_print (trie_t * trie, char prefix[], unsigned prefix_len, uint64_t *matched)
{
    if (!trie)
        return;

    if (trie->terminate) {
        ++*matched;
        printf("\t%.*s\n", prefix_len, prefix);
    }

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (NULL == trie->children[i]) {
            continue;
        }

        prefix[prefix_len] = i + 'a';
        trie_print(trie->children[i], prefix, prefix_len + 1, matched);
    }
}

void check_trie(char *words_path, trie_t *root, uint64_t letters, uint64_t maxnum)  {
    struct timespec checking;
    clock_gettime(CLOCK_REALTIME, &checking);
    char key[256];
    uint64_t matched = 0;
    uint64_t resulted = 0;

    FILE *fd = fopen(words_path, "r");
    while (fgets(key, 256, fd) && maxnum) {
        size_t key_len = strlen(key)-1;
        if (key_len != letters)
            continue;

        --maxnum;
        ++resulted;
        key[key_len] = 0;
        trie_t *trie = NULL;
        trie_search(root, key, key_len, &trie);
        trie_print(trie, key, key_len, &matched);
    }
    fclose(fd);

    struct timespec checked;
    clock_gettime(CLOCK_REALTIME, &checked);
    fprintf(stderr, "token %llu checks takes %ld.%03lu seconds, maxnum %llu, checked: %llu, matched %llu, results per query: %lf\n", letters, checked.tv_sec - checking.tv_sec, ((checked.tv_nsec-checking.tv_nsec)/1000000), maxnum, resulted, matched, ((matched * 1.0) / resulted));
}

int main(int argc, char **argv) {
	char *words_path = "vendor/english-words/words_alpha.txt";
	if (argc > 1)
		words_path = argv[1];

    trie_t *root = calloc(1, sizeof(trie_t));
    char key[256];

    struct timespec pre;
    clock_gettime(CLOCK_REALTIME, &pre);
    FILE *fd = fopen(words_path, "r");
    while (fgets(key, 256, fd)) {
        size_t key_len = strlen(key)-1;
        key[key_len] = 0;
        trie_insert(root, key, key_len);
    }
    fclose(fd);
    struct timespec loaded;
    clock_gettime(CLOCK_REALTIME, &loaded);
    fprintf(stderr, "trie load took %ld.%03lu seconds\n", loaded.tv_sec - pre.tv_sec, ((loaded.tv_nsec-pre.tv_nsec)/1000000));

    uint64_t maxnum = 1000 * 4;
    check_trie(words_path, root, 3, maxnum);
    check_trie(words_path, root, 4, maxnum);
    check_trie(words_path, root, 5, maxnum);
    check_trie(words_path, root, 6, maxnum);
    check_trie(words_path, root, 7, maxnum);
}
