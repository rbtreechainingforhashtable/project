#pragma once

#include <stdint.h>
#include <time.h>

static inline void timespec_now(struct timespec *ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
}

static inline double timespec_elapsed_sec(const struct timespec *start,
                                          const struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec)
        + (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

static inline uint64_t timespec_elapsed_nsec(const struct timespec *start,
                                             const struct timespec *end)
{
    return (uint64_t)(end->tv_sec - start->tv_sec) * 1000000000ULL
        + (uint64_t)(end->tv_nsec - start->tv_nsec);
}
