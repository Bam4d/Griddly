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

let queue: Promise<unknown> = Promise.resolve();

/**
 * Run `fn` with exclusive use of the shared engine. Game setup on the player page and each
 * thumbnail render go through this queue so engine calls of an async job never interleave.
 */
export function withGriddly<T>(fn: (g: Griddly) => Promise<T> | T): Promise<T> {
  const run = queue.then(async () => fn(await getGriddly()));
  queue = run.catch(() => undefined);
  return run;
}

/** Load a GDY string into the shared engine, replacing whatever was loaded. */
export function loadGdy(g: Griddly, gdy: string): void {
  g.unload();
  g.loadGDYString(gdy);
}
