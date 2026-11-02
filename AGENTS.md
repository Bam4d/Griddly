# AGENTS.md — Griddly (vibe_main worktree)

## Working agreement

- All work on this project happens on the **`vibe_main`** branch, checked out in
  this worktree (`~/.vibe/worktrees/Griddly-<hash>/vibe_main`), branched from
  `origin/master`. Do not work from the `~/Griddly` checkout or other branches.
- Keep changes small and coherent; match existing style in the area you touch.

## Project overview

Griddly is a grid-world game engine for Game AI research:

- `src/` — C++ core engine (game logic, rendering via OpenGL/GLFW, observers)
- `python/` — Python bindings (`griddly` package), GDY loader, gym wrapper
- `resources/games/` — built-in game definitions (GDY YAML) under
  `Single-Player/`, `Multi-Agent/`, `RTS/`; tilesets in `resources/images/`
- `js/`, `bindings/` — JS/WASM and other language bindings
- `docs/` — Sphinx docs; `tests/` — tests
- Build: CMake (`CMakeLists.txt`, `configure.sh`); Python package via poetry
  (`python/pyproject.toml`, requires Python <3.12, numpy <1.24)

## Conventions

- Games are defined in GDY YAML; levels are ASCII maps using each object's
  `MapCharacter`.
- Observers: `Sprite2D`, `Block2D`, `Isometric` (GL), plus `Vector`/`ASCII`
  (headless-safe, pure CPU).
