import type { Plugin } from "vite";

/**
 * Stub Node-only builtins for the *browser* build only. @griddly/wasm imports
 * `node:fs` inside its Node branch (guarded at runtime by a
 * `process.versions.node` check); without this stub the bundler warns, and in
 * the Nitro *server* build it would break nitro's own asset reader. The stub
 * only applies when bundling for the browser.
 */
const STUB = `
const unavailable = () => { throw new Error("node builtins are not available in the browser"); };
export const readFileSync = unavailable;
export const readFile = unavailable;
export const promises = { readFile: unavailable, readFileSync: unavailable };
export default { readFileSync: unavailable, readFile: unavailable, promises };
`;

const griddlyWasmNodeStub: Plugin = {
  name: "griddly-wasm-node-stub",
  apply: "build",
  resolveId(id: string, importer: string | undefined) {
    // Only rewrite imports coming from the @griddly/wasm package, and only for
    // browser targets — never for the nitro server bundle.
    if (
      (id === "node:fs" || id === "node:fs/promises") &&
      importer?.includes("griddly-wasm")
    ) {
      return "\0griddly-wasm:node-stub";
    }
    return null;
  },
  load(id: string) {
    if (id === "\0griddly-wasm:node-stub") return STUB;
    return null;
  },
};

export default griddlyWasmNodeStub;
