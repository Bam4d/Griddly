#!/usr/bin/env bash
# Deploy this folder to Studio. `mistral deploy --from-disk` uploads only this directory and skips
# git-ignored files, so the engine package is synced in (un-ignored) for the upload and removed
# afterwards so the ~15 MB copy never gets committed. js/griddly-wasm stays the source of truth.
set -euo pipefail
cd "${0%/*}/.."
bash tools/sync-griddly-wasm.sh
trap 'rm -rf packages/ts' EXIT
bun install --frozen-lockfile
mistral deploy --from-disk "$@"
