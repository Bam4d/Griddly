/**
 * @griddly/wasm — Griddly game engine (C++ → WebAssembly via emscripten/embind)
 * with a typed TypeScript API.
 *
 * Install from GitHub:
 *   npm install "github:bam4d/Griddly#vibe_main:js/griddly-wasm"
 *
 * Usage (browser / bundler such as Vite):
 *   import { createGriddly } from "@griddly/wasm";
 *   const griddly = await createGriddly();
 *   griddly.loadGDYString(gdyYaml);
 *   const game = griddly.createGame("Vector");
 *   ...
 *
 * The package ships the prebuilt artifacts (`griddlyjs.js`, `griddlyjs.wasm`,
 * `griddlyjs.data`). `createGriddly` locates them automatically:
 *  - in bundlers, via `import.meta.url` (Vite serves the wasm/data as assets);
 *  - in plain Node.js, from the package directory on disk.
 */

import type {
  GriddlyJS,
  GriddlyJSGDYWrapper,
  GriddlyJSGameWrapper,
  GriddlyJSPlayerWrapper,
  GriddlyModule,
} from "./module.js";

export * from "./module.js";

/** Convert an embind vector to a plain JS array. */
function vectorToArray<T>(vec: { size(): number; get(i: number): T }): T[] {
  const out: T[] = [];
  for (let i = 0; i < vec.size(); i++) out.push(vec.get(i));
  return out;
}



/** Unwrap embind C++ exceptions (thrown as numeric pointers) into Errors. */
function rethrow(griddlyjs: GriddlyJS, e: unknown): never {
  const ptr =
    typeof e === "number"
      ? e
      : e && typeof (e as { excPtr?: unknown }).excPtr === "number"
        ? (e as { excPtr: number }).excPtr
        : null;
  if (ptr !== null) {
    try {
      throw new Error(griddlyjs.getExceptionMessage(ptr));
    } catch (err) {
      throw err instanceof Error ? err : new Error(String(e));
    }
  }
  throw e instanceof Error ? e : new Error(String(e));
}

export interface GriddlyOptions {
  /**
   * Override asset resolution. Receives ("griddlyjs.wasm" | "griddlyjs.data")
   * and must return a URL/path the fetch/loader can use. By default the
   * package resolves assets relative to its own location (`import.meta.url`).
   */
  locateFile?: (file: string, prefix: string) => string;
  /** Passed straight through to the emscripten module (e.g. `print`, `printErr`). */
  moduleOverrides?: Record<string, unknown>;
}

export class Griddly {
  readonly module: GriddlyModule;
  readonly griddlyjs: GriddlyJS;
  private gdy: GriddlyJSGDYWrapper | null = null;
  private game: GriddlyJSGameWrapper | null = null;
  private players: GriddlyJSPlayerWrapper[] = [];
  playerCount = 0;

  private constructor(module: GriddlyModule) {
    this.module = module;
    this.griddlyjs = new module.GriddlyJS();
  }

  /** Load and initialize the WASM module. Safe to call once per page. */
  static async create(options: GriddlyOptions = {}): Promise<Griddly> {
    const moduleOverrides: Record<string, unknown> = {
      ...(options.moduleOverrides ?? {}),
      locateFile: (file: string, prefix: string): string => {
        if (options.locateFile) return options.locateFile(file, prefix);
        return defaultLocateFile(file, prefix);
      },
    };
    const locateFile = moduleOverrides.locateFile as (
      file: string,
      prefix: string
    ) => string;
    const wasmFactory = (await import("../griddlyjs.js")).default;
    // The build targets ENVIRONMENT=web; in Node we shim the few DOM globals
    // the SDL/web layer touches and force the node wasm path via wasmBinaryFile.
    const proc = (globalThis as { process?: { versions?: { node?: string } } }).process;
    const isNode = typeof proc !== "undefined" && !!proc.versions?.node;
    const overrides: Record<string, unknown> = { locateFile };
    if (isNode) {
      installNodeDomShim();
      // The build targets ENVIRONMENT=web, so emscripten always fetch()es the
      // wasm and the .data file; Node's fetch rejects plain paths. Read the
      // artifacts synchronously and hand them over directly so no fetch runs.
      const { readFileSync } = await import("node:fs");
      const pkgUrl = (name: string) => new URL(`../${name}`, import.meta.url);
      const toArrayBuffer = (buf: Uint8Array): ArrayBuffer =>
        buf.buffer.slice(buf.byteOffset, buf.byteOffset + buf.byteLength) as ArrayBuffer;
      const wasmBytes = readFileSync(pkgUrl("griddlyjs.wasm"));
      overrides.instantiateWasm = (
        info: WebAssembly.Imports,
        cb: (inst: WebAssembly.Instance) => void
      ) => {
        WebAssembly.instantiate(wasmBytes, info).then((result) => cb(result.instance));
        return {} as WebAssembly.WebAssemblyInstantiatedSource;
      };
      overrides.getPreloadedPackage = (name: string, _size: number) => {
        let path = name;
        if (!path.startsWith("/")) path = pkgUrl(name).pathname;
        return toArrayBuffer(readFileSync(path));
      };
    }
    const module = await wasmFactory(overrides as Parameters<typeof wasmFactory>[0]);
    return new Griddly(module);
  }

  /** Parse a GDY YAML string. Replaces any previously loaded GDY. */
  loadGDYString(gdyString: string): void {
    this.unload();
    try {
      this.gdy = this.griddlyjs.loadString(gdyString);
      this.playerCount = this.gdy.getPlayerCount();
    } catch (e) {
      rethrow(this.griddlyjs, e);
    }
  }

  /** Read a GDY file from the emscripten virtual FS (resources are preloaded at /resources). */
  loadGDYFile(path: string): void {
    const gdyString = this.module.FS.readFile(path, { encoding: "utf8" }) as string;
    this.loadGDYString(gdyString);
  }

  /** Create a game with the given observer ("Vector", "ASCII", "Sprite2D", ...). */
  createGame(observerName = "Vector"): GriddlyJSGameWrapper {
    if (!this.gdy) throw new Error("No GDY loaded — call loadGDYString()/loadGDYFile() first");
    try {
      this.game = this.gdy.createGame(observerName);
      this.players = [];
      for (let p = 0; p < this.playerCount; p++) {
        this.players.push(this.game.registerPlayer(`Player ${p}`, observerName));
      }
      this.game.init();
      return this.game;
    } catch (e) {
      rethrow(this.griddlyjs, e);
    }
  }

  loadLevel(levelId: number): void {
    this.requireGame().loadLevel(levelId);
  }

  loadLevelString(levelString: string): void {
    this.requireGame().loadLevelString(levelString);
  }

  reset(): number {
    return this.requireGame().reset();
  }

  seed(seed: number): void {
    this.requireGame().seedRandomGenerator(seed);
  }

  /** Step all players in parallel: one action array per player. */
  stepParallel(playerActions: number[][]): number {
    try {
      return this.requireGame().stepParallel(playerActions);
    } catch (e) {
      rethrow(this.griddlyjs, e);
    }
  }

  /** Convenience for single-player games. */
  step(action: number | number[]): number {
    const actions = Array.isArray(action) ? [action] : [[action]];
    for (let p = 1; p < this.playerCount; p++) actions.push(new Array(actions[0].length).fill(0));
    return this.stepParallel(actions);
  }

  observe(): unknown {
    return this.requireGame().observe();
  }

  /** RGBA pixels of the current render, with width/height. */
  async observeImage(): Promise<{ pixels: Uint8Array; width: number; height: number }> {
    const game = this.requireGame();
    const desc = game.getGlobalObservationDescription();
    const pixels = await Promise.resolve(game.observeImage());
    if (desc.Shape && desc.Shape.size() >= 3) {
      const shape = vectorToArray(desc.Shape);
      return { pixels, width: shape[1], height: shape[2] };
    }
    const side = Math.floor(Math.sqrt(pixels.length / 4));
    return { pixels, width: side, height: side };
  }

  get width(): number {
    return this.requireGame().getWidth();
  }

  get height(): number {
    return this.requireGame().getHeight();
  }

  getState(): unknown {
    return this.requireGame().getState();
  }

  getGlobalVariables(): Record<string, unknown> {
    const game = this.requireGame();
    return game.getGlobalVariables(game.getGlobalVariableNames());
  }

  getActionNames(): string[] {
    if (!this.gdy) throw new Error("No GDY loaded");
    return [...this.gdy.getExternalActionNames()];
  }

  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  getActionInputMappings(): Record<string, any> {
    if (!this.gdy) throw new Error("No GDY loaded");
    return this.gdy.getActionInputMappings();
  }

  getLevelCount(): number {
    if (!this.gdy) throw new Error("No GDY loaded");
    return this.gdy.getLevelCount();
  }

  /** Render-capable observers defined by the loaded GDY. */
  getRenderObserverNames(): string[] {
    const gdy = this.gdy;
    if (!gdy) throw new Error("No GDY loaded");
    const OT = this.module.ObserverType;
    const renderTypes = new Set([OT.SPRITE_2D, OT.BLOCK_2D, OT.ISOMETRIC]);
    return [...gdy.getDefinedObserverNames()].filter((n) =>
      renderTypes.has(gdy.getObserverType(n))
    );
  }

  /** Show the SDL/WebGL canvas (render observers only). */
  enableCanvasRendering(): void {
    this.module.setSDLWindowVisible(true);
  }

  readFile(path: string): string {
    return this.module.FS.readFile(path, { encoding: "utf8" }) as string;
  }

  /** Recursively list .yaml files under a directory in the virtual FS. */
  listYamlFiles(dir: string, out: string[] = []): string[] {
    for (const entry of this.module.FS.readdir(dir)) {
      if (entry === "." || entry === "..") continue;
      const full = `${dir}/${entry}`;
      if (this.module.FS.isDir(this.module.FS.stat(full).mode)) {
        this.listYamlFiles(full, out);
      } else if (entry.endsWith(".yaml")) {
        out.push(full);
      }
    }
    return out;
  }

  /** Release the current game and GDY. */
  unload(): void {
    try {
      if (this.game) {
        this.game.release();
        this.game.delete();
        this.game = null;
      }
      if (this.gdy) {
        this.gdy.delete();
        this.gdy = null;
      }
      this.players = [];
      this.playerCount = 0;
    } catch {
      // best-effort cleanup
    }
  }

  private requireGame(): GriddlyJSGameWrapper {
    if (!this.game) throw new Error("No game created — call createGame() first");
    return this.game;
  }
}

/**
 * Resolve `griddlyjs.wasm` / `griddlyjs.data` next to the package.
 * Works in bundlers (import.meta.url → emitted asset URL) and in Node
 * (file URL → local path).
 */
function defaultLocateFile(file: string, prefix: string): string {
  if (file !== "griddlyjs.wasm" && file !== "griddlyjs.data") return prefix + file;
  // dist/index.js -> ../griddlyjs.wasm resolves to the package root, where
  // the artifacts live. Bundlers (Vite) rewrite this to an emitted asset URL.
  const url = new URL(`../${file}`, import.meta.url);
  // Node's fetch() cannot read file:// URLs; hand emscripten a plain path so
  // its node fallback (fs.readFile) kicks in.
  if (url.protocol === "file:") return fileURLToPathSafe(url);
  return url.href;
}

function fileURLToPathSafe(url: URL): string {
  // minimal file:// -> path conversion (avoids importing node:url at runtime
  // in the browser bundle)
  return decodeURIComponent(url.pathname);
}

/**
 * Minimal DOM shim so the web-targeted emscripten module can initialize under
 * Node.js (used for headless Vector/ASCII observers and tests). No-op in the
 * browser, where the real DOM exists.
 */
function installNodeDomShim(): void {
  const g = globalThis as Record<string, unknown>;
  const fakeElement = () => ({
    style: {},
    width: 0,
    height: 0,
    getContext: () => null,
    addEventListener: () => {},
    removeEventListener: () => {},
    setAttribute: () => {},
    appendChild: () => {},
    remove: () => {},
    toDataURL: () => "",
    classList: { add: () => {}, remove: () => {} },
  });
  const doc = {
    createElement: fakeElement,
    getElementById: () => null,
    querySelector: () => null,
    addEventListener: () => {},
    removeEventListener: () => {},
    body: { appendChild: () => {}, removeChild: () => {} },
    head: { appendChild: () => {} },
    documentElement: { style: {} },
    hidden: false,
    visibilityState: "visible",
    title: "",
    activeElement: null,
    fullscreenElement: null,
    pointerLockElement: null,
    hasFocus: () => false,
  };
  Object.defineProperty(g, "document", { value: doc, configurable: true });
  Object.defineProperty(g, "window", { value: globalThis, configurable: true });
  if (typeof g.location === "undefined") {
    Object.defineProperty(g, "location", {
      value: {
        href: "http://localhost/",
        pathname: "/",
        protocol: "http:",
        host: "localhost",
        hostname: "localhost",
        port: "",
        search: "",
        hash: "",
      },
      configurable: true,
    });
  }
}

/** Create and initialize a Griddly instance. */
export async function createGriddly(options: GriddlyOptions = {}): Promise<Griddly> {
  return Griddly.create(options);
}
