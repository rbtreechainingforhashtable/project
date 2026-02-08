#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <mach/clock.h>
#include <mach/mach.h>
#include <time.h>
#include "hashtable.h"
#include "list.h"

typedef struct ngram_list {
    char **ngrams;
    uint64_t size;
} ngram_list;

typedef struct ngram_index_t {
    hashtable_t *ht;
    uint64_t size;
} ngram_index_t;


ngram_index_t *ngram_index_make() {
    ngram_index_t *new = calloc(1, sizeof(ngram_index_t));
    new->ht = hashtable_new(128, fnv);
    return new;
}

void ngram_index_delete(ngram_index_t *index) {
    hashtable_free(index->ht);
    free(index);
}

ngram_list* ngram_get(char *seq, uint8_t n) {
    uint64_t i;
    ngram_list *list = calloc(1, sizeof(*list));

    if (!seq)
        return list;

    uint64_t len = strlen(seq);
    if (len < n)
        return list;

    for (i = 0; seq[i+n-1]; ++i);
    list->ngrams = calloc(1, sizeof(char*)*i);

    for (i = 0; seq[i+n-1]; ++i) {
        list->ngrams[i] = strndup(seq+i, n);
        //printf("\t\tngram[%llu]='%s'\n", i, list->ngrams[i]);
    }
    list->size = i;

    return list;
}

void ngram_merge(ngram_index_t *ngram_index, ngram_list *list, char *passdata) {
    hashtable_t *ht = ngram_index->ht;
    for(uint64_t i = 0; i < list->size; ++i) {
        char *passkey = list->ngrams[i];
        //printf("\tcheck the passkey %s\n", passkey);
        node_t *data = hashtable_get_auto(ht, passkey);
        if (!data)
        {
			++ngram_index->size;
            list_t *list = list_new();
            list_add(list, passdata, NULL);
            data = hashtable_insert_auto(ht, passkey, list);
            if (!data) {
                printf("error inserting passkey %s\n", passkey);
            }
        }
        else {
            list_add(data->data, passdata, NULL);
        }
    }
}

char** regex_split_to_tokens(char *inp, uint64_t *size) {
    char *ptr = strdup(inp);
    char **ret;

    uint64_t i;
    uint64_t len = strlen(inp);

    for (i = 0; i < len; ) {
        uint64_t cur = strcspn(ptr, "[](){}|");
        i += cur + 1;
    }
    ret = calloc(1, sizeof(char*) * i);

    while (1) {
        char *pch = strstr(ptr, "[");
        if (!pch)
            break;

        char* end = strstr(ptr, "]");
        if (!end)
            break;

        ++end;
        if (*end == '{') {
            end = strstr(end, "}");
            if (!end)
                break;
            ++end;
        }

        if ((*end == '+') || (*end == '*')) {
            ++end;
        }

        //printf("seq copy '%s' to '%s'\n", end, pch);
        memmove(pch, end, len - (end - pch));
    }

    char *pch = strtok(ptr, "[]()|{}");
    for (i = 0; pch; ++i) {
        char* next = strtok(NULL, "[]()|{}");
        if (next) {
            ret[i] = strndup(pch, next - pch);
        } else {
            ret[i] = strdup(pch);
            break;
        }
        pch = next;
    }
    //printf("calculated size is %llu\n", i);
    *size = ++i;

    free(ptr);
    return ret;
}

void regex_split_free(char **regex_split, uint64_t size) {
    for (uint64_t i = 0; i < size; ++i)
        free(regex_split[i]);

    free(regex_split);
}


void ngram_regex_wrapper_push(ngram_index_t *ngram_index, char *regex, char *route_name) {
    uint64_t size;
    char** regex_tokenized;
    regex_tokenized = regex_split_to_tokens(regex, &size);
    printf("regex '%s'\n", regex);
    for(uint64_t i = 0; i < size; ++i) {
        printf("\tngram: '%s'\n", regex_tokenized[i]);
        ngram_merge(ngram_index, ngram_get(regex_tokenized[i], 3), regex);
    }
    regex_split_free(regex_tokenized, size);
}

uint64_t matched;
void matched_data(node_t *node, void *arg, void *arg2, void *arg3) {
    hashtable_t *ht = arg;
    char *request = arg3;
    ++matched;
    //printf("\t-> '%s'\n", node->key);
}

//void ngrams(node_t *node, void *arg, void *arg2, void *arg3) {
//    printf("\t-> key '%s'\n", node->key);
//}

void fill_the_query(list_node *lnode, void *arg) {
    char *data = (char*)lnode->arg1;
    hashtable_t *routes_match = arg;

    node_t *routes = hashtable_get_auto(routes_match, data);
    if (!routes)
    {
        routes = hashtable_insert_auto(routes_match, data, NULL);
        if (!routes) {
            printf("error inserting routes match %s\n", data);
        }
    }
}

void ngram_regex_match(ngram_index_t *ngram_index, char *buf) {
    hashtable_t *routes_match = hashtable_new(32, fnv);
    ngram_list *list = NULL;
    uint64_t len = strlen(buf);
    if (len > 5)
        list = ngram_get(buf, 5);
    else if (len > 4)
        list = ngram_get(buf, 4);
    else if (len > 3)
        list = ngram_get(buf, 3);
    else if (len > 2)
        list = ngram_get(buf, 2);
    else
        return;
    //ngram_list *list = ngram_get(buf, 4);
    hashtable_t *ht = ngram_index->ht;
    for(uint64_t i = 0; i < list->size; ++i) {
        char *passkey = list->ngrams[i];
        node_t *data = hashtable_get_auto(ht, passkey);
        if (!data)
            continue;

        list_t *list = data->data;
        list_foreach_node(list, fill_the_query, routes_match);
    }

    if (routes_match->count)
    {}
    //    printf("request '%s' matched with\n", buf);
    else
        printf("request '%s' NOT MATCHED\n", buf);
    hashtable_foreach(routes_match, matched_data, buf);
    hashtable_free(routes_match);
}

void check_logs(ngram_index_t *ngram_index) {
    FILE *fd = fopen("logs.txt", "r");
    if (!fd)
        return;

    char buf[256];
    while (fgets(buf, 256, fd)) {
        buf[strlen(buf)-1] = 0;
        ngram_regex_match(ngram_index, buf);
    }
    fclose(fd);
}

void regex_tests() {
    ngram_index_t *ngram_index = ngram_index_make();

    ngram_regex_wrapper_push(ngram_index, "/img/(..)(.+)$", "img_back");
    ngram_regex_wrapper_push(ngram_index, "/img/([0-9a-f]{2})([0-9a-f]+)$", "imgversion_back");
    ngram_regex_wrapper_push(ngram_index, "/(?:index)\\.php(?:$|/)", "indexphp");
    ngram_regex_wrapper_push(ngram_index, "^/(?:core/img/background.png|core/img/favicon.ico)(?:$|/)", "main_storage");
    ngram_regex_wrapper_push(ngram_index, "^/(?:index|core/ajax/update|ocs/v[12]|status|updater/.+|oc[ms]-provider/.+)\\.php(?:$|/)", "update_back");
    ngram_regex_wrapper_push(ngram_index, "^/web/api/v1/([A-Za-z]+)$", "web_api");
    ngram_regex_wrapper_push(ngram_index, "/latest/api", "latest_api");
    ngram_regex_wrapper_push(ngram_index, "/api", "api");
    ngram_regex_wrapper_push(ngram_index, "^(/[^/]+)/api(.*)$", "api_prefixed");
    ngram_regex_wrapper_push(ngram_index, "^/api/(?<version>.*)/(?<service>.*)(/.*/.*/.*/.*)$", "api_versioned");
    ngram_regex_wrapper_push(ngram_index, "/reset_password/([0-9a-z]*)", "reset_pass");
    ngram_regex_wrapper_push(ngram_index, "^/.well-known/acme-challenge/(.*)$", "acme");
    ngram_regex_wrapper_push(ngram_index, "/(cms|Account)", "cms");
    ngram_regex_wrapper_push(ngram_index, ".(?:css|js|mjs|svg|gif|png|jpg|ico|wasm|tflite|map|woff2)$", "static");
    ngram_regex_wrapper_push(ngram_index, "(.*/myapp)/(.+\\.php)$", "myapp_php");
    ngram_regex_wrapper_push(ngram_index, "(?<begin>.*myapp)/(?<end>.+\\.php)$", "myapp_php_with_vars");
    ngram_regex_wrapper_push(ngram_index, "^/consultation/tag/(.*)", "consultation");
    ngram_regex_wrapper_push(ngram_index, "^/books-app/region/(.*)/tags/(.*)", "book-region");
    ngram_regex_wrapper_push(ngram_index, "/health", "health");
    ngram_regex_wrapper_push(ngram_index, "/counter", "counter");
    ngram_regex_wrapper_push(ngram_index, "^/(assets|vite|data|system|packs)", "assets");
    ngram_regex_wrapper_push(ngram_index, "/swagger/", "swagger");
    ngram_regex_wrapper_push(ngram_index, "/admin/", "admin");
    ngram_regex_wrapper_push(ngram_index, "/user", "user");
    ngram_regex_wrapper_push(ngram_index, "/metrics", "metrics");
    ngram_regex_wrapper_push(ngram_index, "/hub", "hub");
    ngram_regex_wrapper_push(ngram_index, "/upload", "upload");
    ngram_regex_wrapper_push(ngram_index, "/keycloak", "keycloack");
    ngram_regex_wrapper_push(ngram_index, "/api/v2/tile_vector", "tile_api");
    ngram_regex_wrapper_push(ngram_index, "(\\.\\w+?)$", "misc");
    ngram_regex_wrapper_push(ngram_index, "/tomcat", "tomcat");
    ngram_regex_wrapper_push(ngram_index, "/.*\\.m3u8", "media");
    ngram_regex_wrapper_push(ngram_index, "/.*\\*.md", "docs");

    //hashtable_foreach(ngram_index->ht, ngrams, NULL);

    check_logs(ngram_index);

    ngram_index_delete(ngram_index);
}

void do_check(ngram_index_t *ngram_index, uint64_t letters, uint64_t maxnum) {
    matched = 0;
    struct timespec checking;
    clock_gettime(CLOCK_REALTIME, &checking);

    char buf[256];
    FILE *fd = fopen("words_alpha.txt", "r");
    //for (uint64_t i = 1; i < argc; ++i)
    //    ngram_regex_match(ngram_index, argv[i]);
    uint64_t resulted = 0;
    while (fgets(buf, 256, fd) && maxnum) {
        size_t len = strlen(buf)-1;
        if (len != letters ) {
            continue;
        }

        --maxnum;
        buf[len] = 0;
        //printf("check buf %s\n", buf);
        ngram_regex_match(ngram_index, buf);
        ++resulted;
    }
    fclose(fd);

    struct timespec checked;
    clock_gettime(CLOCK_REALTIME, &checked);
    fprintf(stderr, "token %llu checks takes %ld.%03lu seconds, maxnum %llu, checked: %llu, matched %llu, results per query: %lf\n", letters, checked.tv_sec - checking.tv_sec, ((checked.tv_nsec-checking.tv_nsec)/1000000), maxnum, resulted, matched, ((matched * 1.0) / resulted));
}

void suggests_test(int argc, char **argv) {
    if (argc < 1)
        return;

    ngram_index_t *ngram_index = ngram_index_make();

    //hashtable_foreach(ngram_index->ht, ngrams, NULL);
    FILE *fd = fopen("words_alpha.txt", "r");
    if (!fd)
        return;

    struct timespec pre;
    clock_gettime(CLOCK_REALTIME, &pre);
    char buf[256];
	uint64_t words = 0;
    while (fgets(buf, 256, fd)) {
		++words;
        buf[strlen(buf)-1] = 0;
        //ngram_merge(ngram_index, ngram_get(buf, 2), strdup(buf));
        ngram_merge(ngram_index, ngram_get(buf, 3), strdup(buf));
        //ngram_merge(ngram_index, ngram_get(buf, 4), strdup(buf));
        //ngram_merge(ngram_index, ngram_get(buf, 5), strdup(buf));
		if ((words < 10) || ((words < 100) && ((words % 10) == 0)) || ((words < 1000) && ((words % 100) == 0)) || ((words < 10000) && ((words % 1000) == 0)))
			printf("words: %llu, ngrams: %llu: %s\n", words, ngram_index->size, buf);
    }
    fclose(fd);
    struct timespec loaded;
    clock_gettime(CLOCK_REALTIME, &loaded);
    fprintf(stderr, "inverted index load took %ld.%03lu seconds\n", loaded.tv_sec - pre.tv_sec, ((loaded.tv_nsec-pre.tv_nsec)/1000000));

    uint64_t maxnum = 1000 * 4;

    //do_check(ngram_index, 3, maxnum);
    //do_check(ngram_index, 4, maxnum);
    //do_check(ngram_index, 5, maxnum);
    //do_check(ngram_index, 6, maxnum);
    //do_check(ngram_index, 7, maxnum);


    ngram_index_delete(ngram_index);
}

int main(int argc, char **argv) {
    //regex_tests();
    suggests_test(argc, argv);
}
