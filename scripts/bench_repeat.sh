#!/usr/bin/env bash
# Run a benchmark command N times and report mean ± sample stddev for numeric
# fields in experiment=... log lines (stderr).
#
# Usage:
#   ./scripts/bench_repeat.sh RUNS COMMAND [ARGS...]

set -euo pipefail

if [[ $# -lt 2 ]]; then
	echo "usage: $0 RUNS COMMAND [ARGS...]" >&2
	exit 1
fi

runs=$1
shift

if ! [[ "$runs" =~ ^[0-9]+$ ]] || [[ "$runs" -lt 1 ]]; then
	echo "RUNS must be a positive integer" >&2
	exit 1
fi

log=$(mktemp)
trap 'rm -f "$log"' EXIT

for ((i = 1; i <= runs; ++i)); do
	"$@" 2>>"$log" >/dev/null || {
		echo "run $i failed" >&2
		exit 1
	}
done

awk -v runs="$runs" '
function is_num(s) {
	return s ~ /^-?[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?$/
}
function add(field, val,    v) {
	v = val + 0
	count[field]++
	sum[field] += v
	sumsq[field] += v * v
	if (!(field in min) || v < min[field]) min[field] = v
	if (!(field in max) || v > max[field]) max[field] = v
}
/^experiment=/ {
	for (i = 1; i <= NF; ++i) {
		n = split($i, kv, "=")
		if (n != 2) continue
		if (!is_num(kv[2])) continue
		add(kv[1], kv[2])
	}
}
function mean(f) {
	return count[f] ? sum[f] / count[f] : 0
}
function stddev(f,    n, m, v) {
	n = count[f]
	if (n < 2) return 0
	m = mean(f)
	v = (sumsq[f] - n * m * m) / (n - 1)
	return v > 0 ? sqrt(v) : 0
}
END {
	print "runs=" runs
	for (f in count) {
		printf "%s: mean=%.6f stddev=%.6f min=%.6f max=%.6f n=%d\n",
			f, mean(f), stddev(f), min[f], max[f], count[f]
	}
}
' "$log" | sort

echo "--- raw runs ---" >&2
cat "$log" >&2
