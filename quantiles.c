#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include "qtree.h"

uint64_t nsec_tree;
typedef struct list_node {
    void *arg1;
    void *arg2;
    struct list_node *next;
} list_node;

typedef struct list_t {
    struct list_node *head;
} list_t;

list_t* list_new() {
    return calloc(1, sizeof(list_t));
}

void list_add(list_t *list, void *arg1, void *arg2) {
    list_node *new = malloc(sizeof(*new));

    new->arg1 = arg1;
    new->arg2 = arg2;
    new->next = list->head;

    list->head = new;
}

void list_free(list_t *list) {
    list_node *lnode = list->head;
    while (lnode) {
        list_node *next = lnode->next;
        free(lnode);
        lnode = next;
    }

    free(list);
}

void list_foreach_node(list_t *list, void (*callback)(list_node*, void*), void* arg1) {
    list_node *lnode = list->head;
    while (lnode) {
        list_node *next = lnode->next;
        callback(lnode, arg1);
        lnode = next;
    }
}


typedef struct hashtable_t {
    tree_t *entries;
    uint64_t count;
    uint8_t tree_height;
    double step;
    uint64_t allocated;
    uint64_t hash_collisions;
    uint64_t (*hash_func)(double, double);
} hashtable_t;

hashtable_t *hashtable_new(uint64_t supposed_size, uint64_t (*hash_func)(double, double)) {
    hashtable_t *ht = calloc(1, sizeof(*ht));
    ht->entries = calloc(1, sizeof(tree_t)*supposed_size);
    ht->allocated = supposed_size;
    ht->hash_func = hash_func;

    return ht;
}

void hashtable_fixup_tree_nodes(node_t *node, void *arg, void *arg2, void *arg3) {
    hashtable_t* ht = arg;
    uint64_t *position = arg2;
    uint64_t right_place = node->sum % ht->allocated;
    list_t* delete_buffer = arg3;
    if (right_place != *position) {
        list_add(delete_buffer, &ht->entries[*position], node);
        tree_insert(&ht->entries[right_place], node->key, node->sum, node->data, ht->tree_height);
    }
}

void emptying_moved_node(list_node *lnode, void *arg) {
    node_t *node = (node_t*)lnode->arg2;
    tree_t *tree = (tree_t*)lnode->arg1;
    hashtable_t* ht = arg;
    node = rb_delete(tree, node);
    if (node)
        --ht->count;
    free(node);
}

void hashtable_fixup(hashtable_t* ht, uint64_t old_size) {
    list_t *delete_buffer = list_new();

    for (uint64_t i = 0; i < old_size; ++i) {
        tree_t *cur = &ht->entries[i];
        tree_foreach_node(cur->root, hashtable_fixup_tree_nodes, ht, &i, delete_buffer);
    }

    list_foreach_node(delete_buffer, emptying_moved_node, ht);
    list_free(delete_buffer);
}

hashtable_t *hashtable_resize(hashtable_t* ht, uint64_t new_size) {
    if (!new_size)
        new_size = ht->allocated * 2;

    tree_t *new = calloc(1, sizeof(tree_t)*new_size);

    memcpy(new, ht->entries, sizeof(tree_t)*ht->allocated);
    tree_t *old = ht->entries;
    uint64_t old_size = ht->allocated;
    ht->entries = new;
    ht->allocated = new_size;
    free(old);

    hashtable_fixup(ht, old_size);

    return ht;
}



node_t *hashtable_insert_hash(hashtable_t* ht, double key, uint64_t hash_sum, uint64_t data) {
    uint64_t position = hash_sum % ht->allocated;
    tree_t *node = &ht->entries[position];

    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_REALTIME, &start);
    node_t *ret = tree_insert(node, key, hash_sum, data, ht->tree_height);
    clock_gettime(CLOCK_REALTIME, &end);
    nsec_tree += ((end.tv_sec - start.tv_sec)*1000000000) + (end.tv_nsec - start.tv_nsec);

    if (ret)
        ++ht->count;
    return ret;
}

node_t *hashtable_insert_auto(hashtable_t* ht, double key, uint64_t data) {
    uint64_t hash_sum = ht->hash_func(key, ht->step);
    return hashtable_insert_hash(ht, key, hash_sum, data);
}

node_t *hashtable_get_hash(hashtable_t* ht, double key, uint64_t hash_sum) {
    if (!ht)
        return NULL;

    uint64_t position = hash_sum % ht->allocated;
    tree_t *node = &ht->entries[position];
    node_t *branch = tree_get(node, key);
    if (!branch) {
        return NULL;
    }

    return branch;
}

node_t *hashtable_get_auto(hashtable_t* ht, double key) {
    uint64_t hash_sum = ht->hash_func(key, ht->step);
    return hashtable_get_hash(ht, key, hash_sum);
}

void print_node(node_t *node, void *arg, void *arg2, void *arg3) {
    uint64_t *position = arg2;
    hashtable_t *ht = arg;
    printf("[%p] '%lf'[%llu->%llu]: '%llu'\n", node, node->key, node->sum, *position, node->data);
}

void hashtable_foreach(hashtable_t *ht, void (*callback)(node_t*, void*, void*, void*), void *arg) {
    uint64_t max = ht->allocated;
    for (uint64_t i = 0; i < max; ++i) {
        tree_t *node = &ht->entries[i];
        if (node->count)
            tree_foreach_node(node->root, callback, ht, &i, arg);
    }
}

void hashtable_free(hashtable_t *ht) {
    uint64_t max = ht->allocated;
    list_t *delete_buffer = list_new();


    for (uint64_t i = 0; i < max; ++i) {
        tree_t *node = &ht->entries[i];
        if (node->count)
            if (node->root)
                rb_delete(node, node->root);
    }

    free(ht->entries);
    free(ht);
}


//#define FNV_OFFSET 14695981039346656037UL
//#define FNV_PRIME 1099511628211UL
//uint64_t fnv(const char* key) {
//    uint64_t hash = FNV_OFFSET;
//    for (const char* p = key; *p; p++) {
//        hash ^= (uint64_t)(unsigned char)(*p);
//        hash *= FNV_PRIME;
//    }
//    return hash;
//}
uint64_t ordered_hash(double key, double step) {
    return (uint64_t)(key / step);
}

typedef struct true_t {
    double *arr;
    uint64_t size;
    uint64_t cur;
} true_t;

int compare_dbl(const void * a, const void * b)
{
  double fa = *(const double*) a;
  double fb = *(const double*) b;
  return (fa > fb) - (fa < fb);
}

true_t *true_init(uint64_t size) {
    true_t *t = calloc(1, sizeof(*t));
    t->arr = calloc(1, sizeof(*t) * size);
    t->size = size;

    return t;
}

void true_push (true_t *t, double value, uint64_t count)
{
    for (uint64_t i = t->cur, j = 0; j < count; ++i, ++j) {
        t->arr[i] = value;
    }
    t->cur += count;
}

void true_sort(true_t *t)
{
    qsort (t->arr, t->cur, sizeof(double), compare_dbl);
}

void true_print(true_t *t) {
    for (uint64_t i = 0; i < t->cur; ++i) {
        printf ("\tdbl[%llu]: %lf\n", i, t->arr[i]);
    }
}

double true_quantile(true_t *t, double quantile) {
    uint64_t index = round(t->cur * quantile);
    return t->arr[index];
}


typedef struct quantiles_index_t {
    hashtable_t *ht;
    uint64_t count;
    double from;
    double to;
    double sum;
    int64_t highest;
    int64_t lowest;
} quantiles_index_t;

quantiles_index_t* quantiles_index_make(double from, double to, uint64_t size, uint8_t tree_height) {
    quantiles_index_t *qi = calloc(1, sizeof(*qi));
    qi->ht = hashtable_new(size, ordered_hash);
    qi->ht->step = fabs(to - from) / size;
    //qi->size = size;
    qi->from = from;
    qi->to = to;
    qi->lowest = -1;
    qi->ht->tree_height = tree_height;

    return qi;
}


void quantiles_insert(quantiles_index_t *qi, double data, uint64_t count) {
    uint64_t hash_index = qi->ht->hash_func(data, qi->ht->step);

    struct timespec start;
    struct timespec end;

    hashtable_insert_auto(qi->ht, data, count);

    double cell = (hash_index + 1.00) / qi->ht->allocated;
    //printf("cell: %lf for data:%lf calculate:(%llu+1) / %llu\n", cell, data, hash_index, qi->ht->allocated);

    double addnumber = (cell * count);
    qi->sum += addnumber;
    //printf("qi sum:%lf, cell:%lf\n", qi->sum, cell);
    qi->count += count;

    if (qi->lowest == -1)
        qi->lowest = hash_index;

    if (hash_index > qi->highest)
        qi->highest = hash_index;


    if (hash_index < qi->lowest)
        qi->lowest = hash_index;
}

double quantile_index_dev(quantiles_index_t *qi) {
    return (qi->sum / qi->count);
}

double quantile_calculate(quantiles_index_t *qi, double quantile) {
    printf("\nfind %lf\n", quantile);
    uint64_t cursize = qi->highest - qi->lowest;
    printf("cur size is %llu, highest %lld, lowest %lld\n", cursize, qi->highest, qi->lowest);
    double qi_dev = quantile_index_dev(qi);
    uint64_t index = round(qi_dev * 2 * quantile * cursize);
    index += qi->lowest;
    if (index > qi->highest)
        index = qi->highest;
    printf("index is %llu, step %lf, stddev: %lf, formula: round(%lf * 2 * %lf * %llu)+%llu\n", index, qi->ht->step, qi_dev, qi_dev, quantile, cursize, qi->lowest);
    node_t *qn = NULL;
    while (!qn)
    {
        qn = tree_quantile(&qi->ht->entries[index], quantile);
        if (!qn && (index <= qi->highest))
            if (quantile >= 0.5)
                --index;
            else
                ++index;
        else
            break;
    }
    //quantile_index_dev
    //return index * qi->ht->step;
    return qn ? qn->key : 0;
}

double randfrom(double min, double max)
{
    double range = (max - min);
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

uint64_t qnanoseconds;
uint64_t tnanoseconds;

void gen_data(quantiles_index_t *qi, true_t *t, double from, double to, uint64_t cnt) {
    for (uint64_t i = 0; i < cnt; ++i) {
        double rand = randfrom(from, to);
        struct timespec s_qinsert;
        struct timespec e_qinsert;
        struct timespec s_tinsert;
        struct timespec e_tinsert;

        clock_gettime(CLOCK_REALTIME, &s_qinsert);
        quantiles_insert(qi, rand, 1);
        clock_gettime(CLOCK_REALTIME, &e_qinsert);
        qnanoseconds += ((e_qinsert.tv_sec - s_qinsert.tv_sec)*1000000000) + (e_qinsert.tv_nsec - s_qinsert.tv_nsec);

        clock_gettime(CLOCK_REALTIME, &s_tinsert);
        true_push(t, rand, 1);
        clock_gettime(CLOCK_REALTIME, &e_tinsert);
        tnanoseconds += ((e_tinsert.tv_sec - s_tinsert.tv_sec)*1000000000) + (e_tinsert.tv_nsec - s_tinsert.tv_nsec);
    }
}

void quantiles_test() {
    quantiles_index_t *qi = quantiles_index_make(0, 1, 1024, 5);
    true_t *t = true_init(100000 + 100000000 + 10000000 + 1000000 + 100000);
    qnanoseconds = 0;
    tnanoseconds = 0;
    nsec_tree = 0;

    gen_data(qi, t, 0.0, 0.1, 100000);
    gen_data(qi, t, 0.1, 0.2, 100000000);
    gen_data(qi, t, 0.3, 0.5, 10000000);
    gen_data(qi, t, 0.5, 0.7, 1000000);
    gen_data(qi, t, 0.7, 0.9, 100000);

    //true_print(t);
    struct timespec s_qcalc;
    struct timespec e_qcalc;
    clock_gettime(CLOCK_REALTIME, &s_qcalc);
    printf("q(0.5) = %lf\nq(0.9)= %lf\nq(0.99)= %lf\n", quantile_calculate(qi, 0.5), quantile_calculate(qi, 0.9), quantile_calculate(qi, 0.99));
    clock_gettime(CLOCK_REALTIME, &e_qcalc);
    printf("\tquantile time elapsed: %ld.%03lu, push %.3f, tree %.3f/%llu\n", e_qcalc.tv_sec - s_qcalc.tv_sec, ((e_qcalc.tv_nsec-s_qcalc.tv_nsec)/1000000), qnanoseconds/1000000000.0, nsec_tree/1000000000.0, nsec_tree);

    //struct timespec s_tcalc;
    //struct timespec e_tcalc;
    //clock_gettime(CLOCK_REALTIME, &s_tcalc);
    //true_sort(t);
    //printf("t(0.5) = %lf\nt(0.9)= %lf\nt(0.99)= %lf\n", true_quantile(t, 0.5), true_quantile(t, 0.9), true_quantile(t, 0.99));
    //clock_gettime(CLOCK_REALTIME, &e_tcalc);
    //printf("\ttrue time elapsed: %ld.%03lu, push %.3f\n", e_tcalc.tv_sec - s_tcalc.tv_sec, ((e_tcalc.tv_nsec-s_tcalc.tv_nsec)/1000000), tnanoseconds/1000000000.0);
}


int main() {
    quantiles_test();
}
