#include "randstring.h"

#include <stdlib.h>

char *get_rand_string(size_t size)
{
    char *str = malloc(size);
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKKLMNOPQRSTIVWXYZ";

    if (!str)
        return NULL;

    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int)(sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }

    return str;
}
