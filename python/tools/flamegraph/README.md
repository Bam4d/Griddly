# FlameGraph tools

This folder contains tools for building flamegraphs for various benchmarks in griddly to find slow parts of code and optimizing.

## Prerequisites

### Flamegraph

To generate the flamegraphs from perf.data files, we need to clone the flamegraph repository:
```
git clone https://github.com/brendangregg/FlameGraph
```

### perf

In order to generate performance data we need to install `perf`
```
sudo apt install perf
```


## Usage

Firstly run the benchmark tool on a particular script.

```
./run.sh {flamegraph/location} {benchmark_script.py}
```

For example, to run the `shader_benchmark`:

```
./run.sh path/to/FlameGraph shader_benchmark/benchmark.py
```

This will output several files, one of which is a flamegraph `perf.svg`

### Viewing flamegraphs

You can view flamegraphs in any svg editor, or convenently in firefox or chrome
```
firefox perf.svg
```



