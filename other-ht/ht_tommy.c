#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tommy.h"
#include "../common/lineio.h"
#include "../common/randstring.h"
#include "../common/timing.h"

typedef struct tnode {
	char *key;
	tommy_node node;
} tnode;

static int tommy_check(const void *arg, const void *obj) {
	return strcmp((const char *)arg, ((const tnode *)obj)->key);
}

static void print_elapsed(const char *label,
                          const struct timespec *start,
                          const struct timespec *end) {
	fprintf(stderr, "%s %.3f seconds\n", label, timespec_elapsed_sec(start, end));
}

int main(void) {
	struct timespec init;
	struct timespec inited;
	struct timespec read;
	struct timespec readed;
	tommy_hashdyn hashdyn;
	FILE *fd;
	char buf[256];
	uint64_t sz = 33554431;

	tommy_hashdyn_init(&hashdyn);

	clock_gettime(CLOCK_MONOTONIC, &init);
	fd = fopen("newdata.txt", "w");
	if (!fd) {
		perror("newdata.txt");
		return 1;
	}

	for (uint64_t i = 0; i < sz; ++i) {
		char *new_key = get_rand_string(10);
		tnode *td;
		uint32_t hash_sum;
		tnode *node;

		if (!new_key) {
			fclose(fd);
			tommy_hashdyn_done(&hashdyn);
			return 1;
		}

		td = malloc(sizeof(*td));
		if (!td) {
			free(new_key);
			fclose(fd);
			tommy_hashdyn_done(&hashdyn);
			return 1;
		}

		td->key = new_key;
		hash_sum = tommy_strhash_u32(0, new_key);
		node = tommy_hashdyn_search(&hashdyn, tommy_check, new_key, hash_sum);
		if (!node)
			tommy_hashdyn_insert(&hashdyn, &td->node, td, hash_sum);
		fputs(new_key, fd);
		fputs("\n", fd);
	}
	fclose(fd);

	clock_gettime(CLOCK_MONOTONIC, &inited);
	print_elapsed("init", &init, &inited);

	clock_gettime(CLOCK_MONOTONIC, &read);
	fd = fopen("newdata.txt", "r");
	if (!fd) {
		perror("newdata.txt");
		tommy_hashdyn_done(&hashdyn);
		return 1;
	}

	while (fgets(buf, sizeof(buf), fd)) {
		uint32_t hash_sum;

		strip_line_ending(buf);
		hash_sum = tommy_strhash_u32(0, buf);
		tommy_hashdyn_search(&hashdyn, tommy_check, buf, hash_sum);
	}
	fclose(fd);

	printf("hash table size is %llu, memory %llu\n",
		(unsigned long long)tommy_hashdyn_count(&hashdyn),
		(unsigned long long)tommy_hashdyn_memory_usage(&hashdyn));

	clock_gettime(CLOCK_MONOTONIC, &readed);
	print_elapsed("read", &read, &readed);

	tommy_hashdyn_done(&hashdyn);
	return 0;
}
