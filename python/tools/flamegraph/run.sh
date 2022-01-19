#!/bin/bash

# run the benchmark script
echo "Recording performance data..."
perf record -F 200 --call-graph dwarf python $2

# Generate the folded stack
echo "Processing..."
perf script | $1/stackcollapse-perf.pl > out.perf-folded

# Generate the flame graph svg
echo "Generating flame graph..."
$1/flamegraph.pl out.perf-folded > perf.svg