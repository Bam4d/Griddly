#!/usr/bin/env bash
# Copy the prebuilt @griddly/wasm package from this repo's js/griddly-wasm into the app as a
# workspace package (packages/ts/griddly-wasm), compiling its TypeScript wrapper into dist/.
# `mistral deploy --from-disk` only uploads this directory, so the engine has to live inside it.
# The copy is gitignored: js/griddly-wasm is the single source of truth.
set -euo pipefail
app="$(cd "${0%/*}/.." && pwd)"
src="$app/../../js/griddly-wasm"
dst="$app/packages/ts/griddly-wasm"
for f in griddlyjs.js griddlyjs.wasm griddlyjs.data package.json src/index.ts; do
  [ -f "$src/$f" ] || { echo "missing $src/$f (build the wasm first)" >&2; exit 1; }
done
rm -rf "$dst" && mkdir -p "$dst"
cp "$src"/griddlyjs.{js,wasm,data} "$src/README.md" "$dst/"
bunx -p typescript@5.9.3 tsc -p "$src/tsconfig.json" --outDir "$dst/dist" --noCheck  # type-checked in CI (js-griddly-wasm-test)
# Keep only what the app needs from package.json (no scripts/devDependencies to install).
bun -e '
const [s, d] = process.argv.slice(1);
const p = await Bun.file(s).json();
delete p.scripts; delete p.devDependencies; p.private = true;
await Bun.write(d, JSON.stringify(p, null, 2) + "\n");
' "$src/package.json" "$dst/package.json"
echo "synced @griddly/wasm -> ${dst#$app/}"
