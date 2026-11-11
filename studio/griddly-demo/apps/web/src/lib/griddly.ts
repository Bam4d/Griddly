/**
 * Browser-side loader for the Griddly WASM engine (@griddly/wasm).
 *
 * Vite resolves the package's `griddlyjs.wasm` / `griddlyjs.data` imports to
 * emitted asset URLs, so the default `import.meta.url` resolution in the
 * package just works — no manual `locateFile` needed.
 */
import { createGriddly, type Griddly } from "@griddly/wasm";

let instance: Promise<Griddly> | null = null;

/** Shared, lazily-initialized Griddly instance (WASM init is expensive). */
export function getGriddly(): Promise<Griddly> {
  instance ??= createGriddly();
  return instance;
}
