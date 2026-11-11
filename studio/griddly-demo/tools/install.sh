#!/usr/bin/env bash
# Bootstrap the workspace: install the uv (Python) and Bun (JS) halves, plus any configured hooks,
# then regenerate the web API client when the app has one.
# The command is deterministic and non-interactive and does not run checks or application services.
set -euo pipefail

# Bun does not self-enforce package.json `packageManager` the way corepack does, so a mismatched
# local bun can rewrite `bun.lock` in a way the pinned `oven/bun` Docker image rejects under
# `--frozen-lockfile`. Gate on the pin so the committed lockfile is always the one the image builds.
require_bun() {
  local manager required installed
  manager="$(sed -n 's/.*"packageManager"[[:space:]]*:[[:space:]]*"\(bun@[0-9][0-9.]*\)".*/\1/p' package.json | head -n1)"
  required="${manager#bun@}"
  if [ -z "$required" ]; then
    echo "package.json packageManager is not a pinned bun version: '${manager}'" >&2
    exit 1
  fi
  if ! command -v bun >/dev/null 2>&1; then
    echo "bun ${required} is required but no bun is on PATH. Install it: https://bun.sh" >&2
    exit 1
  fi
  installed="$(bun --version)"
  if [ "$installed" != "$required" ]; then
    echo "bun ${required} is required (package.json packageManager), but bun ${installed} is on PATH." \
      "Run 'bun upgrade --stable' (or install bun ${required}) and retry." >&2
    exit 1
  fi
}

# Source uv.sh (it does not exec when sourced) so both package managers inherit the deterministic
# index environment and any audience-specific authentication prepared for this generated app.
. "${0%/*}/uv.sh"
require_bun
uv sync --all-packages
bun install
# Install hooks only when their configuration is present in this composition.
if [ -f .pre-commit-config.yaml ]; then
  uv run --no-sync pre-commit install --install-hooks
fi
# The optional `fastapi-tanstack-start` integration ships the web API client's generator but no
# client: generate it for the capabilities this app selected, or the web app cannot build. A no-op
# diff once the client is committed.
if [ -f tools/gen-types.sh ]; then
  bash tools/gen-types.sh
fi
