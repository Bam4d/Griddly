# shellcheck shell=bash
# Stable cross-cutting shell policy shared by the capability tool scripts. Source it with
# `. "${0%/*}/lib.sh"` (resolves beside the sourcing script regardless of CWD, with no external
# command). Only domain-neutral policy lives here (skip reporting, the missing-tool gate); each
# capability keeps its own file discovery and domain commands in its own script, so projects and
# targets stay capability-owned. Shipped by `core`, so it is always present. No shebang and no
# `set` — this is a sourced fragment; the sourcing script owns the shell options. The `shell=bash`
# directive above stands in for the shebang so `quality:lint-shell` knows the dialect (SC2148
# otherwise).

# Report a check that is inapplicable to this composition (a skip, not a failure).
skipped() { echo "[check] SKIPPED $1: $2"; }

# Fail an *applicable* step whose executable is missing, pointing at how to install it.
require() {
  command -v "$1" >/dev/null 2>&1 || {
    printf "'%s' is required for this check but is not on PATH -- install it: %s\n" "$1" "$2" >&2
    exit 1
  }
}
