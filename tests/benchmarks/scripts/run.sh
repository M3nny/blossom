#!/usr/bin/env sh
set -e

# The benchmarks resolve their paths from the project root.
cd "$(dirname "$0")/../../.."

[ -x build/bench_throughput ] || { echo "no benchmarks in build/ run 'make' first"; exit 1; }

OUT="tests/benchmarks/results"
mkdir -p "$OUT"

# Pinning to one core keeps the scheduler from moving the working set between L2s mid-measurement.
# This is optional since it is not available everywhere.
PIN=""
command -v taskset > /dev/null && PIN="taskset -c 2"

for name in throughput locality hitrate fpr; do
    echo "bench_$name"
    $PIN "build/bench_$name" > "$OUT/$name.csv"
done

echo "done, CSVs in $OUT/"
