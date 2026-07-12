#pragma once

#include <math.h>
#include <stddef.h>

/*
 * Sample statistics for repeated benchmark timings.
 * Uses sample standard deviation (Bessel, n-1 denominator).
 */

typedef struct bench_stats {
	size_t n;
	double sum;
	double sum_sq;
	double min;
	double max;
} bench_stats_t;

static inline void bench_stats_init(bench_stats_t *s)
{
	s->n = 0;
	s->sum = 0.0;
	s->sum_sq = 0.0;
	s->min = 0.0;
	s->max = 0.0;
}

static inline void bench_stats_add(bench_stats_t *s, double value)
{
	if (!s->n) {
		s->min = value;
		s->max = value;
	} else {
		if (value < s->min)
			s->min = value;
		if (value > s->max)
			s->max = value;
	}
	s->sum += value;
	s->sum_sq += value * value;
	++s->n;
}

static inline double bench_stats_mean(const bench_stats_t *s)
{
	return s->n ? s->sum / (double)s->n : 0.0;
}

static inline double bench_stats_stddev(const bench_stats_t *s)
{
	if (s->n < 2)
		return 0.0;
	{
		double mean = bench_stats_mean(s);
		double var = (s->sum_sq - (double)s->n * mean * mean) / (double)(s->n - 1);
		return var > 0.0 ? sqrt(var) : 0.0;
	}
}
