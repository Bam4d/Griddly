# Benchmarks

C++ micro-benchmarks for the engine hot paths. Build against the Release
static lib (see `build.sh` for the compile/link recipe used to produce them).

- `bench_bare.cpp` — single env, no VectorizedEnv: tight `performActionsPooled`
  loop. Modes: 0 = full step, 1 = no-op actions, 2 = actions without tick.
- `bench_vectorized.cpp` — envs × threads sweep table (env-steps/s).
- `bench_grid2.cpp` — envs × threads sweep for step throughput, full-reset
  throughput, **and** snapshot-reset (`resetFromSnapshot`, the arena memcpy)
  throughput, written to a text file (CSV-friendly).
- `bench_tiled.cpp` — VectorizedEnv + `TiledSDLRenderer`: step-only,
  render-only (frames/s), and combined step+render env-steps/s sweeps.

## Data

- `spiders_lvl4_envs_threads_2026-11-05.txt` — raw output of `bench_grid2` on
  `Single-Player/Mini-Grid/minigrid-spiders.yaml` level 4 (18x55, biggest level),
  2000 steps / 200 resets per cell, commit 1ee4840b.
- `spiders_lvl4_step.csv` / `spiders_lvl4_reset.csv` — same data as CSV
  (rows = num envs, cols = threads).
- `spiders_lvl4_envs_threads_snapreset_2026-11-05.txt` — latest full run with
  all three tables (step / full-reset / snapshot-reset);
  `spiders_lvl4_{step,reset,snapshot_reset}_2026-11-05.csv` — CSV versions.

## How to run

```bash
cd build && ninja Griddly_static   # from repo root
# compile/link recipe in build.sh, then:
./bench_grid2 "Single-Player/Mini-Grid/minigrid-spiders.yaml" 4 2000 200
./bench_bare  "Single-Player/Mini-Grid/minigrid-spiders.yaml" 4 20000 0
```
