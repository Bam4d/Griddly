# Benchmarks

C++ micro-benchmarks for the engine hot paths. Build against the Release
static lib (see `build.sh` for the compile/link recipe used to produce them).

- `bench_bare.cpp` — single env, no VectorizedEnv: tight `performActionsPooled`
  loop. Modes: 0 = full step, 1 = no-op actions, 2 = actions without tick.
- `bench_vectorized.cpp` — envs × threads sweep table (env-steps/s).
- `bench_grid2.cpp` — envs × threads sweep for step throughput **and** reset
  throughput, written to a text file (CSV-friendly).

## Data

- `spiders_lvl4_envs_threads_2026-11-05.txt` — raw output of `bench_grid2` on
  `Single-Player/Mini-Grid/minigrid-spiders.yaml` level 4 (18x55, biggest level),
  2000 steps / 200 resets per cell, commit 1ee4840b.
- `spiders_lvl4_step.csv` / `spiders_lvl4_reset.csv` — same data as CSV
  (rows = num envs, cols = threads).

## How to run

```bash
cd build && ninja Griddly_static   # from repo root
# compile/link recipe in build.sh, then:
./bench_grid2 "Single-Player/Mini-Grid/minigrid-spiders.yaml" 4 2000 200
./bench_bare  "Single-Player/Mini-Grid/minigrid-spiders.yaml" 4 20000 0
```
